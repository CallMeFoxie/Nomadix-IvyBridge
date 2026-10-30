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
    "4119581AA02F81674B3A2A9007DAD04D0D123E429DBF6F6F5F6F5F00ABBE0193"
)

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gk_deferred-network-reconnect.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gl"
    / "nomadix_nii_pointer_reconnect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gl_nii-pointer-reconnect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "E332BFA573506056CA0D88D3EC9590D411D293A9AB3DE6EDCBACB8DD791C112E"
)
builder.STAGE_NAME = "4GL"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GL based on Stage 4GK.",
    "Retains official Intel E9925X3.EFI 9.9.25 for all six 82583V NICs.",
    "Polls Intel EFI_NII_POINTER_PROTOCOL handles every 10 ms.",
    "Connects both PCI parent and MAC child handles as Intel creates them.",
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
        raise ValueError("Stage 4GL output SHA-256 changed")
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4gl-verify-"
    ) as temporary:
        intel_undi = extract_intel_undi(
            builder.OUTPUT,
            Path(temporary) / "intel-undi",
        )
    if hashlib.sha256(intel_undi).hexdigest().upper() != INTEL_UNDI_SHA256:
        raise ValueError("Stage 4GL changed the embedded Intel UNDI")


if __name__ == "__main__":
    main()
