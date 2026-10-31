from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fi"
    / "nomadix_onda_terminal_direct_remaining_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fi_onda-terminal-direct-remaining.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FI"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FI based on hardware-tested Stage 4F4.",
    "Retains Stage 4FH deferred arming and private I/O backend.",
    "Uses only the firmware-resident ONDA Terminal driver.",
    "Bypasses ConnectController driver selection after its EFI_NOT_FOUND result.",
    "Enumerates installed EFI Driver Binding handles at ReadyToBoot.",
    "Calls Supported with the explicit PC-ANSI RemainingDevicePath.",
    "Calls Start only on a binding that returns EFI_SUCCESS.",
    "Prints exact candidate, Supported, Start, and match results.",
    "Does not load, patch, replace, or embed any Terminal PE.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
