from __future__ import annotations

import hashlib
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage3b_onda-exact640-no-wdtapp.bin"
)
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage3r_onda-exact640-dxe-image-relocated-checkpoint.bin"
)

CORE_PEI_FFS_OFFSET = 0x7A27C0
CORE_PEI_FFS_SIZE = 0xD2B2
CORE_PEI_GUID = "92685943-D810-47FF-A112-CC8490776A1F"
DXE_IMAGE_RELOCATED_OFFSET = 0x7AA083
ORIGINAL_INSTRUCTION = bytes.fromhex("8B 45")
CHECKPOINT_LOOP = bytes.fromhex("EB FE")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != uuid.UUID(CORE_PEI_GUID).bytes_le:
        raise ValueError("CORE_PEI GUID not found at the expected offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected CORE_PEI FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("CORE_PEI does not use a data checksum")

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
        raise ValueError("Invalid CORE_PEI FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid CORE_PEI FFS data checksum")


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3B base is not 8 MiB")
    if (
        base[DXE_IMAGE_RELOCATED_OFFSET : DXE_IMAGE_RELOCATED_OFFSET + 2]
        != ORIGINAL_INSTRUCTION
    ):
        raise ValueError("Unexpected post-relocation checkpoint bytes")

    result = bytearray(base)
    result[DXE_IMAGE_RELOCATED_OFFSET : DXE_IMAGE_RELOCATED_OFFSET + 2] = (
        CHECKPOINT_LOOP
    )
    recalculate_ffs_checksums(result, CORE_PEI_FFS_OFFSET, CORE_PEI_FFS_SIZE)
    validate_ffs_checksums(result, CORE_PEI_FFS_OFFSET, CORE_PEI_FFS_SIZE)

    allowed = {
        CORE_PEI_FFS_OFFSET + 16,
        CORE_PEI_FFS_OFFSET + 17,
        DXE_IMAGE_RELOCATED_OFFSET,
        DXE_IMAGE_RELOCATED_OFFSET + 1,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if not changed <= allowed or DXE_IMAGE_RELOCATED_OFFSET not in changed:
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3R with a checkpoint after the DXE image "
        "loader's GetImageInfo, LoadImage, and RelocateImage operations.",
        "Base is Stage 3B, so WdtAppPei remains disabled.",
        "All three image-loader operations execute normally; reaching the "
        "result-copy and success-return path then loops forever.",
        "Purpose: determine whether DXE Core loading and relocation complete.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "checkpoint_patch=0x7AA083: 8B 45 -> EB FE",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[4:]))


if __name__ == "__main__":
    main()
