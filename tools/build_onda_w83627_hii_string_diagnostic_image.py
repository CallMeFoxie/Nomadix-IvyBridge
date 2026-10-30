from pathlib import Path

import build_onda_w83627_hii_monitor_image as stage4ft


ROOT = Path(__file__).resolve().parents[1]

stage4ft.builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fv"
    / "nomadix_w83627_hii_string_diagnostic_dxe.c"
)
stage4ft.builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fv_w83627-hii-string-diagnostic.bin"
)
stage4ft.builder.DRIVER_OUTPUT = stage4ft.builder.OUTPUT.with_suffix(".efi")
stage4ft.builder.STAGE_NAME = "4FV"
stage4ft.builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FV based on Stage 4FS.",
    "Retains the language-independent Stage 4FT W83627 HII monitor.",
    "Reports SetString status for static labels and sensor values.",
    "Reads the updated title token back from the same HII package handle.",
    "Distinguishes SetString failure from AMI TSE legacy string caching.",
    "Applies the same read-only Setup IFR safety patch as Stage 4FT.",
    "Does not write W83627 HWM registers or UEFI variables.",
]


if __name__ == "__main__":
    stage4ft.builder.main()
