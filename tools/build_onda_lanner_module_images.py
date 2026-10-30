from __future__ import annotations

import hashlib
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4c_full-onda-bootstack-lanner-me8.bin"
)
LANNER = ROOT / "post-stage1-boot-readback.bin"
OUTPUT = ROOT / "output" / "onda-exact"
UEFI_EXTRACT = ROOT / "tools" / "UEFIExtract_NE_A76_win64" / "UEFIExtract.exe"
UEFI_REPLACE = ROOT / "tools" / "UEFIReplace.exe"

PROTECTED_PREFIX_END = 0x580000
PEI_START = 0x760000
FLASH_END = 0x800000
SECTION_DXE_DEPEX = "13"
SECTION_PE32 = "10"


@dataclass(frozen=True)
class Variant:
    stage: str
    slug: str
    name: str
    guid: str
    rationale: str


VARIANTS = (
    Variant(
        stage="4D1",
        slug="lanner-oemdxe",
        name="OEMDXE",
        guid="BFE205C9-5B17-4F8F-9375-89614AF8E199",
        rationale="Lanner board/OEM initialization hook",
    ),
    Variant(
        stage="4D2",
        slug="lanner-crbdxe",
        name="CRBDXE",
        guid="16271FCA-55D9-4A33-93FC-5A3EB128DEB6",
        rationale="Lanner CRB board initialization hook",
    ),
    Variant(
        stage="4D3",
        slug="lanner-acpi",
        name="ACPI",
        guid="16D0A23E-C09C-407D-A14A-AD058FDD0CA1",
        rationale="Lanner ACPI table/platform implementation",
    ),
    Variant(
        stage="4D4",
        slug="lanner-csplibdxe",
        name="CspLibDxe",
        guid="CD84562C-6864-40A3-A081-C8D35E82B920",
        rationale="Lanner chipset-support library DXE implementation",
    ),
    Variant(
        stage="4E1",
        slug="lanner-amiboardinfo",
        name="AmiBoardInfo",
        guid="9F3A0016-AE55-4288-829D-D22FD344C347",
        rationale="Lanner board-information and platform-data implementation",
    ),
)

HARDWARE_RESULTS = {
    "4D1": "No observable change from Stage 4C; OEMDXE alone is insufficient.",
    "4D2": "No observable change from Stage 4C; CRBDXE alone is insufficient.",
    "4D3": (
        "ACPI table construction changes, but DSDT size and PnP devices remain "
        "unchanged; UART and fan behavior are unchanged."
    ),
    "4D4": (
        "OpenWrt network access works after correcting its configuration, "
        "but fan and UART behavior are unchanged; CspLibDxe alone is insufficient."
    ),
    "4E1": (
        "No POST and persistent hang; Lanner AmiBoardInfo is incompatible "
        "as a standalone transplant into the ONDA DXE environment."
    ),
}


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


def extract_section(
    image: Path, guid: str, section_type: str, destination: Path
) -> bytes | None:
    process = subprocess.run(
        [
            str(UEFI_EXTRACT),
            str(image),
            guid,
            "-o",
            str(destination),
            "-m",
            "body",
            "-t",
            section_type,
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if process.returncode:
        if destination.exists():
            shutil.rmtree(destination)
        return None
    body = destination / "body.bin"
    if not body.is_file():
        raise ValueError(
            f"UEFIExtract did not create body.bin for {guid} section {section_type}"
        )
    return body.read_bytes()


def replace_section(
    image: Path,
    guid: str,
    section_type: str,
    contents: Path,
    output: Path,
) -> None:
    result = run(
        [
            str(UEFI_REPLACE),
            str(image),
            guid,
            section_type,
            str(contents),
            "-o",
            str(output),
        ]
    )
    if "File replaced" not in result:
        raise RuntimeError(f"UEFIReplace did not confirm replacement:\n{result}")


def verify_section(
    image: Path,
    guid: str,
    section_type: str,
    expected: bytes,
    workspace: Path,
) -> None:
    actual = extract_section(
        image, guid, section_type, workspace / f"verify-{section_type}"
    )
    if actual != expected:
        raise ValueError(
            f"Output {guid} section {section_type} differs from Lanner source"
        )


def build_variant(
    variant: Variant, base: bytes, workspace: Path
) -> tuple[Path, list[str]]:
    variant_workspace = workspace / variant.stage.lower()
    variant_workspace.mkdir()
    current = BASE
    replaced: list[tuple[str, str, bytes]] = []

    for section_type, label in (
        (SECTION_DXE_DEPEX, "DXE dependency"),
        (SECTION_PE32, "PE32 executable"),
    ):
        section_directory = variant_workspace / f"source-{section_type}"
        contents = extract_section(
            LANNER, variant.guid, section_type, section_directory
        )
        if contents is None:
            if section_type == SECTION_PE32:
                raise ValueError(f"{variant.name} has no Lanner PE32 section")
            continue
        next_image = variant_workspace / f"after-{section_type}.bin"
        replace_section(
            current,
            variant.guid,
            section_type,
            section_directory / "body.bin",
            next_image,
        )
        current = next_image
        replaced.append((section_type, label, contents))

    output = (
        OUTPUT
        / f"TEST_ONLY_stage{variant.stage.lower()}_{variant.slug}.bin"
    )
    shutil.copyfile(current, output)
    result = output.read_bytes()
    if len(result) != FLASH_END:
        raise ValueError(f"{variant.stage} output is not 8 MiB")
    if result[:PROTECTED_PREFIX_END] != base[:PROTECTED_PREFIX_END]:
        raise ValueError(f"{variant.stage} changed data below the ONDA stack")
    if result[PEI_START:] != base[PEI_START:]:
        raise ValueError(f"{variant.stage} changed the ONDA PEI region")

    for section_type, _, expected in replaced:
        verify_section(
            output,
            variant.guid,
            section_type,
            expected,
            variant_workspace,
        )

    parse_image = variant_workspace / output.name
    shutil.copyfile(output, parse_image)
    run([str(UEFI_EXTRACT), str(parse_image), "report"])
    metadata = [
        f"Experimental Stage {variant.stage}: one Lanner module in the "
        "hardware-booting Stage 4C ONDA stack.",
    ]
    hardware_result = HARDWARE_RESULTS.get(variant.stage)
    if hardware_result is not None:
        metadata.append(f"hardware_result={hardware_result}")
    metadata.extend(
        [
            f"module={variant.name}",
            f"guid={variant.guid}",
            f"rationale={variant.rationale}",
            "replacement_scope="
            + ", ".join(label for _, label, _ in replaced),
            "All other modules remain from Stage 4C.",
            "0x000000-0x57FFFF and 0x760000-0x7FFFFF are byte-identical "
            "to Stage 4C.",
            f"base_sha256={sha256(base)}",
            f"image_sha256={sha256(result)}",
            f"bootstack_sha256={sha256(result[0x420000:FLASH_END])}",
            f"onda_prepei_sha256={sha256(result[PROTECTED_PREFIX_END:PEI_START])}",
            f"onda_pei_sha256={sha256(result[PEI_START:FLASH_END])}",
        ]
    )
    output.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n", encoding="ascii"
    )
    return output, metadata


def main() -> None:
    base = BASE.read_bytes()
    if len(base) != FLASH_END:
        raise ValueError("Stage 4C base is not 8 MiB")
    if len(LANNER.read_bytes()) != FLASH_END:
        raise ValueError("Lanner source is not 8 MiB")
    if not UEFI_EXTRACT.is_file() or not UEFI_REPLACE.is_file():
        raise FileNotFoundError("UEFIExtract or UEFIReplace is missing")

    with tempfile.TemporaryDirectory(prefix="nomadix-stage4d-") as temporary:
        workspace = Path(temporary)
        for variant in VARIANTS:
            output, metadata = build_variant(variant, base, workspace)
            print(output)
            print("\n".join(metadata[8:]))


if __name__ == "__main__":
    main()
