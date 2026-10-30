from pathlib import Path

import build_onda_w83627_hii_monitor_image as stage4ft


ROOT = Path(__file__).resolve().parents[1]

stage4ft.builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fx"
    / "nomadix_w83627_pre_tse_hii_dxe.c"
)
stage4ft.builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fx_w83627-pre-tse-hii.bin"
)
stage4ft.builder.DRIVER_OUTPUT = stage4ft.builder.OUTPUT.with_suffix(".efi")
stage4ft.builder.STAGE_NAME = "4FX"
stage4ft.builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FX based on Stage 4FS.",
    "Updates standard HII immediately and polls at 10 ms until Setup appears.",
    "Returns to a one-second sensor refresh after the first string update.",
    "Attempts the unavailable Framework HII protocol only once.",
    "Prevents repeated EFI_NOT_FOUND diagnostics from scrolling the screen.",
    "Targets HII strings before AMI TSE creates its private display cache.",
    "Does not write W83627 HWM registers or UEFI variables.",
]


if __name__ == "__main__":
    stage4ft.builder.main()
