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
    "DB1AC3E848A6B0CE6252CBEB9EFD8C88B3DFF41D37EEC8D0D4CCC02A394DB8A7"
)

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gl_nii-pointer-reconnect.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gm"
    / "nomadix_pci_network_reconnect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gm_pci-network-reconnect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "4119581AA02F81674B3A2A9007DAD04D0D123E429DBF6F6F5F6F5F00ABBE0193"
)
builder.STAGE_NAME = "4GM"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GM based on Stage 4GL.",
    "Retains official Intel E9925X3.EFI 9.9.25 for all six 82583V NICs.",
    "Filters PCI I/O handles by exact vendor/device ID 8086:150C.",
    "Connects each matching PCI parent, then each resulting NII child.",
    "Requires EFI_SIMPLE_NETWORK_PROTOCOL on six handles before completion.",
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
        raise ValueError("Stage 4GM output SHA-256 changed")
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4gm-verify-"
    ) as temporary:
        intel_undi = extract_intel_undi(
            builder.OUTPUT,
            Path(temporary) / "intel-undi",
        )
    if hashlib.sha256(intel_undi).hexdigest().upper() != INTEL_UNDI_SHA256:
        raise ValueError("Stage 4GM changed the embedded Intel UNDI")


if __name__ == "__main__":
    main()
