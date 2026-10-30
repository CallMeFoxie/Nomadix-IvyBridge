from __future__ import annotations

import hashlib
import struct
import tempfile
from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gh_quiet-final.bin"
)
INTEL_UNDI = (
    ROOT
    / "ref bios"
    / "PREBOOT"
    / "APPS"
    / "EFI"
    / "EFIx64"
    / "E9925X3.EFI"
)
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gi_intel-82583v-undi.bin"
)
TARGET_GUID = "5A4D0030-9DB6-11E1-A8B0-0800200C9A66"
INTEL_UI_NAME = "Intel82583vUndiDxe"
BASE_SHA256 = (
    "3D843DADDBB95664D51341B76BA8AD8E707F26FC2D48747AEAA61A895AA0FB42"
)
INTEL_UNDI_SIZE = 394560
INTEL_UNDI_SHA256 = (
    "07122889AB1286F263286C1F8D07F8BE3AEAD8323882CB09E7CADE454D96E562"
)
ORIGINAL_REALTEK_SIZE = 121376
ORIGINAL_REALTEK_SHA256 = (
    "8666C0CDC4DA901498315B1EA496B2799554ED32CFCCC98B2967828C4E44F404"
)
OUTPUT_SHA256 = (
    "81E53E198639F6FC3F502AB5C959A7B26ECC910081B4494E885A992D8B63E7A5"
)
MAIN_FV_START = 0x5D0000


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def validate_intel_pe(data: bytes) -> None:
    if len(data) < 0x200 or data[:2] != b"MZ":
        raise ValueError("Intel UNDI is not a DOS/PE image")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("Intel UNDI has no PE signature")

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
        raise ValueError("Intel UNDI is not x86-64 PE32+")
    if subsystem != 0x000B:
        raise ValueError("Intel UNDI is not an EFI boot-service driver")
    if characteristics != 0x002E:
        raise ValueError("Intel UNDI COFF characteristics changed")
    if import_directory != (0, 0):
        raise ValueError("Intel UNDI unexpectedly imports external symbols")
    if relocation_directory[1] == 0:
        raise ValueError("Intel UNDI has no base relocations")

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
    if sections != {".text", ".data", ".reloc"}:
        raise ValueError("Intel UNDI PE section set changed")


def extract_section(
    image: Path,
    section_type: str,
    destination: Path,
) -> bytes:
    builder.run(
        [
            str(builder.UEFI_EXTRACT),
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
            f"UEFIExtract did not extract section {section_type} from "
            f"{TARGET_GUID}"
        )
    return body.read_bytes()


def replace_section(
    source: Path,
    section_type: str,
    payload: Path,
    output: Path,
) -> None:
    result = builder.run(
        [
            str(builder.UEFI_REPLACE),
            str(source),
            TARGET_GUID,
            section_type,
            str(payload),
            "-o",
            str(output),
        ]
    )
    if "File replaced" not in result:
        raise RuntimeError(f"UEFIReplace did not confirm replacement:\n{result}")


def build_once(
    workspace: Path,
    name: str,
    ui_payload: Path,
) -> Path:
    pe_output = workspace / f"{name}-pe.bin"
    final_output = workspace / f"{name}.bin"
    replace_section(BASE, builder.SECTION_PE32, INTEL_UNDI, pe_output)
    replace_section(pe_output, "15", ui_payload, final_output)
    return final_output


def validate_preserved_regions(base: bytes, result: bytes) -> None:
    if len(base) != builder.FLASH_END or len(result) != builder.FLASH_END:
        raise ValueError("Stage 4GI images must be exactly 8 MiB")
    if result[:MAIN_FV_START] != base[:MAIN_FV_START]:
        raise ValueError("Descriptor, ME, NVRAM, or pre-main-FV data changed")
    if result[builder.PEI_START :] != base[builder.PEI_START :]:
        raise ValueError("PEI, SEC, or second microcode region changed")

    base_microcodes = sorted(
        blob for _, blob in builder.find_microcode_ffs(base)
    )
    result_microcodes = sorted(
        blob for _, blob in builder.find_microcode_ffs(result)
    )
    if result_microcodes != base_microcodes:
        raise ValueError("A microcode FFS changed")


def main() -> None:
    base = BASE.read_bytes()
    if sha256(base) != BASE_SHA256:
        raise ValueError("Base image is not exact hardware-tested Stage 4GH")

    intel_undi = INTEL_UNDI.read_bytes()
    if len(intel_undi) != INTEL_UNDI_SIZE:
        raise ValueError("Unexpected Intel E9925X3.EFI size")
    if sha256(intel_undi) != INTEL_UNDI_SHA256:
        raise ValueError("Unexpected Intel E9925X3.EFI SHA-256")
    validate_intel_pe(intel_undi)
    if bytes.fromhex("86800C15") not in intel_undi:
        raise ValueError("Intel UNDI does not contain PCI ID 8086:150C")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4gi-intel-undi-"
    ) as temporary:
        workspace = Path(temporary)
        original_realtek = extract_section(
            BASE,
            builder.SECTION_PE32,
            workspace / "original-realtek",
        )
        if len(original_realtek) != ORIGINAL_REALTEK_SIZE:
            raise ValueError("Unexpected original RtkUndiDxe PE32 size")
        if sha256(original_realtek) != ORIGINAL_REALTEK_SHA256:
            raise ValueError("Unexpected original RtkUndiDxe PE32 SHA-256")

        ui_payload = workspace / "intel-undi-ui.bin"
        ui_bytes = (INTEL_UI_NAME + "\0").encode("utf-16-le")
        ui_payload.write_bytes(ui_bytes)

        first_path = build_once(workspace, "first", ui_payload)
        second_path = build_once(workspace, "second", ui_payload)
        first = first_path.read_bytes()
        second = second_path.read_bytes()
        if first != second:
            raise ValueError("Stage 4GI build is not deterministic")
        if sha256(first) != OUTPUT_SHA256:
            raise ValueError("Stage 4GI output SHA-256 changed")
        validate_preserved_regions(base, first)
        OUTPUT.write_bytes(first)

        builder.run([str(builder.UEFI_EXTRACT), str(OUTPUT), "report"])
        report = OUTPUT.with_suffix(OUTPUT.suffix + ".report.txt")
        report_text = report.read_text(errors="replace")
        if TARGET_GUID not in report_text or INTEL_UI_NAME not in report_text:
            raise ValueError("UEFI report does not contain the Intel UNDI FFS")
        if "RtkUndiDxe" in report_text:
            raise ValueError("UEFI report still labels the replaced driver as Realtek")

        embedded_undi = extract_section(
            OUTPUT,
            builder.SECTION_PE32,
            workspace / "verify-intel-undi",
        )
        if embedded_undi != intel_undi:
            raise ValueError("Embedded Intel UNDI differs from E9925X3.EFI")
        embedded_ui = extract_section(
            OUTPUT,
            "15",
            workspace / "verify-intel-ui",
        )
        if embedded_ui != ui_bytes:
            raise ValueError("Embedded Intel UNDI UI name differs")

    metadata = [
        "Experimental Stage 4GI based on hardware-tested Stage 4GH.",
        "NOT HARDWARE TESTED.",
        "Replaces only the existing RtkUndiDxe PE32 and UI sections.",
        "Retains the existing automatically dispatched DXE FFS container.",
        "Embeds official Intel E9925X3.EFI 9.9.25 for PCI 8086:150C.",
        "The builder does not invoke BootUtil or write NIC NVM.",
        "The Intel driver contains explicit configuration/NVDATA write paths.",
        "Normal dispatch and PXE boot are expected to use read-only NVM paths.",
        "Descriptor, ME, NVRAM, PEI, SEC, and microcode bytes are preserved.",
        f"base_sha256={BASE_SHA256}",
        f"intel_undi_sha256={INTEL_UNDI_SHA256}",
        f"image_sha256={OUTPUT_SHA256}",
        f"onda_bootstack_sha256={sha256(first[0x420000:0x800000])}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n",
        encoding="ascii",
    )
    print(OUTPUT)
    print("\n".join(metadata[-4:]))


if __name__ == "__main__":
    main()
