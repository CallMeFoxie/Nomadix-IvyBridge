from pathlib import Path

import build_onda_platform_dxe_image as builder
import build_onda_terminal_private_bypass_image as stage4f8


ROOT = Path(__file__).resolve().parents[1]

stage4f8.validate_terminal_patch_target()

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f8_terminal-private-bypass.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4f9"
    / "nomadix_terminal_serial_call_trace_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f9_terminal-serial-call-trace.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "09413C91662367D5E5FB8E488FC0AFE00DF9F4206EAA06ED0CE6354AA5014BB0"
)
builder.STAGE_NAME = "4F9"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4F9 based on hardware-tested Stage 4F8.",
    "Retains all Stage 4F2 through Stage 4F8 behavior and diagnostics.",
    "Does not change the RAM-only guarded ONDA Terminal patch.",
    "Does not change any Serial I/O operation or returned EFI status.",
    "Logs every Terminal SetAttributes argument and exact result.",
    "Logs every Terminal SetControl argument and exact result.",
    "Distinguishes the two failures AMI collapses to EFI_DEVICE_ERROR.",
    "Does not read, create, modify, or delete any UEFI variable.",
    "Diagnostic-only image; it does not add serial ConOut or ConIn paths.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "onda_terminal_pe_sha256=" + stage4f8.EXPECTED_TERMINAL_SHA256,
    "included_stage4f8_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f8"
            / "nomadix_terminal_private_bypass_dxe.c"
        ).read_bytes()
    ),
    "included_stage4f7_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f7"
            / "nomadix_terminal_direct_start_dxe.c"
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


if __name__ == "__main__":
    builder.main()
