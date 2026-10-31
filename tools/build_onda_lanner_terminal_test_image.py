from __future__ import annotations

import hashlib
import os
from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
LANNER_DUMP = ROOT / "bios_backup3.bin"
LANNER_TERMINAL_GUID = "7A08CB98-E9BC-41C3-BE19-B302F3F1F595"
LANNER_TERMINAL_SIZE = 0x9E20
LANNER_TERMINAL_SHA256 = (
    "59D496AD71BF69AF554C18461FCA5AD7695158E5E46061A5F0C59D846D2328BC"
)

builder.BASE = (
    ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fc"
    / "nomadix_lanner_terminal_test_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fc_lanner-terminal-test.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)
builder.STAGE_NAME = "4FC"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FC based on hardware-tested Stage 4F4.",
    "Retains Stage 4F2 platform initialization and Stage 4F4 Serial I/O.",
    "Embeds the exact Lanner FW-7582 Terminal PE from bios_backup3.bin.",
    "Validates the embedded payload size and SHA-256 during every build.",
    "Does not replace either firmware-resident Terminal FFS.",
    "Does not read, create, modify, or delete UEFI variables.",
    "Changes only the RAM copy of the custom COM1 device path to ACPI PNP0501.",
    "Uses UID zero and preserves the tested UART node at 115200 8N1.",
    "Loads and starts the embedded Lanner Terminal from a memory buffer.",
    "Identifies its Driver Binding by a before/after handle set difference.",
    "Calls Supported and Start with RemainingDevicePath equal to NULL.",
    "Leaves all load, start, and binding failures nonfatal with diagnostics.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "Diagnostic-only image; it does not persist serial ConOut or ConIn paths.",
    f"lanner_terminal_pe_sha256={LANNER_TERMINAL_SHA256}",
    "included_stage4f4_source_sha256="
    + builder.sha256(
        (
            ROOT
            / "firmware"
            / "stage4f4"
            / "nomadix_serial_io_dxe.c"
        ).read_bytes()
    ),
]


def extract_lanner_terminal(workspace: Path) -> bytes:
    destination = workspace / "lanner-terminal"
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
    digest = hashlib.sha256(payload).hexdigest().upper()
    if digest != LANNER_TERMINAL_SHA256:
        raise ValueError("Unexpected Lanner Terminal PE32 hash")
    builder.validate_pe(payload)
    return payload


def generate_payload_source(payload: bytes, destination: Path) -> None:
    rows = []
    for offset in range(0, len(payload), 16):
        chunk = payload[offset : offset + 16]
        rows.append("    " + ", ".join(f"0x{value:02X}" for value in chunk))
    destination.write_text(
        "#include <stdint.h>\n"
        "__declspec(align(8)) const uint8_t mLannerTerminalImage[] = {\n"
        + ",\n".join(rows)
        + "\n};\n"
        f"const uint64_t mLannerTerminalImageSize = 0x{len(payload):X}ULL;\n",
        encoding="ascii",
    )


def compile_driver(workspace: Path) -> bytes:
    workspace.mkdir(parents=True, exist_ok=True)
    payload = extract_lanner_terminal(workspace)
    payload_source = workspace / "lanner_terminal_payload.c"
    generate_payload_source(payload, payload_source)

    driver_object = workspace / "nomadix_lanner_terminal_test_dxe.obj"
    payload_object = workspace / "lanner_terminal_payload.obj"
    efi = workspace / "nomadix_lanner_terminal_test_dxe.efi"
    build_script = workspace / "build.cmd"
    build_script.write_text(
        "@echo off\r\n"
        f'call "{builder.VCVARS}" >nul || exit /b 1\r\n'
        "cl /nologo /c /O1 /Os /GS- /Gs9999999 /GR- /Zl /W4 /WX "
        f'/Fo"{driver_object}" "{builder.SOURCE}" || exit /b 1\r\n'
        "cl /nologo /c /O1 /Os /GS- /Gs9999999 /GR- /Zl /W4 /WX "
        f'/Fo"{payload_object}" "{payload_source}" || exit /b 1\r\n'
        "link /nologo /dll /entry:EfiMain "
        "/subsystem:efi_boot_service_driver /nodefaultlib /machine:x64 "
        "/fixed:no /dynamicbase:no /Brepro /opt:ref /opt:icf "
        f'/out:"{efi}" "{driver_object}" "{payload_object}" || exit /b 1\r\n',
        encoding="ascii",
        newline="",
    )
    builder.run([os.environ["COMSPEC"], "/d", "/c", str(build_script)])
    if not efi.is_file():
        raise RuntimeError("MSVC linker did not create the EFI driver")
    data = efi.read_bytes()
    builder.validate_pe(data)
    if payload not in data:
        raise ValueError("Compiled driver does not contain the exact payload")
    return data


builder.compile_driver = compile_driver


if __name__ == "__main__":
    if not LANNER_DUMP.is_file():
        raise FileNotFoundError(LANNER_DUMP)
    builder.main()
