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
    / "TEST_ONLY_stage3g_onda-exact640-cpupei-exit-checkpoint.bin"
)

CPU_PEI_FFS_OFFSET = 0x7B3F30
CPU_PEI_FFS_SIZE = 0x152E
CPU_PEI_GUID = "2BB5AFA9-FF33-417B-8497-CB773C2B93BF"
CPU_PEI_EXIT_OFFSET = 0x7B4970
ORIGINAL_INSTRUCTION = bytes.fromhex("33 C0")
CHECKPOINT_LOOP = bytes.fromhex("EB FE")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != uuid.UUID(CPU_PEI_GUID).bytes_le:
        raise ValueError("CpuPei GUID not found at the expected offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected CpuPei FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("CpuPei does not use a data checksum")

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
        raise ValueError("Invalid CpuPei FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid CpuPei FFS data checksum")


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3B base is not 8 MiB")
    if (
        base[CPU_PEI_EXIT_OFFSET : CPU_PEI_EXIT_OFFSET + 2]
        != ORIGINAL_INSTRUCTION
    ):
        raise ValueError("Unexpected CpuPei exit instruction bytes")

    result = bytearray(base)
    result[CPU_PEI_EXIT_OFFSET : CPU_PEI_EXIT_OFFSET + 2] = CHECKPOINT_LOOP
    recalculate_ffs_checksums(result, CPU_PEI_FFS_OFFSET, CPU_PEI_FFS_SIZE)
    validate_ffs_checksums(result, CPU_PEI_FFS_OFFSET, CPU_PEI_FFS_SIZE)

    allowed = {
        CPU_PEI_FFS_OFFSET + 16,
        CPU_PEI_FFS_OFFSET + 17,
        CPU_PEI_EXIT_OFFSET,
        CPU_PEI_EXIT_OFFSET + 1,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if not changed <= allowed or CPU_PEI_EXIT_OFFSET not in changed:
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3G with a checkpoint at the normal CpuPei exit.",
        "Base is Stage 3B, so WdtAppPei remains disabled.",
        "CpuPei executes normally; reaching its success epilogue enters an infinite loop.",
        "Purpose: determine whether CpuPei completes or causes the beep/reset path internally.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "checkpoint_patch=0x7B4970: 33 C0 -> EB FE",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[4:]))


if __name__ == "__main__":
    main()
