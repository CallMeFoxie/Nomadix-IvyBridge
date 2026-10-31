from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gf_quiet-plcm-lpt.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gg"
    / "nomadix_restore_ac_power_policy_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gg_restore-ac-power-policy.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "719C18282FA122E5310A541BFE8E4936E2B1516365E8E2E63FE0492BF0CAE786"
)
builder.STAGE_NAME = "4GG"
builder.DESCRIPTION_LINES = [
    "Diagnostic Stage 4GG based on Stage 4GF.",
    "Reads Restore AC Power Loss from Setup offset 0x2D3.",
    "Clears GEN_PMCON_3.AFTERG3_EN for Power On.",
    "Sets GEN_PMCON_3.AFTERG3_EN for Power Off.",
    "Applies immediately and again at ReadyToBoot with read-back verification.",
    "Retains the working fan, SMBIOS, serial Setup, LPT, and PLCM behavior.",
]


if __name__ == "__main__":
    builder.main()
