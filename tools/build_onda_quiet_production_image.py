from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gc_plcm-boot-message.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gd"
    / "nomadix_quiet_production_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gd_quiet-production.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "8A922584DE1167883AD3ADBE17EFF0BD7107040BF05CE44688A989F993523771"
)
builder.STAGE_NAME = "4GD"
builder.DESCRIPTION_LINES = [
    "Experimental quiet Stage 4GD based on Stage 4GC.",
    "Retains serial Setup, fan profiles, SMBIOS branding, and PLCM output.",
    "Suppresses direct NOMADIX diagnostics and cumulative stage markers.",
    "Keeps the serial ConOut and ConIn protocols fully operational.",
    "AMI POST, Setup, and boot-loader output remain visible.",
]


if __name__ == "__main__":
    builder.main()
