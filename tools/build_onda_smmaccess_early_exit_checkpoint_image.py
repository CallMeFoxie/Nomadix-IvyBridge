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
    / "TEST_ONLY_stage3i_onda-exact640-smmaccess-early-exit-checkpoint.bin"
)

SMM_ACCESS_FFS_OFFSET = 0x7C6D78
SMM_ACCESS_FFS_SIZE = 0x884
SMM_ACCESS_GUID = "6ECFCE51-5724-450C-A38A-58553E954422"
SMM_ACCESS_EARLY_SUCCESS_OFFSET = 0x7C721D
ORIGINAL_INSTRUCTION = bytes.fromhex("33 C0")
CHECKPOINT_LOOP = bytes.fromhex("EB FE")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != uuid.UUID(SMM_ACCESS_GUID).bytes_le:
        raise ValueError("SmmAccessPeim GUID not found at the expected offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected SmmAccessPeim FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("SmmAccessPeim does not use a data checksum")

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
        raise ValueError("Invalid SmmAccessPeim FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid SmmAccessPeim FFS data checksum")


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3B base is not 8 MiB")
    if (
        base[
            SMM_ACCESS_EARLY_SUCCESS_OFFSET : SMM_ACCESS_EARLY_SUCCESS_OFFSET
            + 2
        ]
        != ORIGINAL_INSTRUCTION
    ):
        raise ValueError("Unexpected SmmAccessPeim early-success bytes")

    result = bytearray(base)
    result[
        SMM_ACCESS_EARLY_SUCCESS_OFFSET : SMM_ACCESS_EARLY_SUCCESS_OFFSET + 2
    ] = CHECKPOINT_LOOP
    recalculate_ffs_checksums(
        result, SMM_ACCESS_FFS_OFFSET, SMM_ACCESS_FFS_SIZE
    )
    validate_ffs_checksums(
        result, SMM_ACCESS_FFS_OFFSET, SMM_ACCESS_FFS_SIZE
    )

    allowed = {
        SMM_ACCESS_FFS_OFFSET + 16,
        SMM_ACCESS_FFS_OFFSET + 17,
        SMM_ACCESS_EARLY_SUCCESS_OFFSET,
        SMM_ACCESS_EARLY_SUCCESS_OFFSET + 1,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if (
        not changed <= allowed
        or SMM_ACCESS_EARLY_SUCCESS_OFFSET not in changed
    ):
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3I with a checkpoint at the SmmAccessPeim early-success return.",
        "Base is Stage 3B, so WdtAppPei remains disabled.",
        "This is the normal cold-boot path when BootMode is not S3 resume (0x11).",
        "Purpose: confirm that SmmAccessPeim returns early before SaInitPeim dispatch.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "checkpoint_patch=0x7C721D: 33 C0 -> EB FE",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[4:]))


if __name__ == "__main__":
    main()
