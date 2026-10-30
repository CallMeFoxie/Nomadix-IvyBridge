from __future__ import annotations

import hashlib
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "output" / "incremental-x7" / "TEST_ONLY_stage2a_dxe-microcode.bin"
DONOR = ROOT / "ref bios" / "ONDA H61ITX01.BIN"
OUTPUT_DIR = ROOT / "output" / "onda-exact"
OUTPUT = OUTPUT_DIR / "TEST_ONLY_stage3a_onda-exact640.bin"

DXE_START = 0x420000
OLD_DXE_SIZE = 0x360000
NEW_DXE_SIZE = 0x340000
NEW_DXE_END = DXE_START + NEW_DXE_SIZE
ONDA_FV_START = 0x360000
ONDA_FV_SIZE = 0xA0000
FLASH_END = 0x800000
LAST_DXE_DATA_END = 0x54E260


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def resize_fv_header(image: bytearray, offset: int, expected: int, new: int) -> None:
    if image[offset + 0x28 : offset + 0x2C] != b"_FVH":
        raise ValueError("Missing Lanner DXE FV signature")
    current = struct.unpack_from("<Q", image, offset + 0x20)[0]
    if current != expected:
        raise ValueError(f"Unexpected Lanner DXE FV size 0x{current:X}")

    header_size = struct.unpack_from("<H", image, offset + 0x30)[0]
    block_length = struct.unpack_from("<I", image, offset + 0x3C)[0]
    if new % block_length:
        raise ValueError("New DXE FV size is not block-aligned")

    struct.pack_into("<Q", image, offset + 0x20, new)
    struct.pack_into("<I", image, offset + 0x38, new // block_length)
    struct.pack_into("<H", image, offset + 0x32, 0)
    words = struct.unpack_from(
        f"<{header_size // 2}H", image, offset
    )
    struct.pack_into("<H", image, offset + 0x32, (-sum(words)) & 0xFFFF)


def main() -> None:
    base = BASE.read_bytes()
    donor = DONOR.read_bytes()
    if len(base) != FLASH_END:
        raise ValueError("Base image is not 8 MiB")
    if len(donor) != 0x400000:
        raise ValueError("ONDA donor image is not 4 MiB")
    if base[LAST_DXE_DATA_END:0x780000] != bytes([0xFF]) * (
        0x780000 - LAST_DXE_DATA_END
    ):
        raise ValueError("Expected Lanner DXE tail is not empty")

    onda_fv = donor[ONDA_FV_START : ONDA_FV_START + ONDA_FV_SIZE]
    if onda_fv[0x28:0x2C] != b"_FVH":
        raise ValueError("Missing ONDA PEI FV signature")
    if struct.unpack_from("<Q", onda_fv, 0x20)[0] != ONDA_FV_SIZE:
        raise ValueError("Unexpected ONDA PEI FV size")

    result = bytearray(base)
    resize_fv_header(result, DXE_START, OLD_DXE_SIZE, NEW_DXE_SIZE)
    result[NEW_DXE_END:FLASH_END] = onda_fv

    if result[NEW_DXE_END:FLASH_END] != onda_fv:
        raise ValueError("ONDA FV changed during insertion")
    if result[:DXE_START] != base[:DXE_START]:
        raise ValueError("Descriptor, ME, or NVRAM changed")
    if result[DXE_START + 0x48 : NEW_DXE_END] != base[
        DXE_START + 0x48 : NEW_DXE_END
    ]:
        raise ValueError("A Lanner DXE payload changed")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(result)
    metadata = [
        "Exact-layout ONDA H61 ITX Ivy PEI candidate.",
        "Base: verified Stage 2A DXE microcode image.",
        "Lanner DXE FV resized from 0x360000 to 0x340000; truncated tail was empty FF space.",
        "ONDA 640 KiB FV copied byte-for-byte from donor offset 0x360000 to flash offset 0x760000.",
        "Both donor and target map the ONDA FV to physical address 0xFFF60000.",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        f"source_onda_fv640_sha256={sha256(onda_fv)}",
    ]
    OUTPUT.with_suffix(".txt").write_text("\n".join(metadata) + "\n", encoding="ascii")
    print(OUTPUT)
    print("\n".join(metadata[-4:]))


if __name__ == "__main__":
    main()
