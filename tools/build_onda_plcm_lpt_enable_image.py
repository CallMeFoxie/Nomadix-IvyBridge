from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gc_plcm-boot-message.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4ge"
    / "nomadix_plcm_lpt_enable_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4ge_plcm-lpt-enable.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "8A922584DE1167883AD3ADBE17EFF0BD7107040BF05CE44688A989F993523771"
)
builder.STAGE_NAME = "4GE"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GE based on hardware-tested Stage 4GC.",
    "Enables PCH LPC LPT decoding before accessing the front display.",
    "Activates W83627 logical device 1 at 0x378 with IRQ 7.",
    "Verifies the PCH and Super I/O configuration by read-back.",
    "Then initializes the PLCM and writes HELLO THERE / FOXNODIX.",
]


if __name__ == "__main__":
    builder.main()
