from __future__ import annotations

import hashlib
import re
import struct
import uuid
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "post-stage1-boot-readback.bin"
X7_IMAGE = ROOT / "ref bios" / "x7-vb.bin"
LANNER_REPORT = ROOT / "bios_backup3.bin.report.txt"
X7_REPORT = ROOT / "ref bios" / "x7-vb.bin.report.txt"
OUTPUT = ROOT / "output" / "incremental-x7"

DXE_FV_START = 0x420000
DXE_FV_END = 0x780000
DXE_MICROCODE_OFFSET = 0x420048
DXE_USED_END = 0x53DA30

PEI_FV_START = 0x780000
PEI_FV_END = 0x800000
LANNER_PEI_SOURCE_START = 0x780000
LANNER_PEI_SOURCE_END = 0x800000
X7_PEI_SOURCE_START = 0x360000
X7_PEI_SOURCE_END = 0x400000

MICROCODE_GUID = "17088572-377F-44EF-8F4E-B09FFF46A070"
CORE_GUID = "92685943-D810-47FF-A112-CC8490776A1F"
SEC_GUID = "1BA0062E-C779-4582-8566-336AE8F78F09"
PAD_GUID = "078F54D4-CC22-4048-9E94-879C214D562F"
ALIGN_PAD_GUID = "E4536585-7909-4A60-B5C6-ECDEA6EBFB54"


@dataclass(frozen=True)
class FfsFile:
    guid: str
    name: str
    data: bytes


def parse_size(value: bytes) -> int:
    return int.from_bytes(value, "little")


def guid_from_header(header: bytes) -> str:
    return str(uuid.UUID(bytes_le=header[:16])).upper()


def parse_files(
    image: bytes, report: Path, start: int, end: int
) -> tuple[bytes, list[FfsFile], FfsFile]:
    files: list[FfsFile] = []
    sec: FfsFile | None = None
    lines = report.read_text(errors="replace").splitlines()

    for line in lines:
        columns = [column.strip() for column in line.split("|")]
        if len(columns) < 6 or columns[0] != "File":
            continue
        try:
            offset = int(columns[2], 16)
            size = int(columns[3], 16)
        except ValueError:
            continue
        description = columns[5]
        if not (start <= offset < end):
            continue
        if not description.startswith("---") or description.startswith("----"):
            continue

        blob = image[offset : offset + size]
        if len(blob) != size:
            raise ValueError(f"Truncated FFS file at 0x{offset:X}")
        guid = guid_from_header(blob)
        if guid == PAD_GUID:
            continue

        name = description[3:].strip().split(" | ")[-1]
        ffs = FfsFile(guid=guid, name=name, data=blob)
        if guid == SEC_GUID:
            sec = ffs
        else:
            files.append(ffs)

    if sec is None:
        raise ValueError(f"No SEC volume-top file found in {report}")
    return image[start : start + 0x48], files, sec


def align(value: int, alignment: int = 8) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


def make_pad(size: int, fixed_checksum: int) -> bytes:
    if size < 24 or size >= 0x1000000:
        raise ValueError(f"Invalid PAD FFS size 0x{size:X}")
    header = bytearray(24)
    header[:16] = uuid.UUID(PAD_GUID).bytes_le
    header[17] = fixed_checksum
    header[18] = 0xF0
    header[19] = 0
    header[20:23] = size.to_bytes(3, "little")
    header[23] = 0xF8
    checksum_header = header.copy()
    checksum_header[17] = 0
    checksum_header[23] = 0
    header[16] = (-sum(checksum_header)) & 0xFF
    return bytes(header) + bytes([0xFF]) * (size - len(header))


def build_pei_fv(
    header: bytes,
    files: list[FfsFile],
    sec: FfsFile,
    fixed_checksum: int,
) -> tuple[bytes, list[str]]:
    volume_size = PEI_FV_END - PEI_FV_START
    volume = bytearray([0xFF]) * volume_size
    volume[: len(header)] = header
    cursor = align(len(header))
    manifest: list[str] = []

    for ffs in files:
        cursor = align(cursor)
        end = cursor + len(ffs.data)
        if end > volume_size - len(sec.data):
            raise ValueError(f"{ffs.name} does not fit in PEI FV")
        volume[cursor:end] = ffs.data
        manifest.append(f"{cursor:06X} {len(ffs.data):06X} {ffs.guid} {ffs.name}")
        cursor = end

    cursor = align(cursor)
    sec_offset = volume_size - len(sec.data)
    pad_size = sec_offset - cursor
    if pad_size < 24 or pad_size % 8:
        raise ValueError(f"Invalid PAD size 0x{pad_size:X}")
    volume[cursor:sec_offset] = make_pad(pad_size, fixed_checksum)
    manifest.append(f"{cursor:06X} {pad_size:06X} {PAD_GUID} PAD")
    volume[sec_offset:] = sec.data
    manifest.append(f"{sec_offset:06X} {len(sec.data):06X} {sec.guid} {sec.name}")
    return bytes(volume), manifest


def replace_guid(
    files: list[FfsFile], replacement: FfsFile, target_guid: str | None = None
) -> list[FfsFile]:
    guid = target_guid or replacement.guid
    replaced = False
    result: list[FfsFile] = []
    for ffs in files:
        if ffs.guid == guid:
            result.append(replacement)
            replaced = True
        else:
            result.append(ffs)
    if not replaced:
        raise ValueError(f"GUID {guid} not found for replacement")
    return result


def find_guid(files: list[FfsFile], guid: str) -> FfsFile:
    for ffs in files:
        if ffs.guid == guid:
            return ffs
    raise ValueError(f"GUID {guid} not found")


def convert_fixed_checksum(ffs: FfsFile, fixed_checksum: int) -> FfsFile:
    if ffs.data[19] & 0x40:
        return ffs
    data = bytearray(ffs.data)
    data[16] = 0
    data[17] = fixed_checksum
    checksum_header = data[:24]
    checksum_header[17] = 0
    checksum_header[23] = 0
    data[16] = (-sum(checksum_header)) & 0xFF
    return FfsFile(guid=ffs.guid, name=ffs.name, data=bytes(data))


def resize_fv_header(header: bytes, size: int) -> bytes:
    result = bytearray(header)
    header_size = struct.unpack_from("<H", result, 0x30)[0]
    if header_size != len(header):
        raise ValueError("Unexpected FV header length")
    struct.pack_into("<Q", result, 0x20, size)
    block_length = struct.unpack_from("<I", result, 0x3C)[0]
    if size % block_length:
        raise ValueError("FV size is not a multiple of its block length")
    struct.pack_into("<I", result, 0x38, size // block_length)
    struct.pack_into("<H", result, 0x32, 0)
    words = struct.unpack(f"<{header_size // 2}H", result)
    struct.pack_into("<H", result, 0x32, (-sum(words)) & 0xFFFF)
    return bytes(result)


def replace_dxe_microcode(
    image: bytes, alignment_pad: FfsFile, microcode: FfsFile
) -> bytes:
    original_size = parse_size(image[DXE_MICROCODE_OFFSET + 20 : DXE_MICROCODE_OFFSET + 23])
    append_end = DXE_USED_END + len(alignment_pad.data) + len(microcode.data)
    if append_end >= DXE_FV_END:
        raise ValueError("Relocated DXE microcode does not fit")

    result = bytearray(image)
    result[DXE_MICROCODE_OFFSET : DXE_MICROCODE_OFFSET + original_size] = make_pad(
        original_size, 0x5A
    )
    result[DXE_USED_END : DXE_USED_END + len(alignment_pad.data)] = (
        convert_fixed_checksum(alignment_pad, 0x5A).data
    )
    microcode_start = DXE_USED_END + len(alignment_pad.data)
    result[microcode_start:append_end] = microcode.data
    return bytes(result)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def changed_ranges(before: bytes, after: bytes) -> list[tuple[int, int]]:
    ranges: list[tuple[int, int]] = []
    start: int | None = None
    for index, (old, new) in enumerate(zip(before, after)):
        if old != new and start is None:
            start = index
        elif old == new and start is not None:
            ranges.append((start, index - 1))
            start = None
    if start is not None:
        ranges.append((start, len(before) - 1))
    return ranges


def merge_ranges(
    ranges: list[tuple[int, int]], maximum_gap: int = 0x1000
) -> list[tuple[int, int]]:
    if not ranges:
        return []
    merged = [ranges[0]]
    for start, end in ranges[1:]:
        previous_start, previous_end = merged[-1]
        if start - previous_end - 1 <= maximum_gap:
            merged[-1] = (previous_start, end)
        else:
            merged.append((start, end))
    return merged


def validate_microcodes(blob: bytes) -> list[str]:
    results: list[str] = []
    cursor = 24
    while cursor + 48 <= len(blob):
        if blob[cursor : cursor + 48] == bytes([0xFF]) * 48:
            break
        (
            header_version,
            revision,
            date,
            signature,
            _checksum,
            _loader_revision,
            _flags,
            _data_size,
            total_size,
        ) = struct.unpack_from("<9I", blob, cursor)
        if header_version != 1:
            raise ValueError(f"Invalid microcode header at 0x{cursor:X}")
        size = total_size or 2048
        payload = blob[cursor : cursor + size]
        if len(payload) != size or sum(struct.unpack(f"<{size // 4}I", payload)) & 0xFFFFFFFF:
            raise ValueError(f"Invalid microcode checksum for CPUID 0x{signature:X}")
        results.append(
            f"CPUID={signature:08X} revision={revision:08X} date={date:08X} size={size:X}"
        )
        cursor += size
    return results


def validate_ffs(ffs: bytes, expected_guid: str | None = None) -> None:
    if len(ffs) < 24:
        raise ValueError("FFS file is shorter than its header")
    size = parse_size(ffs[20:23])
    if size != len(ffs):
        raise ValueError(f"FFS length mismatch: header 0x{size:X}, actual 0x{len(ffs):X}")
    guid = guid_from_header(ffs)
    if expected_guid is not None and guid != expected_guid:
        raise ValueError(f"Expected FFS {expected_guid}, found {guid}")
    header = bytearray(ffs[:24])
    header[17] = 0
    header[23] = 0
    if sum(header) & 0xFF:
        raise ValueError(f"Invalid FFS header checksum for {guid}")
    if ffs[19] & 0x40 and (sum(ffs[24:]) + ffs[17]) & 0xFF:
        raise ValueError(f"Invalid FFS data checksum for {guid}")


def validate_fv(volume: bytes) -> list[FfsFile]:
    if volume[0x28:0x2C] != b"_FVH":
        raise ValueError("Missing FV signature")
    fv_size = struct.unpack_from("<Q", volume, 0x20)[0]
    header_size = struct.unpack_from("<H", volume, 0x30)[0]
    if fv_size != len(volume):
        raise ValueError(f"FV length mismatch: header 0x{fv_size:X}, actual 0x{len(volume):X}")
    header_words = struct.unpack(f"<{header_size // 2}H", volume[:header_size])
    if sum(header_words) & 0xFFFF:
        raise ValueError("Invalid FV header checksum")

    files: list[FfsFile] = []
    cursor = align(header_size)
    while cursor < len(volume):
        if volume[cursor : cursor + 24] == bytes([0xFF]) * 24:
            raise ValueError(f"Unexpected free space before volume end at 0x{cursor:X}")
        size = parse_size(volume[cursor + 20 : cursor + 23])
        if size < 24 or cursor + size > len(volume):
            raise ValueError(f"Invalid FFS size 0x{size:X} at 0x{cursor:X}")
        blob = volume[cursor : cursor + size]
        validate_ffs(blob)
        guid = guid_from_header(blob)
        files.append(FfsFile(guid=guid, name=guid, data=blob))
        cursor = align(cursor + size)
    if cursor != len(volume):
        raise ValueError("FFS alignment exceeds FV boundary")
    return files


def write_image(
    name: str,
    baseline: bytes,
    image: bytes,
    description: str,
    manifest: list[str] | None = None,
) -> None:
    path = OUTPUT / f"{name}.bin"
    path.write_bytes(image)
    ranges = changed_ranges(baseline, image)
    windows = merge_ranges(ranges)
    metadata = [
        description,
        f"image_sha256={sha256(image)}",
        f"top512_sha256={sha256(image[PEI_FV_START:PEI_FV_END])}",
        f"changed_bytes={sum(end - start + 1 for start, end in ranges)}",
        "changed_windows="
        + ",".join(f"0x{start:06X}-0x{end:06X}" for start, end in windows),
    ]
    if manifest is not None:
        metadata.extend(["", "PEI manifest:", *manifest])
    path.with_suffix(".txt").write_text("\n".join(metadata) + "\n", encoding="ascii")


def main() -> None:
    baseline = BASELINE.read_bytes()
    x7 = X7_IMAGE.read_bytes()
    if len(baseline) != 0x800000:
        raise ValueError("Baseline is not an 8 MiB SPI image")
    if len(x7) != 0x400000:
        raise ValueError("X7 donor is not a 4 MiB SPI image")

    OUTPUT.mkdir(parents=True, exist_ok=True)

    lanner_header, lanner_files, lanner_sec = parse_files(
        baseline,
        LANNER_REPORT,
        LANNER_PEI_SOURCE_START,
        LANNER_PEI_SOURCE_END,
    )
    x7_header, x7_files, x7_sec = parse_files(
        x7, X7_REPORT, X7_PEI_SOURCE_START, X7_PEI_SOURCE_END
    )
    x7_header_512k = resize_fv_header(x7_header, PEI_FV_END - PEI_FV_START)
    x7_microcode = find_guid(x7_files, MICROCODE_GUID)
    x7_microcode_align_pad = find_guid(x7_files, ALIGN_PAD_GUID)
    x7_core = find_guid(x7_files, CORE_GUID)
    lanner_core = find_guid(lanner_files, CORE_GUID)
    x7_sec_lanner_fs = convert_fixed_checksum(x7_sec, 0x5A)
    validate_microcodes(x7_microcode.data)

    stage_2a = replace_dxe_microcode(
        baseline, x7_microcode_align_pad, x7_microcode
    )
    write_image(
        "TEST_ONLY_stage2a_dxe-microcode",
        baseline,
        stage_2a,
        "Baseline plus X7 Sandy/Ivy microcode FFS in the main DXE FV; original PEI/SEC unchanged.",
    )

    lanner_with_microcode = [
        *lanner_files,
        convert_fixed_checksum(x7_microcode_align_pad, 0x5A),
        x7_microcode,
    ]
    fv_2b, manifest_2b = build_pei_fv(
        lanner_header, lanner_with_microcode, lanner_sec, 0x5A
    )
    stage_2b = bytearray(stage_2a)
    stage_2b[PEI_FV_START:PEI_FV_END] = fv_2b
    stage_2b = bytes(stage_2b)
    write_image(
        "TEST_ONLY_stage2b_lanner-pei-plus-microcode",
        baseline,
        stage_2b,
        "Stage 2A plus X7 microcode FFS inserted into otherwise original Lanner PEI FV; executable PEIM and SEC code unchanged.",
        manifest_2b,
    )

    lanner_core_slot_pad = FfsFile(
        guid=PAD_GUID,
        name="Original CORE_PEI slot PAD",
        data=make_pad(len(lanner_core.data), 0x5A),
    )
    core_files = [
        *replace_guid(
            lanner_with_microcode, lanner_core_slot_pad, target_guid=CORE_GUID
        ),
        convert_fixed_checksum(x7_microcode_align_pad, 0x5A),
        x7_core,
    ]
    fv_2c, manifest_2c = build_pei_fv(
        lanner_header, core_files, lanner_sec, 0x5A
    )
    stage_2c = bytearray(stage_2a)
    stage_2c[PEI_FV_START:PEI_FV_END] = fv_2c
    stage_2c = bytes(stage_2c)
    write_image(
        "TEST_ONLY_stage2c_x7-core_lanner-sec-stack",
        baseline,
        stage_2c,
        "Stage 2B with only CORE_PEI replaced by X7; all Lanner PEIM and Lanner SEC retained.",
        manifest_2c,
    )

    fv_2d, manifest_2d = build_pei_fv(
        lanner_header, lanner_with_microcode, x7_sec_lanner_fs, 0x5A
    )
    stage_2d = bytearray(stage_2a)
    stage_2d[PEI_FV_START:PEI_FV_END] = fv_2d
    stage_2d = bytes(stage_2d)
    write_image(
        "TEST_ONLY_stage2d_lanner-core-stack_x7-sec",
        baseline,
        stage_2d,
        "Stage 2B with only SEC replaced by X7; all Lanner PEIM and Lanner CORE_PEI retained.",
        manifest_2d,
    )

    fv_2e, manifest_2e = build_pei_fv(
        lanner_header, core_files, x7_sec_lanner_fs, 0x5A
    )
    stage_2e = bytearray(stage_2a)
    stage_2e[PEI_FV_START:PEI_FV_END] = fv_2e
    stage_2e = bytes(stage_2e)
    write_image(
        "TEST_ONLY_stage2e_x7-core-sec_lanner-stack",
        baseline,
        stage_2e,
        "Stage 2B with X7 CORE_PEI and SEC; all other PEIM remain from Lanner.",
        manifest_2e,
    )

    x7_with_lanner_core = replace_guid(x7_files, lanner_core)
    fv_2f, manifest_2f = build_pei_fv(
        x7_header_512k,
        [convert_fixed_checksum(ffs, 0xAA) for ffs in x7_with_lanner_core],
        convert_fixed_checksum(lanner_sec, 0xAA),
        0xAA,
    )
    stage_2f = bytearray(stage_2a)
    stage_2f[PEI_FV_START:PEI_FV_END] = fv_2f
    stage_2f = bytes(stage_2f)
    write_image(
        "TEST_ONLY_stage2f_x7-stack_lanner-core-sec",
        baseline,
        stage_2f,
        "Complete X7 platform/MemoryInit PEIM set with Lanner CORE_PEI and Lanner SEC.",
        manifest_2f,
    )

    fv_2g, manifest_2g = build_pei_fv(
        x7_header_512k,
        x7_files,
        convert_fixed_checksum(lanner_sec, 0xAA),
        0xAA,
    )
    stage_2g = bytearray(stage_2a)
    stage_2g[PEI_FV_START:PEI_FV_END] = fv_2g
    stage_2g = bytes(stage_2g)
    write_image(
        "TEST_ONLY_stage2g_x7-stack-core_lanner-sec",
        baseline,
        stage_2g,
        "Complete X7 platform/MemoryInit PEIM set and X7 CORE_PEI with Lanner SEC.",
        manifest_2g,
    )

    fv_2h, manifest_2h = build_pei_fv(
        x7_header_512k, x7_files, x7_sec, 0xAA
    )
    stage_2h = bytearray(stage_2a)
    stage_2h[PEI_FV_START:PEI_FV_END] = fv_2h
    stage_2h = bytes(stage_2h)
    write_image(
        "TEST_ONLY_stage2h_full-x7-pei",
        baseline,
        stage_2h,
        "Complete internally consistent X7 PEI stack, CORE_PEI, microcodes, and SEC.",
        manifest_2h,
    )

    for path in sorted(OUTPUT.glob("*.bin")):
        image = path.read_bytes()
        if len(image) != len(baseline):
            raise ValueError(f"{path.name} has an invalid image size")
        if image[:DXE_FV_START] != baseline[:DXE_FV_START]:
            raise ValueError(f"{path.name} changes descriptor, ME, NVRAM, or pre-DXE data")
        dxe_size = struct.unpack_from("<Q", image, DXE_FV_START + 0x20)[0]
        if dxe_size != DXE_FV_END - DXE_FV_START:
            raise ValueError(f"{path.name} has an invalid DXE FV size")
        dxe_microcode_offset = DXE_USED_END + len(x7_microcode_align_pad.data)
        dxe_microcode_size = parse_size(
            image[dxe_microcode_offset + 20 : dxe_microcode_offset + 23]
        )
        dxe_microcode = image[
            dxe_microcode_offset : dxe_microcode_offset + dxe_microcode_size
        ]
        validate_ffs(dxe_microcode, MICROCODE_GUID)
        validate_microcodes(dxe_microcode)
        if path.name.startswith("TEST_ONLY_stage2a_"):
            if image[PEI_FV_START:] != baseline[PEI_FV_START:]:
                raise ValueError("Stage 2A unexpectedly changes PEI/SEC")
        else:
            validate_fv(image[PEI_FV_START:PEI_FV_END])

    print(f"Built 7 incremental images in {OUTPUT}")


if __name__ == "__main__":
    main()
