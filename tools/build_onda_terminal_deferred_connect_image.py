from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fg"
    / "nomadix_onda_terminal_deferred_connect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fg_onda-terminal-deferred-connect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FG"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FG based on hardware-tested Stage 4F4.",
    "Supersedes failed Stage 4FF automatic ONDA Terminal exposure.",
    "Uses only the firmware-resident ONDA Terminal driver.",
    "Publishes the original safe Stage 4F4 vendor path during DXE dispatch.",
    "Publishes the AMI private container with its $SIO signature initially zero.",
    "Prevents BDS from automatically starting ONDA Terminal before diagnostics.",
    "At ReadyToBoot only, changes the RAM path to ACPI PNP0501 UID0.",
    "At the same point, writes $SIO and prints DEFERRED-ARMED.",
    "Then explicitly requests the PC-ANSI child through ConnectController.",
    "Does not load, patch, replace, or embed any Terminal PE.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
