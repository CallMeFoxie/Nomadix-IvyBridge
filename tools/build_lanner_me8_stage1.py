from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LANNER = ROOT / "lanner-read1.bin"
ASROCK = ROOT / "ref bios" / "ASRock H61M 2.00"
OUTPUT = ROOT / "output" / "TEST_ONLY_stage1_livebase_me8.bin"
DEFAULT_FITC = (
    ROOT
    / "tools"
    / "Intel ME System Tools v8 r3"
    / "Flash Image Tool"
    / "v8.1.40.1456"
    / "fitc.exe"
)

EXPECTED_LANNER_SHA256 = (
    "F0231B4E134E50CC7290B8BF8FBE3AAB8FA1482289816497321E427D8B3A5120"
)
EXPECTED_ASROCK_SHA256 = (
    "899958FE4A32745B566379393DC9B5138E56FE448760091234DF0FD1606685D2"
)
EXPECTED_FITC_SHA256 = (
    "D78F66E70949C33C7F6E589F3E13596C0AF7AA808CE8E3819A5A784CD3A4EF3E"
)
EXPECTED_OUTPUT_SHA256 = (
    "782B5713D7DC40E54BA75184379D5A0B03C7CD602362C8C8343B5E47A5BC176A"
)

FLASH_SIZE = 0x800000
DESCRIPTOR_END = 0x1000
ME_END = 0x400000
BIOS_START = ME_END


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def run(command: list[str], cwd: Path) -> str:
    process = subprocess.run(
        command,
        cwd=cwd,
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


def named_element(root: ET.Element, tag: str, name: str) -> ET.Element:
    for element in root.iter(tag):
        if element.get("name") == name:
            return element
    raise ValueError(f"FITC configuration element not found: {tag} {name}")


def prepare_config(
    source: Path,
    destination: Path,
    bios_region: Path,
    me_region: Path,
    output: Path,
) -> None:
    tree = ET.parse(source)
    root = tree.getroot()

    named_element(
        root,
        "DensityComp1",
        "Flash component 1 density",
    ).set("value", "8MB")
    named_element(
        root,
        "RegionLength",
        "BIOS region length",
    ).set("value", "0x00400000")
    named_element(
        root,
        "InputFile",
        "BIOS binary input file",
    ).set("value", str(bios_region))
    named_element(
        root,
        "InputFile",
        "ME Binary Input File",
    ).set("value", str(me_region))
    named_element(
        root,
        "LanPowerWell",
        "LAN Power Well Config",
    ).set("value", "3")

    prog_settings = root.find("ProgSettings")
    if prog_settings is None:
        raise ValueError("FITC configuration has no ProgSettings")
    build_output = prog_settings.find("BuildOutputFilename")
    if build_output is None:
        raise ValueError("FITC configuration has no BuildOutputFilename")
    build_output.set("value", str(output))

    ET.indent(tree, space="  ")
    tree.write(
        destination,
        encoding="utf-8",
        xml_declaration=True,
    )


def build_stage1(fitc: Path) -> bytes:
    for path, expected in (
        (LANNER, EXPECTED_LANNER_SHA256),
        (ASROCK, EXPECTED_ASROCK_SHA256),
        (fitc, EXPECTED_FITC_SHA256),
    ):
        if not path.is_file():
            raise FileNotFoundError(path)
        actual = sha256(path.read_bytes())
        if actual != expected:
            raise ValueError(
                f"Unexpected SHA-256 for {path}: {actual}, expected {expected}"
            )

    lanner = LANNER.read_bytes()
    if len(lanner) != FLASH_SIZE:
        raise ValueError("Lanner input is not an 8 MiB SPI image")

    with tempfile.TemporaryDirectory(prefix="nomadix-fitc-stage1-") as temporary:
        workspace = Path(temporary)
        donor = workspace / "ASRock H61M 2.00"
        shutil.copyfile(ASROCK, donor)
        config = workspace / "asrock.xml"
        fitc_output = workspace / "fitc-stage1.bin"
        bios_region = workspace / "lanner-bios-region.bin"
        bios_region.write_bytes(lanner[BIOS_START:FLASH_SIZE])

        decompose_log = run(
            [
                str(fitc),
                "/w",
                str(workspace),
                "/s",
                str(workspace),
                "/d",
                str(workspace / "Build"),
                "/save",
                str(config),
                str(donor),
            ],
            workspace,
        )
        donor_me_files = list(workspace.rglob("ME Region.bin"))
        if not config.is_file() or len(donor_me_files) != 1:
            created = "\n".join(
                str(path.relative_to(workspace))
                for path in sorted(workspace.rglob("*"))
            )
            raise RuntimeError(
                "FITC did not decompose the ASRock donor as expected.\n"
                f"{decompose_log}\nCreated files:\n{created}"
            )
        donor_me = donor_me_files[0]

        prepared_config = workspace / "lanner-me8.xml"
        prepare_config(
            config,
            prepared_config,
            bios_region,
            donor_me,
            fitc_output,
        )
        run(
            [
                str(fitc),
                "/b",
                "/o",
                str(fitc_output),
                "/w",
                str(workspace),
                "/s",
                str(workspace),
                "/d",
                str(workspace / "Build"),
                "/flashsize1",
                "4",
                str(prepared_config),
            ],
            workspace,
        )
        if not fitc_output.is_file():
            raise RuntimeError("FITC did not create the Stage 1 image")
        result = bytearray(fitc_output.read_bytes())

    if len(result) != FLASH_SIZE:
        raise ValueError("FITC Stage 1 output is not 8 MiB")
    result[:DESCRIPTOR_END] = lanner[:DESCRIPTOR_END]
    final = bytes(result)

    if final[:DESCRIPTOR_END] != lanner[:DESCRIPTOR_END]:
        raise ValueError("Original Lanner descriptor was not preserved")
    if final[BIOS_START:] != lanner[BIOS_START:]:
        raise ValueError("FITC changed the Lanner BIOS region")
    if final[DESCRIPTOR_END:ME_END] == lanner[DESCRIPTOR_END:ME_END]:
        raise ValueError("FITC did not replace the Intel ME region")
    if sha256(final) != EXPECTED_OUTPUT_SHA256:
        raise ValueError("Stage 1 output does not match the verified image")
    return final


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build the verified Lanner FW-7582 ME8 Stage 1 image.",
    )
    parser.add_argument(
        "--fitc",
        type=Path,
        default=DEFAULT_FITC,
        help="Path to Intel FITC 8.1.40.1456 fitc.exe",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    result = build_stage1(args.fitc.resolve())
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(result)
    metadata = [
        "Reproducible Lanner FW-7582 Stage 1 ME8 image.",
        "ASRock H61M-ITX 2.00 supplies Intel ME 8.0.13.1502.",
        "FITC configuration changes: 8 MiB component, 4 MiB Lanner BIOS,",
        "and Lanner LAN Power Well Config value 3.",
        "The original Lanner descriptor and BIOS region are preserved exactly.",
        f"lanner_sha256={EXPECTED_LANNER_SHA256}",
        f"asrock_sha256={EXPECTED_ASROCK_SHA256}",
        f"fitc_sha256={EXPECTED_FITC_SHA256}",
        f"image_sha256={sha256(result)}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n",
        encoding="ascii",
    )
    print(OUTPUT)
    print("\n".join(metadata[5:]))


if __name__ == "__main__":
    main()
