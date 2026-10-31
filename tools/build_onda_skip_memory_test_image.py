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
    / "TEST_ONLY_stage3c_onda-exact640-skip-memory-test.bin"
)

MEMORY_INIT_FFS_OFFSET = 0x760048
MEMORY_INIT_FFS_SIZE = 0x273D2
MEMORY_INIT_GUID = "3B42EF57-16D3-44CB-8632-9FDB06B41451"
MEMORY_TEST_OPERATION_OFFSET = 0x7610D5
ORIGINAL_OPERATION = bytes.fromhex("FF 75 FC")
IGNORE_OPERATION = bytes.fromhex("6A 00 90")


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
        raise ValueError("Stage 3A base is not 8 MiB")
    if (
        base[
            MEMORY_TEST_OPERATION_OFFSET : MEMORY_TEST_OPERATION_OFFSET + 3
        ]
        != ORIGINAL_OPERATION
    ):
        raise ValueError("Unexpected BaseMemoryTest operation argument bytes")

    result = bytearray(base)
    result[
        MEMORY_TEST_OPERATION_OFFSET : MEMORY_TEST_OPERATION_OFFSET + 3
    ] = IGNORE_OPERATION
    recalculate_ffs_checksums(
        result, MEMORY_INIT_FFS_OFFSET, MEMORY_INIT_FFS_SIZE
    )
    validate_ffs_checksums(
        result, MEMORY_INIT_FFS_OFFSET, MEMORY_INIT_FFS_SIZE
    )

    allowed = {
        MEMORY_INIT_FFS_OFFSET + 16,
        MEMORY_INIT_FFS_OFFSET + 17,
        MEMORY_TEST_OPERATION_OFFSET,
        MEMORY_TEST_OPERATION_OFFSET + 1,
        MEMORY_TEST_OPERATION_OFFSET + 2,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if not changed <= allowed or MEMORY_TEST_OPERATION_OFFSET not in changed:
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3C with the PEI base memory test skipped.",
        "MRC training, SPD discovery, memory-map creation, and PEI memory installation remain enabled.",
        "Only the BaseMemoryTest operation argument changes to Ignore (enum value zero).",
        "Purpose: determine whether the approximately 10-second failure occurs inside BaseMemoryTest.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "operation_patch=0x7610D5: FF 75 FC -> 6A 00 90",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[4:]))


if __name__ == "__main__":
    main()
