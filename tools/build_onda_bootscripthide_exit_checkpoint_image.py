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
    / "TEST_ONLY_stage3k_onda-exact640-bootscripthide-exit-checkpoint.bin"
)

BOOT_SCRIPT_HIDE_FFS_OFFSET = 0x7CBEF8
BOOT_SCRIPT_HIDE_FFS_SIZE = 0xC60
BOOT_SCRIPT_HIDE_GUID = "9B8A0C3A-5186-4B55-89F4-CAFDE613DAB1"
BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET = 0x7CC3B6
ORIGINAL_INSTRUCTION = bytes.fromhex("5E 5B")
CHECKPOINT_LOOP = bytes.fromhex("EB FE")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != uuid.UUID(BOOT_SCRIPT_HIDE_GUID).bytes_le:
        raise ValueError("BootScriptHidePei GUID not found at the expected offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected BootScriptHidePei FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("BootScriptHidePei does not use a data checksum")

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
        raise ValueError("Invalid BootScriptHidePei FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid BootScriptHidePei FFS data checksum")


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3B base is not 8 MiB")
    if (
        base[
            BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET :
            BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET + 2
        ]
        != ORIGINAL_INSTRUCTION
    ):
        raise ValueError("Unexpected BootScriptHidePei epilogue bytes")

    result = bytearray(base)
    result[
        BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET :
        BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET + 2
    ] = CHECKPOINT_LOOP
    recalculate_ffs_checksums(
        result, BOOT_SCRIPT_HIDE_FFS_OFFSET, BOOT_SCRIPT_HIDE_FFS_SIZE
    )
    validate_ffs_checksums(
        result, BOOT_SCRIPT_HIDE_FFS_OFFSET, BOOT_SCRIPT_HIDE_FFS_SIZE
    )

    allowed = {
        BOOT_SCRIPT_HIDE_FFS_OFFSET + 16,
        BOOT_SCRIPT_HIDE_FFS_OFFSET + 17,
        BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET,
        BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET + 1,
    }
    changed = {
        index for index, (old, new) in enumerate(zip(base, result)) if old != new
    }
    if not changed <= allowed or BOOT_SCRIPT_HIDE_EPILOGUE_OFFSET not in changed:
        raise ValueError(f"Unexpected changed offsets: {sorted(changed ^ allowed)}")

    OUTPUT.write_bytes(result)
    metadata = [
        "Diagnostic ONDA Stage 3K with a checkpoint at the common "
        "BootScriptHidePei epilogue.",
        "Base is Stage 3B, so WdtAppPei remains disabled.",
        "BootScriptHidePei executes normally; every return path reaches this "
        "epilogue and then loops forever.",
        "Purpose: determine whether the first PEIM after SaInitPeim completes.",
        f"base_sha256={sha256(base)}",
        f"image_sha256={sha256(result)}",
        f"dxe_header4k_sha256={sha256(result[0x420000:0x421000])}",
        f"onda_fv640_sha256={sha256(result[0x760000:0x800000])}",
        "checkpoint_patch=0x7CC3B6: 5E 5B -> EB FE",
        f"changed_offsets={','.join(f'0x{offset:X}' for offset in sorted(changed))}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[4:]))


if __name__ == "__main__":
    main()
