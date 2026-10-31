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
    "1516D1E92E4D98D723BDC9B814AB3E367091AD9C7EB9B7D095ED92C4B12B59E6"
)

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gm_pci-network-reconnect.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gn"
    / "nomadix_single_port_pxe_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gn_single-port-pxe.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "DB1AC3E848A6B0CE6252CBEB9EFD8C88B3DFF41D37EEC8D0D4CCC02A394DB8A7"
)
builder.STAGE_NAME = "4GN"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GN based on hardware-tested Stage 4GM.",
    "Wraps Intel Supported() before BDS to allow only PCI 0000:01:00.0.",
    "Connects only the first 82583V and its resulting NII child.",
    "Removes Intel HII package lists to hide incompatible null-string pages.",
    "Retains official Intel E9925X3.EFI 9.9.25 byte-for-byte.",
    "Uses direct serial output independent of Stage 4GH quiet diagnostics.",
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
        raise ValueError("Stage 4GN output SHA-256 changed")
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4gn-verify-"
    ) as temporary:
        intel_undi = extract_intel_undi(
            builder.OUTPUT,
            Path(temporary) / "intel-undi",
        )
    if hashlib.sha256(intel_undi).hexdigest().upper() != INTEL_UNDI_SHA256:
        raise ValueError("Stage 4GN changed the embedded Intel UNDI")


if __name__ == "__main__":
    main()
