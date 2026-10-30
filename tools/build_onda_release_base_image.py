from __future__ import annotations

import hashlib
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "post-stage1-boot-readback.bin"
DONOR = ROOT / "ref bios" / "ONDA H61ITX01.BIN"
OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4c_full-onda-bootstack-lanner-me8.bin"
)

EXPECTED_BASE_SHA256 = (
    "DD6BDB258D0F9DB0457B5E522402873C953ACFA3DC70CA58E58FE34DFB3F127B"
)
EXPECTED_DONOR_SHA256 = (
    "5F5E0AB78A2D736F443940F6691C0BA5EACD2D6DADAAC1A5284B5B3F32E20E72"
)
EXPECTED_OUTPUT_SHA256 = (
    "8523A25786DA11675E5DE769E77DFE2347DFEC818E6E83D4E4737068B79FB4B7"
)

TARGET_BOOTSTACK_START = 0x420000
TARGET_DONOR_START = 0x580000
TARGET_PEI_START = 0x760000
FLASH_END = 0x800000
DONOR_BIOS_START = 0x180000
DONOR_PEI_START = 0x360000

WDT_APP_FFS_OFFSET = 0x3BB708
WDT_APP_FFS_SIZE = 0x42C
WDT_APP_GUID = uuid.UUID("0F69F6D7-0E4B-43A6-BFC2-6871694369B0")
WDT_APP_ENTRY_OFFSET = 0x3BB99C
ORIGINAL_WDT_ENTRY = bytes.fromhex("55 8B EC")
DISABLED_WDT_ENTRY = bytes.fromhex("31 C0 C3")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def recalculate_ffs_checksums(image: bytearray, offset: int, size: int) -> None:
    if image[offset : offset + 16] != WDT_APP_GUID.bytes_le:
        raise ValueError("WdtAppPei GUID not found at the expected donor offset")
    if int.from_bytes(image[offset + 20 : offset + 23], "little") != size:
        raise ValueError("Unexpected WdtAppPei FFS size")
    if not image[offset + 19] & 0x40:
        raise ValueError("WdtAppPei does not use a data checksum")

    image[offset + 17] = (-sum(image[offset + 24 : offset + size])) & 0xFF
    header = bytearray(image[offset : offset + 24])
    header[16] = 0
    header[17] = 0
    header[23] = 0
    image[offset + 16] = (-sum(header)) & 0xFF


def validate_ffs_checksums(image: bytes, offset: int, size: int) -> None:
    header = bytearray(image[offset : offset + 24])
    stored_header_checksum = header[16]
    header[16] = 0
    header[17] = 0
    header[23] = 0
    if (sum(header) + stored_header_checksum) & 0xFF:
        raise ValueError("Invalid WdtAppPei FFS header checksum")
    if (sum(image[offset + 24 : offset + size]) + image[offset + 17]) & 0xFF:
        raise ValueError("Invalid WdtAppPei FFS data checksum")


def build_image(base: bytes, donor: bytes) -> bytes:
    if len(base) != FLASH_END:
        raise ValueError("Post-Stage1 base is not 8 MiB")
    if len(donor) != 0x400000:
        raise ValueError("ONDA donor is not 4 MiB")
    if sha256(base) != EXPECTED_BASE_SHA256:
        raise ValueError("Post-Stage1 base hash does not match")
    if sha256(donor) != EXPECTED_DONOR_SHA256:
        raise ValueError("ONDA donor hash does not match")
    if (
        donor[WDT_APP_ENTRY_OFFSET : WDT_APP_ENTRY_OFFSET + 3]
        != ORIGINAL_WDT_ENTRY
    ):
        raise ValueError("Unexpected ONDA WdtAppPei entry-point bytes")

    patched_donor = bytearray(donor)
    patched_donor[
        WDT_APP_ENTRY_OFFSET : WDT_APP_ENTRY_OFFSET + 3
    ] = DISABLED_WDT_ENTRY
    recalculate_ffs_checksums(
        patched_donor,
        WDT_APP_FFS_OFFSET,
        WDT_APP_FFS_SIZE,
    )
    validate_ffs_checksums(
        patched_donor,
        WDT_APP_FFS_OFFSET,
        WDT_APP_FFS_SIZE,
    )

    result = bytearray(base)
    result[TARGET_BOOTSTACK_START:TARGET_DONOR_START] = b"\xFF" * (
        TARGET_DONOR_START - TARGET_BOOTSTACK_START
    )
    result[TARGET_DONOR_START:TARGET_PEI_START] = donor[
        DONOR_BIOS_START:DONOR_PEI_START
    ]
    result[TARGET_PEI_START:FLASH_END] = patched_donor[
        DONOR_PEI_START:FLASH_END
    ]

    if result[:TARGET_BOOTSTACK_START] != base[:TARGET_BOOTSTACK_START]:
        raise ValueError("Descriptor, ME, or lower Lanner data changed")
    if result[TARGET_BOOTSTACK_START:TARGET_DONOR_START] != b"\xFF" * (
        TARGET_DONOR_START - TARGET_BOOTSTACK_START
    ):
        raise ValueError("Obsolete Lanner DXE range was not erased")
    if result[TARGET_DONOR_START:TARGET_PEI_START] != donor[
        DONOR_BIOS_START:DONOR_PEI_START
    ]:
        raise ValueError("ONDA pre-PEI boot-stack copy mismatch")

    final = bytes(result)
    if sha256(final) != EXPECTED_OUTPUT_SHA256:
        raise ValueError("Release base does not match verified Stage 4C")
    return final


def main() -> None:
    for required in (BASE, DONOR):
        if not required.is_file():
            raise FileNotFoundError(required)

    result = build_image(BASE.read_bytes(), DONOR.read_bytes())
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(result)
    metadata = [
        "Reproducible Stage 4C release base.",
        "Built directly from the initialized Lanner ME8 read-back and ONDA donor.",
        "Eliminates the historical Angxun X7-VB Stage 2A dependency because",
        "Stage 4C erases the complete range containing its microcode changes.",
        "Disables ONDA WdtAppPei using the hardware-tested Stage 3B patch.",
        f"base_sha256={EXPECTED_BASE_SHA256}",
        f"donor_sha256={EXPECTED_DONOR_SHA256}",
        f"image_sha256={sha256(result)}",
        f"bootstack_sha256={sha256(result[TARGET_BOOTSTACK_START:FLASH_END])}",
    ]
    OUTPUT.with_suffix(".txt").write_text(
        "\n".join(metadata) + "\n",
        encoding="ascii",
    )
    print(OUTPUT)
    print("\n".join(metadata[5:]))


if __name__ == "__main__":
    main()
