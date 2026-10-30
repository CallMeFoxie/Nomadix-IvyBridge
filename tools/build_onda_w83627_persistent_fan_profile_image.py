from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fz_w83627-fan-profiles.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4ga"
    / "nomadix_w83627_persistent_fan_profile_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4ga_w83627-persistent-fan-profile.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "7B45973B16E12CA7872448497E0F14741F1227F5753D9F8DBE82C1CAC584AC4B"
)
builder.STAGE_NAME = "4GA"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GA based on hardware-tested Stage 4FZ.",
    "Retains the Quiet, Balanced, and Full Speed HII actions.",
    "Stores the selected profile in a validated nonvolatile UEFI variable.",
    "Loads and applies the profile after Stage 4F2 initializes the W83627.",
    "Defaults to Full Speed for missing, corrupt, or unreadable variables.",
    "Verifies all four PWM writes and falls back to 255 on any mismatch.",
    "Leaves continuous automatic fan regulation to OpenWrt.",
]


if __name__ == "__main__":
    builder.main()
