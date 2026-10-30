from pathlib import Path

import build_onda_w83627_hii_monitor_image as stage4ft


ROOT = Path(__file__).resolve().parents[1]

stage4ft.builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fw"
    / "nomadix_w83627_legacy_hii_dxe.c"
)
stage4ft.builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fw_w83627-legacy-hii.bin"
)
stage4ft.builder.DRIVER_OUTPUT = stage4ft.builder.OUTPUT.with_suffix(".efi")
stage4ft.builder.STAGE_NAME = "4FW"
stage4ft.builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FW based on Stage 4FS.",
    "Updates both standard UEFI HII and Framework EFI_HII_PROTOCOL strings.",
    "Uses legacy GUID D7AD636E-B997-459B-BF3F-8846897980E1.",
    "Finds the 16-bit legacy handle by the IT8772E_B title token.",
    "Uses the package ISO-639 primary language for NewString updates.",
    "Reads the updated title back through the legacy GetString function.",
    "Does not write W83627 HWM registers or UEFI variables.",
]


if __name__ == "__main__":
    stage4ft.builder.main()
