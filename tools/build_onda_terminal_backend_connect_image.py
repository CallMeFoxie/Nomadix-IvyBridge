from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fh"
    / "nomadix_onda_terminal_backend_connect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fh_onda-terminal-backend-connect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FH"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FH based on hardware-tested Stage 4F4.",
    "Retains Stage 4FG deferred ONDA Terminal arming.",
    "Uses only the firmware-resident ONDA Terminal driver.",
    "Provides the AMI private PCI I/O backend expected at container +0x108.",
    "Passes through byte accesses to physical COM1 at 0x3F8.",
    "Returns IIR 0xFF only for the private helper's second offset-2 read.",
    "Makes the ONDA private UART detection take its immediate success path.",
    "Prints PRIVATE-IO when ONDA first invokes the private backend.",
    "All later public EFI Serial I/O methods continue using physical COM1.",
    "Does not load, patch, replace, or embed any Terminal PE.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
