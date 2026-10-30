from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4ga_w83627-persistent-fan-profile.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gb"
    / "nomadix_foxnodix_smbios_branding_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gb_foxnodix-smbios-branding.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "89E91C1D7A0FC5766A9E231C5036547FDC350CB9A1588A30CD5FCF66A09111DA"
)
builder.STAGE_NAME = "4GB"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GB based on hardware-tested Stage 4GA.",
    "Retains the persistent W83627 fan profile implementation.",
    "Updates existing SMBIOS Type 0, 1, 2, and 3 strings through",
    "EFI_SMBIOS_PROTOCOL before BDS.",
    "Uses FOXNODIX, FW-7582, and Nomadix Ivy Edition branding.",
    "Does not patch compressed AMI or ONDA modules.",
    "Reports fields whose original SMBIOS string index is zero.",
]


if __name__ == "__main__":
    builder.main()
