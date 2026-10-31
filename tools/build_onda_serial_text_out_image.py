from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fl"
    / "nomadix_serial_text_out_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fl_serial-text-out.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FL"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FL based on hardware-tested Stage 4F4.",
    "Retains Ivy Bridge support, platform initialization, and COM1 Serial I/O.",
    "Adds an output-only EFI Simple Text Output protocol over physical COM1.",
    "Implements an 80x25 UTF-8 ANSI terminal at 115200 8N1.",
    "Implements reset, output, mode, attribute, clear, cursor, and visibility.",
    "Installs Simple Text Output on the existing COM1 controller handle.",
    "Selects the serial terminal as ConOut and StdErr in the system table.",
    "Recalculates the EFI system-table CRC after changing console pointers.",
    "Leaves ConsoleInHandle and ConIn completely unchanged.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Does not load, patch, replace, or invoke the ONDA or Lanner Terminal.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Hardware-tested: AMI BDS preserves ConOut and renders POST over COM1.",
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
