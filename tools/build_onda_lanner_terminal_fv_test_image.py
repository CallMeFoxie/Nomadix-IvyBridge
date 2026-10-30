from __future__ import annotations

import hashlib
import tempfile
from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
INTERMEDIATE = (
    ROOT / "output" / "onda-exact" / "INTERNAL_stage4fd_lanner-terminal.bin"
)
LANNER_DUMP = ROOT / "bios_backup3.bin"
LANNER_TERMINAL_GUID = "7A08CB98-E9BC-41C3-BE19-B302F3F1F595"
LANNER_TERMINAL_SIZE = 0x9E20
LANNER_TERMINAL_SHA256 = (
    "59D496AD71BF69AF554C18461FCA5AD7695158E5E46061A5F0C59D846D2328BC"
)
ORIGINAL_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)

builder.BASE = INTERMEDIATE
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fd"
    / "nomadix_lanner_terminal_fv_test_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fd_lanner-terminal-fv-test.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.STAGE_NAME = "4FD"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FD based on hardware-tested Stage 4F4.",
    "Supersedes unsafe Stage 4FC, whose embedded payload made its DXE too large.",
    "Replaces only the ONDA Terminal PE32 with the smaller Lanner Terminal PE.",
    "Sets the resident Terminal FFS DEPEX to FALSE/END to prevent auto-start.",
    "Uses a small custom loader rather than embedding the Terminal payload.",
    "Retains Stage 4F2 platform initialization and Stage 4F4 Serial I/O.",
    "Validates the exact Lanner Terminal size and SHA-256 during every build.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Changes only the RAM copy of the COM1 device path to ACPI PNP0501/UID0.",
    "Reloads the firmware-resident Lanner Terminal through FV2 ReadSection.",
    "Identifies its new Driver Binding by before/after handle set difference.",
    "Calls Supported and Start with RemainingDevicePath equal to NULL.",
    "Leaves all load, start, and binding failures nonfatal with diagnostics.",
    f"lanner_terminal_pe_sha256={LANNER_TERMINAL_SHA256}",
]


def extract_lanner_terminal(destination: Path) -> bytes:
    builder.run(
        [
            str(builder.UEFI_EXTRACT),
            str(LANNER_DUMP),
            LANNER_TERMINAL_GUID,
            "-o",
            str(destination),
            "-m",
            "body",
            "-t",
            builder.SECTION_PE32,
        ]
    )
    body = destination / "body.bin"
    if not body.is_file():
        raise RuntimeError("UEFIExtract did not extract the Lanner Terminal PE32")
    payload = body.read_bytes()
    if len(payload) != LANNER_TERMINAL_SIZE:
        raise ValueError("Unexpected Lanner Terminal PE32 size")
    if hashlib.sha256(payload).hexdigest().upper() != LANNER_TERMINAL_SHA256:
        raise ValueError("Unexpected Lanner Terminal PE32 hash")
    builder.validate_pe(payload)
    return payload


def replace_terminal(
    source: Path,
    payload: Path,
    false_depex: Path,
    output: Path,
) -> None:
    pe_output = output.with_name(output.name + ".pe")
    if pe_output.exists():
        pe_output.unlink()
    if output.exists():
        output.unlink()
    result = builder.run(
        [
            str(builder.UEFI_REPLACE),
            str(source),
            LANNER_TERMINAL_GUID,
            builder.SECTION_PE32,
            str(payload),
            "-o",
            str(pe_output),
        ]
    )
    if "File replaced" not in result:
        raise RuntimeError(f"UEFIReplace did not replace Terminal:\n{result}")
    result = builder.run(
        [
            str(builder.UEFI_REPLACE),
            str(pe_output),
            LANNER_TERMINAL_GUID,
            builder.SECTION_DXE_DEPEX,
            str(false_depex),
            "-o",
            str(output),
        ]
    )
    pe_output.unlink()
    if "File replaced" not in result:
        raise RuntimeError(f"UEFIReplace did not disable Terminal:\n{result}")


def validate_preserved_regions(original: bytes, result: bytes) -> None:
    if len(result) != len(original):
        raise ValueError("Stage 4FD image size changed")
    if result[: builder.PROTECTED_PREFIX_END] != original[
        : builder.PROTECTED_PREFIX_END
    ]:
        raise ValueError("Descriptor, ME, NVRAM, or erased prefix changed")
    if result[builder.PEI_START :] != original[builder.PEI_START :]:
        raise ValueError("PEI/SEC or microcode region changed")
    original_microcodes = builder.find_microcode_ffs(original)
    result_microcodes = builder.find_microcode_ffs(result)
    if sorted(blob for _, blob in original_microcodes) != sorted(
        blob for _, blob in result_microcodes
    ):
        raise ValueError("A Stage 4F4 microcode FFS changed")


def main() -> None:
    original = ORIGINAL_BASE.read_bytes()
    if (
        len(original) != builder.FLASH_END
        or builder.sha256(original) != ORIGINAL_BASE_SHA256
    ):
        raise ValueError("Original base is not hardware-tested Stage 4F4")

    try:
        with tempfile.TemporaryDirectory(
            prefix="nomadix-stage4fd-terminal-"
        ) as temporary:
            workspace = Path(temporary)
            payload_directory = workspace / "lanner-terminal"
            payload = extract_lanner_terminal(payload_directory)
            payload_path = payload_directory / "body.bin"
            false_depex = workspace / "false-end.depex"
            false_depex.write_bytes(bytes((0x07, 0x08)))
            second_intermediate = workspace / "second-intermediate.bin"
            replace_terminal(
                ORIGINAL_BASE,
                payload_path,
                false_depex,
                INTERMEDIATE,
            )
            replace_terminal(
                ORIGINAL_BASE,
                payload_path,
                false_depex,
                second_intermediate,
            )
            if INTERMEDIATE.read_bytes() != second_intermediate.read_bytes():
                raise ValueError("Terminal PE replacement is not deterministic")

            intermediate = INTERMEDIATE.read_bytes()
            validate_preserved_regions(original, intermediate)
            builder.EXPECTED_BASE_SHA256 = builder.sha256(intermediate)
            builder.main()

            result = builder.OUTPUT.read_bytes()
            validate_preserved_regions(original, result)
            extracted = builder.extract_section(
                builder.OUTPUT,
                builder.SECTION_PE32,
                workspace / "verify-custom-driver",
            )
            if extracted != builder.DRIVER_OUTPUT.read_bytes():
                raise ValueError("Final custom driver extraction mismatch")

            terminal_directory = workspace / "verify-lanner-terminal"
            builder.run(
                [
                    str(builder.UEFI_EXTRACT),
                    str(builder.OUTPUT),
                    LANNER_TERMINAL_GUID,
                    "-o",
                    str(terminal_directory),
                    "-m",
                    "body",
                    "-t",
                    builder.SECTION_PE32,
                ]
            )
            final_terminal = (terminal_directory / "body.bin").read_bytes()
            if final_terminal != payload:
                raise ValueError("Final image does not contain exact Lanner PE")

            depex_directory = workspace / "verify-lanner-depex"
            builder.run(
                [
                    str(builder.UEFI_EXTRACT),
                    str(builder.OUTPUT),
                    LANNER_TERMINAL_GUID,
                    "-o",
                    str(depex_directory),
                    "-m",
                    "body",
                    "-t",
                    builder.SECTION_DXE_DEPEX,
                ]
            )
            if (depex_directory / "body.bin").read_bytes() != bytes((0x07, 0x08)):
                raise ValueError("Final Lanner Terminal DEPEX is not FALSE/END")
    finally:
        if INTERMEDIATE.exists():
            INTERMEDIATE.unlink()


if __name__ == "__main__":
    main()
