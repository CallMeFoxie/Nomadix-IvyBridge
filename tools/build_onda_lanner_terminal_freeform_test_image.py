from __future__ import annotations

import hashlib
import tempfile
import uuid
from pathlib import Path

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "output" / "onda-exact" / "TEST_ONLY_stage4f4_serial-io.bin"
INTERNAL_OUTPUT = (
    ROOT / "output" / "onda-exact" / "INTERNAL_stage4fe_loader.bin"
)
FINAL_OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fe_lanner-terminal-freeform-test.bin"
)
FINAL_DRIVER = FINAL_OUTPUT.with_suffix(".efi")
LANNER_DUMP = ROOT / "bios_backup3.bin"
LANNER_TERMINAL_GUID = "7A08CB98-E9BC-41C3-BE19-B302F3F1F595"
PAYLOAD_FILE_GUID = "D3C9A4E1-7B62-4A41-91E8-54421073FC2D"
LANNER_TERMINAL_SIZE = 0x9E20
LANNER_TERMINAL_SHA256 = (
    "59D496AD71BF69AF554C18461FCA5AD7695158E5E46061A5F0C59D846D2328BC"
)
BASE_SHA256 = (
    "D883584A746D686E26C1904339230CAEFFFA79AEE7E7E7393BFF0DE5CFF196C9"
)

builder.BASE = BASE
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4fe"
    / "nomadix_lanner_terminal_freeform_test_dxe.c"
)
builder.OUTPUT = INTERNAL_OUTPUT
builder.DRIVER_OUTPUT = INTERNAL_OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = BASE_SHA256
builder.STAGE_NAME = "4FE"
builder.DESCRIPTION_LINES = [
    "Internal Stage 4FE loader build based on hardware-tested Stage 4F4.",
    "Uses a small loader with no embedded Terminal payload.",
    "The final builder adds the payload as a non-dispatchable freeform FFS.",
    "Does not read, create, modify, or delete UEFI variables.",
]


def extract_lanner_terminal(destination: Path) -> bytes:
    builder.run(
        [
            str(builder.UEFI_EXTRACT),
            str(LANNER_DUMP),
            LANNER_TERMINAL_GUID,
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
        raise RuntimeError("UEFIExtract did not extract the Lanner Terminal PE32")
    payload = body.read_bytes()
    if len(payload) != LANNER_TERMINAL_SIZE:
        raise ValueError("Unexpected Lanner Terminal PE32 size")
    if hashlib.sha256(payload).hexdigest().upper() != LANNER_TERMINAL_SHA256:
        raise ValueError("Unexpected Lanner Terminal PE32 hash")
    builder.validate_pe(payload)
    return payload


def build_freeform_ffs(payload: bytes) -> bytes:
    section_size = 4 + len(payload)
    if section_size >= 0x1000000:
        raise ValueError("Lanner PE32 section requires an extended header")
    section = (
        section_size.to_bytes(3, "little")
        + bytes((int(builder.SECTION_PE32, 16),))
        + payload
    )
    section += bytes((-len(section)) & 3)

    file_size = 24 + len(section)
    if file_size >= 0x1000000:
        raise ValueError("Lanner payload FFS requires an extended header")
    header = bytearray(24)
    header[:16] = uuid.UUID(PAYLOAD_FILE_GUID).bytes_le
    header[17] = (-sum(section)) & 0xFF
    header[18] = 0x02
    header[19] = 0x40
    header[20:23] = file_size.to_bytes(3, "little")
    header[23] = 0xF8
    checksum_header = bytearray(header)
    checksum_header[17] = 0
    checksum_header[23] = 0
    header[16] = (-sum(checksum_header)) & 0xFF

    verification_header = bytearray(header)
    verification_header[17] = 0
    verification_header[23] = 0
    if sum(verification_header) & 0xFF:
        raise ValueError("Generated freeform FFS header checksum is invalid")
    if (sum(section) + header[17]) & 0xFF:
        raise ValueError("Generated freeform FFS data checksum is invalid")
    result = bytes(header) + section
    return result + bytes((0xFF,)) * ((-len(result)) & 7)


def insert_into_trailing_free_space(image: bytes, ffs: bytes) -> tuple[bytes, int]:
    cursor = builder.PEI_START
    while cursor > builder.PROTECTED_PREFIX_END and image[cursor - 1] == 0xFF:
        cursor -= 1
    free_start = (cursor + 7) & ~7
    if free_start + len(ffs) > builder.PEI_START:
        raise ValueError("Outer firmware volume has insufficient free space")
    if any(value != 0xFF for value in image[free_start : free_start + len(ffs)]):
        raise ValueError("Payload target is not erased firmware-volume space")
    result = bytearray(image)
    result[free_start : free_start + len(ffs)] = ffs
    return bytes(result), free_start


def cleanup_internal() -> None:
    for path in (
        INTERNAL_OUTPUT,
        INTERNAL_OUTPUT.with_suffix(".efi"),
        INTERNAL_OUTPUT.with_suffix(".txt"),
        INTERNAL_OUTPUT.with_suffix(INTERNAL_OUTPUT.suffix + ".report.txt"),
    ):
        if path.exists():
            path.unlink()


def main() -> None:
    cleanup_internal()
    try:
        builder.main()
        internal = INTERNAL_OUTPUT.read_bytes()
        driver = builder.DRIVER_OUTPUT.read_bytes()
        if len(driver) >= 0x4000:
            raise ValueError("Stage 4FE loader unexpectedly exceeds 16 KiB")

        with tempfile.TemporaryDirectory(
            prefix="nomadix-stage4fe-freeform-"
        ) as temporary:
            workspace = Path(temporary)
            payload = extract_lanner_terminal(workspace / "lanner-terminal")
            ffs = build_freeform_ffs(payload)
            first, file_offset = insert_into_trailing_free_space(internal, ffs)
            second, second_offset = insert_into_trailing_free_space(internal, ffs)
            if first != second or file_offset != second_offset:
                raise ValueError("Freeform FFS insertion is not deterministic")

            FINAL_OUTPUT.write_bytes(first)
            FINAL_DRIVER.write_bytes(driver)
            builder.run([str(builder.UEFI_EXTRACT), str(FINAL_OUTPUT), "report"])
            report = FINAL_OUTPUT.with_suffix(
                FINAL_OUTPUT.suffix + ".report.txt"
            )
            report_text = report.read_text(errors="replace")
            if PAYLOAD_FILE_GUID not in report_text:
                raise ValueError("UEFIExtract did not parse the freeform payload FFS")

            extracted_directory = workspace / "verify-payload"
            builder.run(
                [
                    str(builder.UEFI_EXTRACT),
                    str(FINAL_OUTPUT),
                    PAYLOAD_FILE_GUID,
                    "-o",
                    str(extracted_directory),
                    "-m",
                    "body",
                    "-t",
                    builder.SECTION_PE32,
                ]
            )
            if (extracted_directory / "body.bin").read_bytes() != payload:
                raise ValueError("Final freeform FFS payload does not match Lanner PE")

            base = BASE.read_bytes()
            result = FINAL_OUTPUT.read_bytes()
            if result[: builder.PROTECTED_PREFIX_END] != base[
                : builder.PROTECTED_PREFIX_END
            ]:
                raise ValueError("Descriptor, ME, NVRAM, or prefix changed")
            if result[builder.PEI_START :] != base[builder.PEI_START :]:
                raise ValueError("PEI/SEC or second microcode region changed")
            internal_microcodes = builder.find_microcode_ffs(internal)
            result_microcodes = builder.find_microcode_ffs(result)
            if internal_microcodes != result_microcodes:
                raise ValueError("Freeform insertion moved or changed microcode FFS")

            metadata = [
                "Experimental Stage 4FE based on hardware-tested Stage 4F4.",
                "Stage 4FC is unsafe and must not be flashed.",
                "Uses a small custom loader without an embedded payload.",
                "Stores the exact Lanner Terminal PE in outer-FV erased space.",
                "The payload FFS type is FREEFORM and cannot be DXE-dispatched.",
                "Existing FFS files and their offsets are not moved by insertion.",
                "The loader reads the payload through FV2 ReadSection by GUID.",
                "This stage calls LoadImage only and does not call StartImage.",
                "The RAM-only COM1 device path becomes ACPI PNP0501/UID0.",
                "No UEFI variable is read, created, modified, or deleted.",
                f"payload_file_guid={PAYLOAD_FILE_GUID}",
                f"payload_ffs_offset=0x{file_offset:06X}",
                f"payload_ffs_size=0x{len(ffs):X}",
                f"lanner_terminal_pe_sha256={LANNER_TERMINAL_SHA256}",
                f"base_sha256={builder.sha256(base)}",
                f"driver_sha256={builder.sha256(driver)}",
                f"image_sha256={builder.sha256(result)}",
                "bootstack_sha256="
                + builder.sha256(result[0x420000 : builder.FLASH_END]),
                "microcode_ffs_offsets="
                + ",".join(
                    f"0x{offset:06X}" for offset, _ in result_microcodes
                ),
            ]
            FINAL_OUTPUT.with_suffix(".txt").write_text(
                "\n".join(metadata) + "\n",
                encoding="ascii",
            )
            print(FINAL_OUTPUT)
            print("\n".join(metadata[9:]))
    finally:
        cleanup_internal()


if __name__ == "__main__":
    main()
