from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fy_w83627-hii-formset.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fz"
    / "nomadix_w83627_fan_profiles_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fz_w83627-fan-profiles.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "97DBF3CECCA9FCFCD620DC2ECF63F463DDAAFBB3E1079809C8A012DA15703411"
)
builder.STAGE_NAME = "4FZ"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FZ based on hardware-tested Stage 4FY.",
    "Retains the standalone Nomadix Hardware Monitor HII formset.",
    "Adds volatile Quiet, Balanced, and Full Speed callback actions.",
    "Quiet writes PWM1-PWM4 = 10/255.",
    "Balanced writes PWM1-PWM4 = 96/255.",
    "Full Speed writes PWM1-PWM4 = 255/255.",
    "Verifies every write and falls back to 255 on any mismatch.",
    "Does not persist fan profiles in NVRAM.",
    "Stage 4F2 restores fail-safe Full Speed at every restart.",
]


if __name__ == "__main__":
    builder.main()
