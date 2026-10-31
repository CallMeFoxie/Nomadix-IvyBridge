from pathlib import Path

import build_onda_w83627_hii_monitor_image as stage4ft


ROOT = Path(__file__).resolve().parents[1]

stage4ft.builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fu"
    / "nomadix_w83627_hii_diagnostic_dxe.c"
)
stage4ft.builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fu_w83627-hii-diagnostic.bin"
)
stage4ft.builder.DRIVER_OUTPUT = stage4ft.builder.OUTPUT.with_suffix(".efi")
stage4ft.builder.STAGE_NAME = "4FU"
stage4ft.builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FU based on Stage 4FS.",
    "Retains the Stage 4FT read-only W83627DHG-P HII monitor.",
    "Enumerates every language reported by each HII package list.",
    "Does not assume that the AMI Setup package uses en-US.",
    "Emits one-shot HII protocol, package-list, and matched-language "
    "diagnostics.",
    "Applies the same read-only Setup IFR safety patch as Stage 4FT.",
    "Does not write W83627 HWM registers or UEFI variables.",
]


if __name__ == "__main__":
    stage4ft.builder.main()
