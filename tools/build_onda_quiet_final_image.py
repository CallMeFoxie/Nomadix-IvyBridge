from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gg_restore-ac-power-policy.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gh"
    / "nomadix_quiet_final_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gh_quiet-final.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "4D610CC28896823938408D97370F6F5621F2658FBF6B3C901FEA2C0738FD988D"
)
builder.STAGE_NAME = "4GH"
builder.DESCRIPTION_LINES = [
    "Quiet final Stage 4GH based on hardware-tested Stage 4GG.",
    "Retains working Restore AC Power Loss for both Power On and Power Off.",
    "Retains persistent fan profiles, FOXNODIX SMBIOS, and working PLCM.",
    "Suppresses all NOMADIX boot, input, NVRAM, HII, SMBIOS, PLCM, and",
    "AC-power-policy diagnostics while preserving serial Setup and AMI output.",
]


if __name__ == "__main__":
    builder.main()
