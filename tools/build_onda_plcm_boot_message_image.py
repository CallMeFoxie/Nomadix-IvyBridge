from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gb_foxnodix-smbios-branding.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gc"
    / "nomadix_plcm_boot_message_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gc_plcm-boot-message.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "7EBAC40B21FFCD1E312A4B87913C3124BA708178485277F668C5D9570B8E6B2A"
)
builder.STAGE_NAME = "4GC"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GC based on hardware-tested Stage 4GB.",
    "Retains FOXNODIX SMBIOS branding and persistent fan profiles.",
    "Uses the Lanner plcm_drv parallel-port protocol before boot.",
    "Probes 0x378, 0x278, and 0x3BC in the same order as plcm_drv.",
    "Initializes the two-line display and writes HELLO THERE / FOXNODIX.",
    "OpenWrt plcm_drv may take control and replace the message later.",
]


if __name__ == "__main__":
    builder.main()
