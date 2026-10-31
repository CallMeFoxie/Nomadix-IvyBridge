from pathlib import Path

import build_onda_platform_dxe_image as builder
import build_onda_terminal_private_bypass_image as stage4f8


ROOT = Path(__file__).resolve().parents[1]

stage4f8.validate_terminal_patch_target()

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fa_terminal-device-path-open-diag.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fb"
    / "nomadix_terminal_preflight_status_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fb_terminal-preflight-status.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "98B9E114C012876EF6FFA3A67B5939A683965557828A0ACE433A260BDBE3C321"
)
builder.STAGE_NAME = "4FB"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FB based on hardware-tested Stage 4FA.",
    "Retains all Stage 4F2 through Stage 4FA behavior and diagnostics.",
    "Prints an unconditional Stage 4FB ARMED marker.",
    "Retains the guarded RAM-only ONDA Terminal $SIO branch patch.",
    "Validates the exact original first Start preflight JNE bytes.",
    "Redirects only that error branch from generic cleanup to the epilogue.",
    "Preserves the original EFI_STATUS returned by helper RVA 0x2010.",
    "Does not bypass a successful or failed preflight result.",
    "At this first branch no child allocation or protocol open has occurred.",
    "Does not change firmware Terminal FFS, Serial I/O, or UEFI variables.",
    "Diagnostic-only image; it does not add serial ConOut or ConIn paths.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "onda_terminal_pe_sha256=" + stage4f8.EXPECTED_TERMINAL_SHA256,
    "included_stage4fa_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4fa"
            / "nomadix_terminal_device_path_open_diag_dxe.c"
        ).read_bytes()
    ),
    "included_stage4f9_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f9"
            / "nomadix_terminal_serial_call_trace_dxe.c"
        ).read_bytes()
    ),
    "included_stage4f8_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f8"
            / "nomadix_terminal_private_bypass_dxe.c"
        ).read_bytes()
    ),
]


if __name__ == "__main__":
    builder.main()
