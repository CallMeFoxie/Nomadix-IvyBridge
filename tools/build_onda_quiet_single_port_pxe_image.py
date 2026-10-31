from __future__ import annotations

import hashlib
import tempfile
from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
INTEL_UNDI_GUID = "5A4D0030-9DB6-11E1-A8B0-0800200C9A66"
INTEL_UNDI_SHA256 = (
    "07122889AB1286F263286C1F8D07F8BE3AEAD8323882CB09E7CADE454D96E562"
)
OUTPUT_SHA256 = (
    "1452E92B27AC2244B59164DBDF111BF9D15EDE08EB3EB54B4DA1F18E6FED7B79"
)

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4go_notified-single-port-pxe.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gp"
    / "nomadix_quiet_single_port_pxe_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gp_quiet-single-port-pxe.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "EB153383EC033EC12D6FDD579C68F09ABB1094A5352BC6D9C8CAD9904AF37BE4"
)
builder.STAGE_NAME = "4GP"
builder.DESCRIPTION_LINES = [
    "Quiet Stage 4GP based on hardware-tested Stage 4GO.",
    "Retains first-port-only Intel 82583V UEFI PXE behavior.",
    "Retains removal of incompatible Intel adapter HII package lists.",
    "Compiles out all NOMADIX-4GJ through NOMADIX-4GO network diagnostics.",
    "Leaves all six 82583V controllers available to OpenWrt e1000e.",
    "UEFI PXE is intentionally limited to PCI 0000:01:00.0.",
]


def extract_intel_undi(image: Path, destination: Path) -> bytes:
    builder.run(
        [
            str(builder.UEFI_EXTRACT),
            str(image),
            INTEL_UNDI_GUID,
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
        raise RuntimeError("UEFIExtract did not extract embedded Intel UNDI")
    return body.read_bytes()


def main() -> None:
    builder.main()
    output = builder.OUTPUT.read_bytes()
    if hashlib.sha256(output).hexdigest().upper() != OUTPUT_SHA256:
        raise ValueError("Stage 4GP output SHA-256 changed")
    driver = builder.DRIVER_OUTPUT.read_bytes()
    if b"[NOMADIX-4G" in driver:
        raise ValueError("Stage 4GP still contains network diagnostics")
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4gp-verify-"
    ) as temporary:
        intel_undi = extract_intel_undi(
            builder.OUTPUT,
            Path(temporary) / "intel-undi",
        )
    if hashlib.sha256(intel_undi).hexdigest().upper() != INTEL_UNDI_SHA256:
        raise ValueError("Stage 4GP changed the embedded Intel UNDI")


if __name__ == "__main__":
    main()
