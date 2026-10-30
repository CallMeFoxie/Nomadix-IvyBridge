import hashlib
from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
TERMINAL_PE = (
    ROOT
    / "ref bios"
    / "ONDA H61ITX01.BIN.dump"
    / "2 BIOS region"
    / "2 8C8CE578-8A3D-4F1C-9935-896185C32DD3"
    / "0 FV_MAIN_NESTED"
    / "0 Compressed section"
    / "0 Volume image section"
    / "0 8C8CE578-8A3D-4F1C-9935-896185C32DD3"
    / "123 Terminal"
    / "0 Compressed section"
    / "0 PE32 image section"
    / "body.bin"
)
EXPECTED_TERMINAL_SHA256 = (
    "31453921E085B6115053A86AD24D6F6A15108F97E66A4DEA432BEB04EFF0642D"
)
PRIVATE_CHECK_OFFSET = 0x2D28
EXPECTED_PRIVATE_CHECK = bytes.fromhex("84 C0 0F 85 8E 05 00 00")

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f7_terminal-direct-start.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f8"
    / "nomadix_terminal_private_bypass_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f8_terminal-private-bypass.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "8E7EEE93AC7C895ED61C1D3D99C3CD4EC1AF4A4995209926881735EB4B164ABA"
)
builder.STAGE_NAME = "4F8"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4F8 based on hardware-tested Stage 4F7.",
    "Retains all Stage 4F2 through Stage 4F7 diagnostics.",
    "Does not modify the firmware-resident ONDA Terminal FFS or any NVRAM.",
    "Loads a second private RAM copy of the exact ONDA Terminal PE32.",
    "Validates PE size and exact original private-check instruction bytes.",
    "NOPs only the conditional rejection after the AMI private $SIO helper.",
    "Leaves all standard Supported, SetAttributes, SetControl, and child logic.",
    "Identifies the newly installed Driver Binding handle by set difference.",
    "Calls Supported and Start only on the newly patched Terminal binding.",
    "Leaves all patch/reload/start failures nonfatal with exact diagnostics.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Diagnostic-only image; it does not add serial ConOut or ConIn paths.",
    f"onda_terminal_pe_sha256={EXPECTED_TERMINAL_SHA256}",
    "included_stage4f7_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f7"
            / "nomadix_terminal_direct_start_dxe.c"
        ).read_bytes()
    ),
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
]


def validate_terminal_patch_target() -> None:
    data = TERMINAL_PE.read_bytes()
    digest = hashlib.sha256(data).hexdigest().upper()
    if digest != EXPECTED_TERMINAL_SHA256:
        raise ValueError("Unexpected ONDA Terminal PE32 hash")
    if len(data) != 0xC440:
        raise ValueError("Unexpected ONDA Terminal PE32 size")
    actual = data[
        PRIVATE_CHECK_OFFSET : PRIVATE_CHECK_OFFSET + len(EXPECTED_PRIVATE_CHECK)
    ]
    if actual != EXPECTED_PRIVATE_CHECK:
        raise ValueError("Unexpected ONDA Terminal private-check bytes")


if __name__ == "__main__":
    validate_terminal_patch_target()
    builder.main()
