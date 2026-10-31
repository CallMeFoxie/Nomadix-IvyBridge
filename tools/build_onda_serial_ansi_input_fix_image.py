from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fq_serial-clean-console.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fr"
    / "nomadix_serial_ansi_input_fix_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fr_serial-ansi-input-fix.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "46A426CE10D50E85EACF1396F9CB28720E9401AC0B77F7E032752013A715EB06"
)
builder.STAGE_NAME = "4FR"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FR based on Stage 4FQ.",
    "Retains the clean bidirectional serial AMI Setup console.",
    "Ages incomplete ANSI escape sequences only on the real 10 ms timer.",
    "Ignores frequent AMI WaitForKey polls for escape timeout accounting.",
    "Prevents arrow-key ESC prefixes from becoming Quit hotkeys.",
    "Maps UEFI black right/left pointers to ASCII > and <.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
