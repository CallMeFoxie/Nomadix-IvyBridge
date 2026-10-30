from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fs_serial-ansi-bold.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fy"
    / "nomadix_w83627_hii_formset_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fy_w83627-hii-formset.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "0EB27240360DDEFDAEEB94E87DE2D5CA18D901DDB579E68E9A12F27DAC50EBE6"
)
builder.STAGE_NAME = "4FY"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FY based on Stage 4FS.",
    "Publishes a new standalone Nomadix Hardware Monitor HII formset.",
    "Does not modify the original Setup FFS or any ITE form or string.",
    "Builds standard Forms and UCS-2 Strings packages without VFR tools.",
    "Uses EFI_HII_PLATFORM_SETUP_FORMSET_GUID for Setup enumeration.",
    "Includes W83627 temp1/temp3, fan1-fan4, and PWM1-PWM4 values.",
    "Registers initial live values before publishing the package list.",
    "Refreshes owned HII strings once per second after publication.",
    "Does not write W83627 HWM registers or UEFI variables.",
]


if __name__ == "__main__":
    builder.main()
