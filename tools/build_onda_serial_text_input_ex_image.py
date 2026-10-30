from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fm_serial-text-in-out.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fn"
    / "nomadix_serial_text_input_ex_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fn_serial-text-input-ex.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "4D64CA4DD66E0662C0E4ED940EF972A650DA8279A46632EB7A01800D23F4BF1D"
)
builder.STAGE_NAME = "4FN"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FN based on hardware-tested output Stage 4FL.",
    "Retains Stage 4FM Simple Text Input and UART polling.",
    "Adds EFI Simple Text Input Ex for AMI TSE hotkey consumption.",
    "Implements ReadKeyStrokeEx, WaitForKeyEx, and toggle state.",
    "Implements 16 standard RegisterKeyNotify callback slots.",
    "Publishes Simple Text Input Ex on the existing COM1 console handle.",
    "Logs the first received UART byte and first queued EFI key.",
    "Logs the first ReadKeyStrokeEx call and first notification registration.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Does not load, patch, replace, or invoke an existing Terminal driver.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Hardware-tested: UART RX, key decoding, and ReadKeyStrokeEx all succeed.",
    "AMI rejects Unicode ESC; Stage 4FO corrects it to UEFI SCAN_ESC.",
    "included_stage4fm_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fm"
            / "nomadix_serial_text_in_out_dxe.c"
        ).read_bytes()
    ),
    "included_stage4fl_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fl"
            / "nomadix_serial_text_out_dxe.c"
        ).read_bytes()
    ),
    "included_stage4f4_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f4"
            / "nomadix_serial_io_dxe.c"
        ).read_bytes()
    ),
]


if __name__ == "__main__":
    builder.main()
