from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f3_serial-diag.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f4"
    / "nomadix_serial_io_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "66C23FE1F5062843A6E2106C031AAE7F61AE24CCAD2FED2F3AE0713C1A56C4B6"
)
builder.STAGE_NAME = "4F4"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4F4 based on hardware-tested diagnostic Stage 4F3.",
    "Retains Stage 4F2 microcode 0x21 and platform initialization.",
    "Retains the Stage 4F3 direct-COM1 boot diagnostics.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Publishes one EFI Serial I/O protocol handle for physical COM1.",
    "Installs a unique vendor controller path followed by a UART node.",
    "Implements 16550 reset, attributes, control, status, read, and write.",
    "Defaults to 115200 8N1 with no flow control.",
    "The legacy-boot diagnostic must change from EFI_NOT_FOUND/0 to success/1.",
    "Also enumerates Simple Text Output paths to detect AMI Terminal binding.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Diagnostic-only image; it does not yet add serial ConOut or ConIn paths.",
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
