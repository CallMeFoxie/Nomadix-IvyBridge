from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f5"
    / "nomadix_terminal_connect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f5_terminal-connect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4F5"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4F5 based on hardware-tested Stage 4F4.",
    "Retains Stage 4F2 platform init and Stage 4F3/4F4 diagnostics.",
    "Retains the working Stage 4F4 COM1 EFI Serial I/O protocol publisher.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Calls ConnectController recursively on the COM1 handle at boot transition.",
    "Allows a second attempt if the first event cannot connect a driver.",
    "Enumerates post-connect Simple Text Output handles and device paths.",
    "Waits briefly for buffered AMI serial output before its diagnostic block.",
    "Defaults to COM1 at 115200 8N1 with no flow control.",
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
