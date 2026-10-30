from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f5_terminal-connect.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f6"
    / "nomadix_terminal_reload_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f6_terminal-reload.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "4896BDB7EC3558940C4EC3EB0ED653F7F3D56F8AD96CBEC0F72E4D75FBB2679C"
)
builder.STAGE_NAME = "4F6"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4F6 based on hardware-tested Stage 4F5.",
    "Retains Stage 4F2 platform init and Stage 4F3/4F4 diagnostics.",
    "Retains the working Stage 4F4 COM1 EFI Serial I/O protocol publisher.",
    "Supersedes the unsuccessful late ConnectController-only Stage 4F5 logic.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Reads the original ONDA Terminal PE32 through FirmwareVolume2.",
    "Loads and starts a fresh Terminal image after COM1 publication.",
    "Connects all matching drivers recursively to the COM1 controller.",
    "Leaves all reload failures nonfatal after printing exact EFI statuses.",
    "Stage 4F4 boot-transition diagnostics enumerate resulting text children.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Diagnostic-only image; it does not add serial ConOut or ConIn paths.",
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
