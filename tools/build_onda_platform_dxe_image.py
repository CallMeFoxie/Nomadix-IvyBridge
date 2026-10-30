from __future__ import annotations

import hashlib
import os
import struct
import subprocess
import tempfile
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f1_intel-306a9-microcode-21.bin"
)
SOURCE = ROOT / "firmware" / "stage4f2" / "nomadix_platform_dxe.c"
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4f2_nomadix-platform-dxe.bin"
)
DRIVER_OUTPUT = OUTPUT.with_suffix(".efi")
UEFI_EXTRACT = ROOT / "tools" / "UEFIExtract_NE_A76_win64" / "UEFIExtract.exe"
UEFI_REPLACE = ROOT / "tools" / "UEFIReplace.exe"
VCVARS = Path(
    r"C:\Program Files (x86)\Microsoft Visual Studio"
    r"\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

TARGET_GUID = "FC58EFE0-90FC-424B-8628-1AD4002CDAF2"
SECTION_PE32 = "10"
SECTION_DXE_DEPEX = "13"
EXPECTED_DEPEX = bytes((0x06, 0x08))
EXPECTED_BASE_SHA256 = (
    "5C93FBDA6B989EBEB76F9A42D6C2B0B040CF4146C9038767B18E4B5492A39C76"
)
MICROCODE_GUID = uuid.UUID("17088572-377F-44EF-8F4E-B09FFF46A070")
MICROCODE_FFS_SIZE = 0x16018
PROTECTED_PREFIX_END = 0x580000
PEI_START = 0x760000
FLASH_END = 0x800000
STAGE_NAME = "4F2"
DESCRIPTION_LINES = [
    "Experimental Stage 4F2 based on hardware-tested Stage 4F1.",
    "Retains the hardware-verified Intel 306A9 microcode revision 0x21.",
    "Hardware verified: OpenWrt boots, w83627ehf binds at 0x290 without "
    "the OS initializer, PWM starts at 0xFF, and the serial console works.",
    "Replaces only the ITEPOWERFAILDXE PE32 section with a custom x86-64 "
    "Nomadix platform initializer.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
    "The replaced ONDA driver uses the ITE 87/01/55/55 unlock sequence and "
    "F2/F4 registers; it is ineffective on the confirmed Winbond B0/73.",
    "Enables PCH COMA/COMB decode without clearing existing LPC bits.",
    "Uses free GEN2_DEC for 0x290-0x29F while preserving ONDA GEN1_DEC.",
    "Requires PCH 8086:1C5C and Winbond ID/revision B0/73.",
    "Activates Winbond HWM LDN 0B at 0x290 and initializes monitoring.",
    "Sets all four PWM duty registers to fail-safe 0xFF.",
    "Performs read-back verification and returns an EFI error on mismatch.",
]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def run(command: list[str]) -> str:
    process = subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    output = "\n".join(
        part.strip() for part in (process.stdout, process.stderr) if part.strip()
    )
    if process.returncode:
        raise RuntimeError(
            f"Command failed with exit code {process.returncode}: "
            f"{subprocess.list2cmdline(command)}\n{output}"
        )
    return output


def compile_driver(workspace: Path) -> bytes:
    workspace.mkdir(parents=True, exist_ok=True)
    obj = workspace / "nomadix_platform_dxe.obj"
    efi = workspace / "nomadix_platform_dxe.efi"
    build_script = workspace / "build.cmd"
    build_script.write_text(
        "@echo off\r\n"
        f'call "{VCVARS}" >nul || exit /b 1\r\n'
        "cl /nologo /c /O1 /Os /GS- /Gs9999999 /GR- /Zl /W4 /WX "
        f'/Fo"{obj}" "{SOURCE}" || exit /b 1\r\n'
        "link /nologo /dll /entry:EfiMain "
        "/subsystem:efi_boot_service_driver /nodefaultlib /machine:x64 "
        "/fixed:no /dynamicbase:no /Brepro /opt:ref /opt:icf "
        f'/out:"{efi}" "{obj}" || exit /b 1\r\n',
        encoding="ascii",
        newline="",
    )
    run([os.environ["COMSPEC"], "/d", "/c", str(build_script)])
    if not efi.is_file():
        raise RuntimeError("MSVC linker did not create the EFI driver")
    data = efi.read_bytes()
    validate_pe(data)
    return data


def validate_pe(data: bytes) -> None:
    if len(data) < 0x200 or data[:2] != b"MZ":
        raise ValueError("Compiled driver is not a DOS/PE image")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("Compiled driver has no PE signature")
    coff_header = struct.unpack_from("<HHIIIHH", data, pe_offset + 4)
    machine = coff_header[0]
    section_count = coff_header[1]
    optional_size = coff_header[5]
    characteristics = coff_header[6]
    optional = pe_offset + 24
    magic = struct.unpack_from("<H", data, optional)[0]
    subsystem = struct.unpack_from("<H", data, optional + 68)[0]
    import_directory = struct.unpack_from("<II", data, optional + 120)
    relocation_directory = struct.unpack_from("<II", data, optional + 152)
    if machine != 0x8664 or magic != 0x020B:
        raise ValueError("Compiled driver is not x86-64 PE32+")
    if subsystem != 0x000B:
        raise ValueError("Compiled driver is not an EFI boot-service driver")
    if characteristics & 0x2002 != 0x2002:
        raise ValueError("Compiled driver is not an executable DLL")
    if import_directory != (0, 0):
        raise ValueError("Compiled driver unexpectedly imports external symbols")
    if relocation_directory[1] == 0:
        raise ValueError("Compiled driver has no base relocations")

    section_offset = optional + optional_size
    sections: set[str] = set()
    for _ in range(section_count):
        name = (
            data[section_offset : section_offset + 8]
            .rstrip(b"\0")
            .decode("ascii")
        )
        sections.add(name)
        section_offset += 40
    if ".text" not in sections or ".reloc" not in sections:
        raise ValueError("Compiled driver lacks code or relocation sections")


def find_microcode_ffs(image: bytes) -> list[tuple[int, bytes]]:
    files: list[tuple[int, bytes]] = []
    cursor = 0
    while True:
        offset = image.find(MICROCODE_GUID.bytes_le, cursor)
        if offset < 0:
            break
        cursor = offset + 1
        if offset + 24 > len(image):
            continue
        size = int.from_bytes(image[offset + 20 : offset + 23], "little")
        if (
            size != MICROCODE_FFS_SIZE
            or image[offset + 18] != 0x01
            or image[offset + 19] != 0x48
            or image[offset + 23] != 0xF8
        ):
            continue
        blob = image[offset : offset + size]
        if len(blob) != size:
            raise ValueError("Truncated microcode FFS")
        header = bytearray(blob[:24])
        header[17] = 0
        header[23] = 0
        if sum(header) & 0xFF:
            raise ValueError("Invalid microcode FFS header checksum")
        if (sum(blob[24:]) + blob[17]) & 0xFF:
            raise ValueError("Invalid microcode FFS data checksum")
        files.append((offset, blob))
    if len(files) != 2:
        raise ValueError(f"Expected two microcode FFS files, found {len(files)}")
    return files


def extract_section(
    image: Path,
    section_type: str,
    destination: Path,
) -> bytes:
    run(
        [
            str(UEFI_EXTRACT),
            str(image),
            TARGET_GUID,
            "-o",
            str(destination),
            "-m",
            "body",
            "-t",
            section_type,
        ]
    )
    body = destination / "body.bin"
    if not body.is_file():
        raise RuntimeError(
            f"UEFIExtract did not extract section {section_type} from {TARGET_GUID}"
        )
    return body.read_bytes()


def replace_pe32(driver: Path) -> None:
    output = run(
        [
            str(UEFI_REPLACE),
            str(BASE),
            TARGET_GUID,
            SECTION_PE32,
            str(driver),
            "-o",
            str(OUTPUT),
        ]
    )
    if "File replaced" not in output:
        raise RuntimeError(f"UEFIReplace did not confirm replacement:\n{output}")


def generate_report() -> Path:
    run([str(UEFI_EXTRACT), str(OUTPUT), "report"])
    report = OUTPUT.with_suffix(OUTPUT.suffix + ".report.txt")
    if not report.is_file():
        raise RuntimeError(f"UEFIExtract did not create a Stage {STAGE_NAME} report")
    return report


def main() -> None:
    for required in (BASE, SOURCE, UEFI_EXTRACT, UEFI_REPLACE, VCVARS):
        if not required.is_file():
            raise FileNotFoundError(required)

    base = BASE.read_bytes()
    if len(base) != FLASH_END or sha256(base) != EXPECTED_BASE_SHA256:
        raise ValueError("Base is not the hardware-tested Stage 4F1 image")

    with tempfile.TemporaryDirectory(
        prefix=f"nomadix-stage{STAGE_NAME.lower()}-"
    ) as temporary:
        workspace = Path(temporary)
        driver = compile_driver(workspace)
        second_driver = compile_driver(workspace / "determinism")
        if driver != second_driver:
            raise ValueError("EFI driver compilation is not deterministic")
        DRIVER_OUTPUT.write_bytes(driver)

        original_depex = extract_section(
            BASE, SECTION_DXE_DEPEX, workspace / "original-depex"
        )
        if original_depex != EXPECTED_DEPEX:
            raise ValueError(
                f"Unexpected ITEPOWERFAILDXE DEPEX: {original_depex.hex()}"
            )

        replace_pe32(DRIVER_OUTPUT)
        result = OUTPUT.read_bytes()
        if len(result) != len(base):
            raise ValueError(f"Stage {STAGE_NAME} image size changed")
        if result[:PROTECTED_PREFIX_END] != base[:PROTECTED_PREFIX_END]:
            raise ValueError("Descriptor, ME, NVRAM, or erased prefix changed")
        if result[PEI_START:] != base[PEI_START:]:
            raise ValueError("PEI/SEC or microcode region changed")
        base_microcodes = find_microcode_ffs(base)
        result_microcodes = find_microcode_ffs(result)
        if sorted(blob for _, blob in base_microcodes) != sorted(
            blob for _, blob in result_microcodes
        ):
            raise ValueError("A base-image microcode FFS changed")

        extracted_driver = extract_section(
            OUTPUT, SECTION_PE32, workspace / "verify-driver"
        )
        if extracted_driver != driver:
            raise ValueError(
                f"Stage {STAGE_NAME} PE32 section does not match compiled driver"
            )
        output_depex = extract_section(
            OUTPUT, SECTION_DXE_DEPEX, workspace / "verify-depex"
        )
        if output_depex != EXPECTED_DEPEX:
            raise ValueError(
                f"Stage {STAGE_NAME} changed the original TRUE/END DEPEX"
            )

    report = generate_report()
    report_text = report.read_text(errors="replace")
    if TARGET_GUID not in report_text or "ITEPOWERFAILDXE" not in report_text:
        raise ValueError("UEFIExtract report does not contain the target FFS")

    result = OUTPUT.read_bytes()
    metadata = DESCRIPTION_LINES + [
        f"base_sha256={sha256(base)}",
        f"source_sha256={sha256(SOURCE.read_bytes())}",
        f"driver_sha256={sha256(DRIVER_OUTPUT.read_bytes())}",
        f"image_sha256={sha256(result)}",
        f"bootstack_sha256={sha256(result[0x420000:FLASH_END])}",
        "base_microcode_ffs_offsets="
        + ",".join(f"0x{offset:06X}" for offset, _ in find_microcode_ffs(base)),
        "image_microcode_ffs_offsets="
        + ",".join(f"0x{offset:06X}" for offset, _ in find_microcode_ffs(result)),
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    print(OUTPUT)
    print("\n".join(metadata[10:]))


if __name__ == "__main__":
    main()
