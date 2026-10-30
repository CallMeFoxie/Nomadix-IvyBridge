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
    / "TEST_ONLY_stage3e_onda-exact640-post-memory-checkpoint.bin"
)

MEMORY_INIT_FFS_OFFSET = 0x760048
MEMORY_INIT_FFS_SIZE = 0x273D2
MEMORY_INIT_GUID = "3B42EF57-16D3-44CB-8632-9FDB06B41451"
POST_MEMORY_INSTALL_OFFSET = 0x762471
ORIGINAL_INSTRUCTION = bytes.fromhex("8B 06")
CHECKPOINT_LOOP = bytes.fromhex("EB FE")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != uuid.UUID(MEMORY_INIT_GUID).bytes_le:
        raise ValueError("MemoryInit GUID not found at the expected offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected MemoryInit FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("MemoryInit does not use a data checksum")

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
        raise ValueError("Invalid MemoryInit FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid MemoryInit FFS data checksum")


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3B base is not 8 MiB")
    if (
        base[POST_MEMORY_INSTALL_OFFSET : POST_MEMORY_INSTALL_OFFSET + 2]
        != ORIGINAL_INSTRUCTION
    ):
        raise ValueError("Unexpected post-memory-install instruction bytes")

    result = bytearray(base)
    result[
        POST_MEMORY_INSTALL_OFFSET : POST_MEMORY_INSTALL_OFFSET + 2
    ] = CHECKPOINT_LOOP
    recalculate_ffs_checksums(
        result, MEMORY_INIT_FFS_OFFSET, MEMORY_INIT_FFS_SIZE
    )
    validate_ffs_checksums(
        result, MEMORY_INIT_FFS_OFFSET, MEMORY_INIT_FFS_SIZE
    )

    allowed = {
        MEMORY_INIT_FFS_OFFSET + 16,
        MEMORY_INIT_FFS_OFFSET + 17,
        POST_MEMORY_INSTALL_OFFSET,
        POST_MEMORY_INSTALL_OFFSET + 1,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if not changed <= allowed or POST_MEMORY_INSTALL_OFFSET not in changed:
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3E with a checkpoint loop after PEI memory installation.",
        "Base is Stage 3B, so WdtAppPei remains disabled.",
        "If the memory-install function returns, execution enters an intentional infinite loop.",
        "Purpose: distinguish failure inside memory installation from later PEI execution.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "checkpoint_patch=0x762471: 8B 06 -> EB FE",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[4:]))


if __name__ == "__main__":
    main()
