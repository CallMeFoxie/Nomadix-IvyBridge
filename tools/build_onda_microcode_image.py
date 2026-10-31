from __future__ import annotations

import hashlib
import struct
import subprocess
import uuid
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4c_full-onda-bootstack-lanner-me8.bin"
)
MICROCODE = ROOT / "ref microcode" / "intel-ucode" / "06-3a-09"
LICENSE = ROOT / "ref microcode" / "license"
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f1_intel-306a9-microcode-21.bin"
)
UEFI_EXTRACT = ROOT / "tools" / "UEFIExtract_NE_A76_win64" / "UEFIExtract.exe"

MICROCODE_GUID = uuid.UUID("17088572-377F-44EF-8F4E-B09FFF46A070")
EXPECTED_BASE_SHA256 = (
    "8523A25786DA11675E5DE769E77DFE2347DFEC818E6E83D4E4737068B79FB4B7"
)
EXPECTED_MICROCODE_SHA256 = (
    "CA2FA408851D20E4F9993F5A3B60D41F17A6AE3B8C4BFA0B730FD162ABAEA885"
)
EXPECTED_FFS_OFFSETS = (0x7059E8, 0x787438)
EXPECTED_FFS_SIZE = 0x16018
TARGET_SIGNATURE = 0x000306A9
OLD_REVISION = 0x1C
NEW_REVISION = 0x21

EXPECTED_RECORDS = (
    (0x000306A6, 0x04, 0x2000),
    (0x000306A9, 0x1C, 0x3000),
    (0x000306A8, 0x10, 0x2800),
    (0x000306A5, 0x07, 0x2400),
    (0x000306A4, 0x07, 0x2400),
    (0x000306A2, 0x08, 0x2000),
    (0x000206A7, 0x29, 0x2800),
    (0x000306A9, 0x1C, 0x3000),
    (0x000206A7, 0x29, 0x2800),
)


@dataclass(frozen=True)
class Microcode:
    data: bytes
    revision: int
    date: int
    signature: int
    flags: int


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def parse_microcode(data: bytes) -> Microcode:
    if len(data) < 48 or len(data) % 4:
        raise ValueError("Intel microcode has an invalid length")
    (
        header_version,
        revision,
        date,
        signature,
        _checksum,
        loader_revision,
        flags,
        data_size,
        total_size,
    ) = struct.unpack_from("<9I", data)
    size = total_size or 0x800
    expected_data_size = data_size or 0x7D0
    if header_version != 1 or loader_revision != 1:
        raise ValueError("Unsupported Intel microcode header")
    if size != len(data) or expected_data_size + 48 != size:
        raise ValueError("Intel microcode header length does not match its file")
    if sum(struct.unpack(f"<{size // 4}I", data)) & 0xFFFFFFFF:
        raise ValueError(
            f"Intel microcode checksum is invalid for CPUID 0x{signature:08X}"
        )
    return Microcode(
        data=data,
        revision=revision,
        date=date,
        signature=signature,
        flags=flags,
    )


def parse_ffs_records(ffs: bytes) -> list[Microcode]:
    if len(ffs) != EXPECTED_FFS_SIZE:
        raise ValueError(f"Unexpected microcode FFS length 0x{len(ffs):X}")
    if ffs[:16] != MICROCODE_GUID.bytes_le:
        raise ValueError("Microcode FFS GUID does not match")
    if int.from_bytes(ffs[20:23], "little") != len(ffs):
        raise ValueError("Microcode FFS header length does not match")
    if ffs[18] != 0x01 or ffs[19] != 0x48 or ffs[23] != 0xF8:
        raise ValueError("Unexpected microcode FFS type, attributes, or state")
    header = bytearray(ffs[:24])
    header[17] = 0
    header[23] = 0
    if sum(header) & 0xFF:
        raise ValueError("Microcode FFS header checksum is invalid")
    if (sum(ffs[24:]) + ffs[17]) & 0xFF:
        raise ValueError("Microcode FFS data checksum is invalid")

    records: list[Microcode] = []
    cursor = 24
    while cursor + 48 <= len(ffs):
        if ffs[cursor : cursor + 48] == b"\xFF" * 48:
            if ffs[cursor:] != b"\xFF" * (len(ffs) - cursor):
                raise ValueError("Non-erased data follows the microcode records")
            break
        total_size = struct.unpack_from("<I", ffs, cursor + 32)[0] or 0x800
        end = cursor + total_size
        if end > len(ffs):
            raise ValueError("Microcode record extends beyond its FFS")
        records.append(parse_microcode(ffs[cursor:end]))
        cursor = end
    return records


def rebuild_ffs(ffs: bytes, replacement: Microcode) -> bytes:
    records = parse_ffs_records(ffs)
    actual = tuple(
        (record.signature, record.revision, len(record.data)) for record in records
    )
    if actual != EXPECTED_RECORDS:
        raise ValueError(f"Unexpected Stage 4C microcode records: {actual!r}")

    result_records: list[bytes] = []
    replaced = False
    for record in records:
        if record.signature != TARGET_SIGNATURE:
            result_records.append(record.data)
        elif not replaced:
            result_records.append(replacement.data)
            replaced = True
        elif record.revision != OLD_REVISION or record.data != records[1].data:
            raise ValueError("The second 306A9 record is not the expected duplicate")
    if not replaced:
        raise ValueError("No 306A9 microcode was replaced")

    body = b"".join(result_records)
    capacity = len(ffs) - 24
    if len(body) > capacity:
        raise ValueError("Updated microcode records do not fit their FFS")
    body += b"\xFF" * (capacity - len(body))

    result = bytearray(ffs[:24] + body)
    result[17] = (-sum(body)) & 0xFF
    parse_ffs_records(bytes(result))
    updated = [
        record
        for record in parse_ffs_records(bytes(result))
        if record.signature == TARGET_SIGNATURE
    ]
    if len(updated) != 1 or updated[0].revision != NEW_REVISION:
        raise ValueError("Updated FFS does not contain exactly one 306A9 revision 0x21")
    return bytes(result)


def verify_only_ffs_changed(before: bytes, after: bytes) -> None:
    cursor = 0
    for offset in EXPECTED_FFS_OFFSETS:
        if before[cursor:offset] != after[cursor:offset]:
            raise ValueError(f"Unexpected change before FFS at 0x{offset:X}")
        cursor = offset + EXPECTED_FFS_SIZE
    if before[cursor:] != after[cursor:]:
        raise ValueError("Unexpected change after the final microcode FFS")


def generate_report() -> None:
    process = subprocess.run(
        [str(UEFI_EXTRACT), str(OUTPUT), "report"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if process.returncode:
        output = "\n".join(
            part.strip() for part in (process.stdout, process.stderr) if part.strip()
        )
        raise RuntimeError(f"UEFIExtract failed with exit code {process.returncode}:\n{output}")
    report = OUTPUT.with_suffix(OUTPUT.suffix + ".report.txt")
    if not report.is_file():
        raise RuntimeError("UEFIExtract did not create the expected report")


def main() -> None:
    base = BASE.read_bytes()
    source = MICROCODE.read_bytes()
    if sha256(base) != EXPECTED_BASE_SHA256:
        raise ValueError("Stage 4C base hash does not match the hardware-tested image")
    if sha256(source) != EXPECTED_MICROCODE_SHA256:
        raise ValueError("Intel 06-3a-09 source hash does not match the pinned blob")
    if not LICENSE.is_file():
        raise FileNotFoundError("Intel microcode redistribution license is missing")

    replacement = parse_microcode(source)
    if (
        replacement.signature != TARGET_SIGNATURE
        or replacement.revision != NEW_REVISION
        or replacement.flags != 0x12
        or replacement.date != 0x02132019
    ):
        raise ValueError("Intel 06-3a-09 source header does not match expectations")

    result = bytearray(base)
    for offset in EXPECTED_FFS_OFFSETS:
        original = base[offset : offset + EXPECTED_FFS_SIZE]
        result[offset : offset + EXPECTED_FFS_SIZE] = rebuild_ffs(
            original, replacement
        )
    final = bytes(result)
    verify_only_ffs_changed(base, final)

    OUTPUT.write_bytes(final)
    metadata = [
        "Experimental Stage 4F1 based on hardware-tested Stage 4C.",
        "Updates Intel CPUID 306A9 microcode from revision 0x1C to 0x21.",
        "Hardware verified with Xeon E3-1220 v2: OpenWrt boots and reports "
        "active microcode revision 0x21.",
        "Each raw microcode FFS originally contained two identical 306A9/0x1C "
        "records; each pair is replaced by one Intel 306A9/0x21 record.",
        "All non-306A9 microcode records remain bit-identical and in their "
        "original order.",
        "Intel source: https://github.com/intel/"
        "Intel-Linux-Processor-Microcode-Data-Files/blob/main/"
        "intel-ucode/06-3a-09",
        "Intel source header: CPUID=000306A9 revision=00000021 "
        "date=02132019 flags=00000012 size=00003800",
        f"base_sha256={sha256(base)}",
        f"intel_06-3a-09_sha256={sha256(source)}",
        f"image_sha256={sha256(final)}",
        "modified_ffs_offsets=" + ",".join(
            f"0x{offset:06X}" for offset in EXPECTED_FFS_OFFSETS
        ),
        f"modified_ffs_size=0x{EXPECTED_FFS_SIZE:X}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    generate_report()
    print(OUTPUT)
    print("\n".join(metadata[5:]))


if __name__ == "__main__":
    main()
