from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fk"
    / "nomadix_onda_terminal_variable_shim_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fk_onda-terminal-variable-shim.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FK"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FK based on hardware-tested Stage 4F4.",
    "Retains Stage 4FI direct ONDA binding and private I/O backend.",
    "Does not create, delete, or modify any UEFI variable.",
    "Temporarily hooks RuntimeServices GetVariable only around Terminal Start.",
    "Returns one enabled port for SerialPortsEnabledVar.",
    "Returns PNP0501 UID0 mapped as SIO for SioSerialPortsLocationVar.",
    "Passes every other GetVariable request to the original service.",
    "Restores the exact original GetVariable pointer after Start returns.",
    "Allows ONDA preflight to map ACPI PNP0501 UID0 to serial index zero.",
    "Uses only the firmware-resident ONDA Terminal driver.",
    "Does not load, patch, replace, or embed any Terminal PE.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]


if __name__ == "__main__":
    builder.main()
