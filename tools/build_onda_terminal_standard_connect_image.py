from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4ff"
    / "nomadix_onda_terminal_standard_connect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4ff_onda-terminal-standard-connect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FF"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FF based on hardware-tested Stage 4F4.",
    "Abandons all Lanner Terminal payload and LoadImage experiments.",
    "Uses only the firmware-resident ONDA Terminal driver.",
    "Publishes COM1 with an ACPI PNP0501 UID0 path and the tested UART node.",
    "Provides the exact AMI private serial fields used by ONDA Terminal.",
    "Sets $SIO at +0x40 and the direct I/O base 0x3F8 at +0xB8.",
    "Leaves protocol backend, register BAR, width, and MMIO flags zero.",
    "Provides a container through offset 0x12F for ONDA private state.",
    "Requests the standard PC-ANSI terminal vendor child explicitly.",
    "Calls ConnectController at ReadyToBoot with that RemainingDevicePath.",
    "Does not load, patch, replace, or embed any Terminal PE.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
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
