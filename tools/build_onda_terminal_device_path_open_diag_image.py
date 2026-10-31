from pathlib import Path

import build_onda_platform_dxe_image as builder
import build_onda_terminal_private_bypass_image as stage4f8


ROOT = Path(__file__).resolve().parents[1]

stage4f8.validate_terminal_patch_target()

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f9_terminal-serial-call-trace.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fa"
    / "nomadix_terminal_device_path_open_diag_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fa_terminal-device-path-open-diag.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "52910FE9A16B000DD8CCC574CC48547342F70073CBA567D018FF8974C614619F"
)
builder.STAGE_NAME = "4FA"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FA based on hardware-tested Stage 4F9.",
    "Retains all Stage 4F2 through Stage 4F9 behavior and diagnostics.",
    "Prints an unconditional Stage 4FA ARMED marker.",
    "Does not change the RAM-only guarded ONDA Terminal patch.",
    "Does not bypass another ONDA Terminal Start check.",
    "After patched Start fails, queries DevicePath open information.",
    "Prints every DevicePath agent, controller, attribute, and open count.",
    "Repeats the exact AMI DevicePath BY_DRIVER open with the same handles.",
    "Prints the original retry EFI status and returned interface pointer.",
    "Closes only a newly successful diagnostic reopen.",
    "Does not read, create, modify, or delete any UEFI variable.",
    "Diagnostic-only image; it does not add serial ConOut or ConIn paths.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "onda_terminal_pe_sha256=" + stage4f8.EXPECTED_TERMINAL_SHA256,
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
