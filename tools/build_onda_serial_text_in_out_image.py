from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fl_serial-text-out.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fm"
    / "nomadix_serial_text_in_out_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fm_serial-text-in-out.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "EBE2BA4FB89197C646816B6707B1881C6FF18ED44098BF43A9280BF59827129E"
)
builder.STAGE_NAME = "4FM"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FM based on hardware-tested Stage 4FL.",
    "Retains the working COM1 Serial I/O and serial Simple Text Output.",
    "Adds EFI Simple Text Input on the same physical COM1 handle.",
    "Selects the serial input protocol as SystemTable ConIn.",
    "Uses a nonblocking 10 ms timer to poll the 16550 receive FIFO.",
    "Implements WaitForKey and a 32-entry EFI input-key queue.",
    "Accepts ASCII, Enter, Escape, Backspace, Tab, and raw Delete.",
    "Parses ANSI arrows, Home, End, Insert, Delete, Page Up/Down, and F1-F12.",
    "Recalculates the EFI system-table CRC after changing ConIn.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Does not load, patch, replace, or invoke an existing Terminal driver.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Hardware-tested: ConIn is accepted and keyboard-not-detected disappears.",
    "AMI hotkeys do not consume ESC or DEL without Simple Text Input Ex.",
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
    "included_stage4f3_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f3"
            / "nomadix_serial_diag_dxe.c"
        ).read_bytes()
    ),
    "included_stage4f2_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f2"
            / "nomadix_platform_dxe.c"
        ).read_bytes()
    ),
]


if __name__ == "__main__":
    builder.main()
