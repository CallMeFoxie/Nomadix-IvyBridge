from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fp_serial-setup-console.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fq"
    / "nomadix_serial_clean_console_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fq_serial-clean-console.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "BE96A0365A63EB2FD3E4E424EFA831E7196B51BEB2127AF43B6DD30F88852062"
)
builder.STAGE_NAME = "4FQ"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FQ based on Stage 4FP.",
    "Retains complete bidirectional serial AMI Setup access.",
    "Disables Stage 4F3 serial-handle transition diagnostics.",
    "Disables Stage 4F4 text-output transition diagnostics.",
    "Disables Stage 4FN RX, key, ReadKeyEx, and notify trace output.",
    "Prevents direct UART debug writes from corrupting the ANSI Setup screen.",
    "Retains only short initialization markers before AMI draws the console.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
