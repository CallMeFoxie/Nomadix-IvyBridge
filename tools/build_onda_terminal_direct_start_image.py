from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f6_terminal-reload.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f7"
    / "nomadix_terminal_direct_start_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f7_terminal-direct-start.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "8308A9FE9363FCD5D3A9CCFDB0F7D22EDFAF39B1417D2EB4094387EB5AA2D020"
)
builder.STAGE_NAME = "4F7"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4F7 based on hardware-tested Stage 4F6.",
    "Retains Stage 4F2 platform init and Stage 4F3/4F4 diagnostics.",
    "Retains Stage 4F4 COM1 publication and Stage 4F6 Terminal reload.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Enumerates EFI Driver Binding protocol instances after Terminal reload.",
    "Calls Supported directly for the COM1 controller on each binding.",
    "Calls Start directly on the first binding that returns EFI_SUCCESS.",
    "Prints non-unsupported candidates, version, status, and start result.",
    "Leaves all direct-start failures nonfatal after exact diagnostics.",
    "Stage 4F4 boot-transition diagnostics enumerate resulting text children.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Diagnostic-only image; it does not add serial ConOut or ConIn paths.",
    "included_stage4f6_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f6"
            / "nomadix_terminal_reload_dxe.c"
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
