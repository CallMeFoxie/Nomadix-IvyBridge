from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fo_serial-escape-scan.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fp"
    / "nomadix_serial_setup_console_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fp_serial-setup-console.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "62529D5372DC1700528CBED63129E5CDEE18A8994ED53971B7127DC4FEC4378B"
)
builder.STAGE_NAME = "4FP"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FP based on hardware-tested Stage 4FO.",
    "Retains complete bidirectional serial AMI Setup access.",
    "Coalesces pending repeated Escape scan codes into one queued key.",
    "Prevents repeated Quit Without Saving dialogs after entering Setup.",
    "Maps UEFI box-drawing and block glyphs to stable ASCII characters.",
    "Avoids dependency on Minicom UTF-8 or CP437 conversion settings.",
    "Leaves other Unicode text encoded as UTF-8.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "included_stage4fo_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fo"
            / "nomadix_serial_escape_scan_dxe.c"
        ).read_bytes()
    ),
    "included_stage4fn_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fn"
            / "nomadix_serial_text_input_ex_dxe.c"
        ).read_bytes()
    ),
]


if __name__ == "__main__":
    builder.main()
