from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4ge_plcm-lpt-enable.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gf"
    / "nomadix_quiet_plcm_lpt_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gf_quiet-plcm-lpt.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "B0F2DADA932F6175EF886E054DBB254FA127F3D4FE13ECE8ACC4D095FC172190"
)
builder.STAGE_NAME = "4GF"
builder.DESCRIPTION_LINES = [
    "Quiet Stage 4GF based on hardware-tested Stage 4GE.",
    "Retains the working PCH and W83627 LPT initialization.",
    "Retains HELLO THERE / FOXNODIX on the front display.",
    "Suppresses NOMADIX boot, input, HII, NVRAM, SMBIOS, and PLCM diagnostics.",
    "Keeps AMI POST, serial Setup, ConOut, and ConIn fully operational.",
]


if __name__ == "__main__":
    builder.main()
