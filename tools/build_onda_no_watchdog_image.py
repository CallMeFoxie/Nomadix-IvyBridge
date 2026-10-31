from __future__ import annotations

import hashlib
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "output" / "onda-exact" / "TEST_ONLY_stage3a_onda-exact640.bin"
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage3b_onda-exact640-no-wdtapp.bin"
)

ONDA_TO_TARGET_DELTA = 0x400000
WDT_APP_FFS_OFFSET = 0x3BB708 + ONDA_TO_TARGET_DELTA
WDT_APP_FFS_SIZE = 0x42C
WDT_APP_GUID = "0F69F6D7-0E4B-43A6-BFC2-6871694369B0"
WDT_APP_ENTRY_OFFSET = 0x3BB99C + ONDA_TO_TARGET_DELTA
ORIGINAL_ENTRY = bytes.fromhex("55 8B EC")
NOOP_ENTRY = bytes.fromhex("31 C0 C3")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != uuid.UUID(WDT_APP_GUID).bytes_le:
        raise ValueError("WdtAppPei GUID not found at the expected offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected WdtAppPei FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("WdtAppPei does not use a data checksum")

    image[offset + 17] = (-sum(image[offset + 24 : offset + size])) & 0xFF
    header = bytearray(image[offset : offset + 24])
    header[16] = 0
    header[17] = 0
    header[23] = 0
    image[offset + 16] = (-sum(header)) & 0xFF


def validate_ffs_checksums(image: bytes, offset: int, size: int) -> None:
    header = bytearray(image[offset : offset + 24])
    stored_header_checksum = header[16]
    header[16] = 0
    header[17] = 0
    header[23] = 0
    if (sum(header) + stored_header_checksum) & 0xFF:
        raise ValueError("Invalid WdtAppPei FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid WdtAppPei FFS data checksum")


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3A base is not 8 MiB")
    if base[WDT_APP_ENTRY_OFFSET : WDT_APP_ENTRY_OFFSET + 3] != ORIGINAL_ENTRY:
        raise ValueError("Unexpected WdtAppPei entry-point bytes")

    result = bytearray(base)
    result[WDT_APP_ENTRY_OFFSET : WDT_APP_ENTRY_OFFSET + 3] = NOOP_ENTRY
    recalculate_ffs_checksums(result, WDT_APP_FFS_OFFSET, WDT_APP_FFS_SIZE)
    validate_ffs_checksums(result, WDT_APP_FFS_OFFSET, WDT_APP_FFS_SIZE)

    allowed = {
        WDT_APP_FFS_OFFSET + 16,
        WDT_APP_FFS_OFFSET + 17,
        WDT_APP_ENTRY_OFFSET,
        WDT_APP_ENTRY_OFFSET + 1,
        WDT_APP_ENTRY_OFFSET + 2,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if not changed <= allowed or WDT_APP_ENTRY_OFFSET not in changed:
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3B with WdtAppPei disabled.",
        "WdtPei and its PPI remain unchanged; only WdtAppPei entry returns EFI_SUCCESS.",
        "Purpose: prevent the observed 10-second TCO watchdog arm during PEI diagnosis.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "entry_patch=0x7BB99C: 55 8B EC -> 31 C0 C3",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text("\n".join(metadata) + "\n", encoding="ascii")
    print(OUTPUT)
    print("\n".join(metadata[3:]))


if __name__ == "__main__":
    main()
