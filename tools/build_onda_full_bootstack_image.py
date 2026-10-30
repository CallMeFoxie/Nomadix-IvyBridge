from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage3b_onda-exact640-no-wdtapp.bin"
)
DONOR = ROOT / "ref bios" / "ONDA H61ITX01.BIN"
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4c_full-onda-bootstack-lanner-me8.bin"
)

TARGET_BOOTSTACK_START = 0x420000
DONOR_BIOS_START = 0x180000
DONOR_PEI_START = 0x360000
TARGET_DONOR_START = 0x580000
TARGET_PEI_START = 0x760000
FLASH_END = 0x800000


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def main() -> None:
    base = BASE.read_bytes()
    donor = DONOR.read_bytes()
    if len(base) != 0x800000:
        raise ValueError("Stage 3B base is not 8 MiB")
    if len(donor) != 0x400000:
        raise ValueError("ONDA donor is not 4 MiB")

    result = bytearray(base)
    result[TARGET_BOOTSTACK_START:TARGET_DONOR_START] = b"\xFF" * (
        TARGET_DONOR_START - TARGET_BOOTSTACK_START
    )
    result[TARGET_DONOR_START:TARGET_PEI_START] = donor[
        DONOR_BIOS_START:DONOR_PEI_START
    ]

    if result[:TARGET_BOOTSTACK_START] != base[:TARGET_BOOTSTACK_START]:
        raise ValueError("Descriptor, ME, or lower Lanner data changed")
    if result[TARGET_PEI_START:FLASH_END] != base[TARGET_PEI_START:FLASH_END]:
        raise ValueError("Stage 3B ONDA PEI region changed")
    if result[TARGET_DONOR_START:TARGET_PEI_START] != donor[
        DONOR_BIOS_START:DONOR_PEI_START
    ]:
        raise ValueError("ONDA donor DXE/NVRAM stack copy mismatch")

    OUTPUT.write_bytes(result)
    metadata = [
        "Experimental Stage 4C with the complete top-aligned ONDA BIOS "
        "boot stack and the Lanner descriptor/ME8 base.",
        "Hardware verified with Intel Xeon E3-1220 v2 (Ivy Bridge, "
        "CPUID 306A9): OpenWrt boots with all four cores enabled.",
        "0x000000-0x41FFFF: retained from Stage 3B.",
        "0x420000-0x57FFFF: erased to FF so the old Lanner DXE FV cannot "
        "be enumerated.",
        "0x580000-0x75FFFF: copied from ONDA donor 0x180000-0x35FFFF.",
        "0x760000-0x7FFFFF: retained Stage 3B ONDA PEI with WdtAppPei "
        "disabled.",
        f"base_sha256={sha256(base)}",
        f"donor_sha256={sha256(donor)}",
        f"image_sha256={sha256(result)}",
        f"bootstack_sha256={sha256(result[0x420000:0x800000])}",
        f"onda_prepei_sha256={sha256(result[0x580000:0x760000])}",
        f"onda_pei_sha256={sha256(result[0x760000:0x800000])}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[5:]))


if __name__ == "__main__":
    main()
