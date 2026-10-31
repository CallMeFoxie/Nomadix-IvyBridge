from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fr_serial-ansi-input-fix.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fs"
    / "nomadix_serial_bold_attribute_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fs_serial-ansi-bold.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "49EFE71116C6DCFA4AC4A07815A44F2153024BE1C48585C9603450B55C470C1D"
)
builder.STAGE_NAME = "4FS"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FS based on Stage 4FR.",
    "Retains clean bidirectional AMI Setup and fixed ANSI key parsing.",
    "Maps UEFI foreground intensity to explicit ANSI SGR bold.",
    "Uses SGR 1 for intense text and SGR 22 for normal text.",
    "Uses standard 30-37 foreground colors instead of 90-97 bright colors.",
    "Improves selected-item visibility in Minicom terminals.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
