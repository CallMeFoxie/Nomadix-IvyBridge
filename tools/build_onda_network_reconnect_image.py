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
    "5C78C0CE4E36B0293B8F26FFC5C3091C51DF22F5BA185289AA4AE74D054B40CA"
)

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gi_intel-82583v-undi.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4gj"
    / "nomadix_network_reconnect_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4gj_network-reconnect.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "81E53E198639F6FC3F502AB5C959A7B26ECC910081B4494E885A992D8B63E7A5"
)
builder.STAGE_NAME = "4GJ"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4GJ based on structurally validated Stage 4GI.",
    "Retains official Intel E9925X3.EFI 9.9.25 for all six 82583V NICs.",
    "Registers for EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_31.",
    "Recursively connects each new NII handle during the AMI BDS pass.",
    "Intended to attach SNP, DHCP, IP, and PXE before Boot menu enumeration.",
    "Emits one NOMADIX-4GJ NII/CONNECT result per discovered interface.",
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
        raise ValueError("Stage 4GJ output SHA-256 changed")
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4gj-verify-"
    ) as temporary:
        intel_undi = extract_intel_undi(
            builder.OUTPUT,
            Path(temporary) / "intel-undi",
        )
    if hashlib.sha256(intel_undi).hexdigest().upper() != INTEL_UNDI_SHA256:
        raise ValueError("Stage 4GJ changed the embedded Intel UNDI")


if __name__ == "__main__":
    main()
