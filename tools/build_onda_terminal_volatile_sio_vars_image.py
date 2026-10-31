from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fj"
    / "nomadix_onda_terminal_volatile_sio_vars_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fj_onda-terminal-volatile-sio-vars.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FJ"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FJ based on hardware-tested Stage 4F4.",
    "Retains Stage 4FI direct ONDA binding and private I/O backend.",
    "Creates only volatile current-boot AMI SIO mapping variables.",
    "Sets SerialPortsEnabledVar to one enabled port.",
    "Sets SioSerialPortsLocationVar to PNP0501 UID0 mapped as SIO.",
    "Uses EfiSioVariableGuid exactly as ONDA Terminal helper RVA 0x0478.",
    "Does not set EFI_VARIABLE_NON_VOLATILE and does not write flash NVRAM.",
    "Allows ONDA preflight to map ACPI PNP0501 UID0 to serial index zero.",
    "Uses only the firmware-resident ONDA Terminal driver.",
    "Does not load, patch, replace, or embed any Terminal PE.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
