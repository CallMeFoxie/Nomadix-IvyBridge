from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f2_nomadix-platform-dxe.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f3"
    / "nomadix_serial_diag_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f3_serial-diag.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "48CFB5E8F41B6D79288B8E86E75F089E6F29D43CB6B4B75643B572A3D05D07DF"
)
builder.STAGE_NAME = "4F3"
builder.DESCRIPTION_LINES = [
    "Experimental diagnostic Stage 4F3 based on hardware-tested Stage 4F2.",
    "Retains Stage 4F2 microcode 0x21 and LPC/UART/Winbond initialization.",
    "Replaces the same ITEPOWERFAILDXE PE32 section with a cumulative driver.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Writes diagnostics directly to COM1 at 115200 8N1 with no flow control.",
    "Prints an ARMED banner immediately after successful Stage 4F2 init.",
    "Registers both ReadyToBoot and LegacyBoot notifications.",
    "At the first notification, locates every EFI Serial I/O protocol handle.",
    "Prints each matching handle's complete binary EFI device path as hex.",
    "Bounds each device path to 1024 bytes and rejects malformed nodes.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Diagnostic-only image; do not treat as the final console-redirection fix.",
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
