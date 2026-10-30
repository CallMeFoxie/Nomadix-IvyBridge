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
    "EB153383EC033EC12D6FDD579C68F09ABB1094A5352BC6D9C8CAD9904AF37BE4"
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
    / "stage4go"
    / "nomadix_notified_single_port_pxe_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4go_notified-single-port-pxe.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "DB1AC3E848A6B0CE6252CBEB9EFD8C88B3DFF41D37EEC8D0D4CCC02A394DB8A7"
)
builder.STAGE_NAME = "4GO"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GO based on hardware-tested Stage 4GM.",
    "Supersedes unsafe Stage 4GN, which returned EFI_NOT_FOUND from DXE.",
    "Waits for Intel Driver Binding through standard protocol notification.",
    "Installs the single-port Supported wrapper before AMI BDS ConnectAll.",
    "Arms network reconnect only after the filter is installed successfully.",
    "Never treats an absent early Intel binding as a DXE entry-point failure.",
    "Removes incompatible Intel HII packages after first-port initialization.",
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
        raise ValueError("Stage 4GO output SHA-256 changed")
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4go-verify-"
    ) as temporary:
        intel_undi = extract_intel_undi(
            builder.OUTPUT,
            Path(temporary) / "intel-undi",
        )
    if hashlib.sha256(intel_undi).hexdigest().upper() != INTEL_UNDI_SHA256:
        raise ValueError("Stage 4GO changed the embedded Intel UNDI")


if __name__ == "__main__":
    main()
