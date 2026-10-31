from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fn_serial-text-input-ex.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fo"
    / "nomadix_serial_escape_scan_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fo_serial-escape-scan.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "FC23B8490CAC80C9CFB34A45993F79E803895001DF46CA2C628E10B5912D8352"
)
builder.STAGE_NAME = "4FO"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FO based on hardware-tested Stage 4FN.",
    "Retains working serial output, UART RX, key parsing, and Text Input Ex.",
    "Changes standalone Escape from Unicode 0x001B to UEFI SCAN_ESC 0x0017.",
    "Returns Unicode zero for Escape as expected by AMI keyboard hotkeys.",
    "Leaves all ANSI multi-byte key mappings unchanged.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Hardware-tested: SCAN_ESC enters AMI Setup over serial.",
    "Repeated queued Escape keys reopen Quit Without Saving; fixed by Stage 4FP.",
    "included_stage4fn_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fn"
            / "nomadix_serial_text_input_ex_dxe.c"
        ).read_bytes()
    ),
    "included_stage4fm_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fm"
            / "nomadix_serial_text_in_out_dxe.c"
        ).read_bytes()
    ),
]


if __name__ == "__main__":
    builder.main()
