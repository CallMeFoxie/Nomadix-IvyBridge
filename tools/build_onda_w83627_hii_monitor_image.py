from pathlib import Path
import tempfile

import build_onda_platform_dxe_image as builder


ROOT = Path(__file__).resolve().parents[1]
SETUP_GUID = "899407D7-99FE-43D8-9A21-79EC328CAC21"
SETUP_IFR_SECTION = "18"

builder.BASE = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4fs_serial-ansi-bold.bin"
)
builder.SOURCE = (
    ROOT
    / "firmware"
    / "stage4ft"
    / "nomadix_w83627_hii_monitor_dxe.c"
)
builder.OUTPUT = (
    ROOT
    / "output"
    / "onda-exact"
    / "TEST_ONLY_stage4ft_w83627-hii-monitor.bin"
)
builder.DRIVER_OUTPUT = builder.OUTPUT.with_suffix(".efi")
builder.EXPECTED_BASE_SHA256 = (
    "0EB27240360DDEFDAEEB94E87DE2D5CA18D901DDB579E68E9A12F27DAC50EBE6"
)
builder.STAGE_NAME = "4FT"
builder.DESCRIPTION_LINES = [
    "Experimental Stage 4FT based on Stage 4FS.",
    "Retains the cumulative bidirectional serial AMI Setup console.",
    "Publishes a read-only W83627DHG-P hardware monitor through the existing "
    "AMI HII page.",
    "Shows board temp1/temp3, fan1-fan4 RPM, and PWM1-PWM4 percentages.",
    "Uses the Linux w83627ehf register map and fan-divisor conversion.",
    "Converts the legacy ITE Smart Fan checkbox into a read-only text row.",
    "Suppresses the legacy ITE Smart Fan configuration submenu.",
    "Does not write W83627 HWM registers or UEFI variables.",
    "Preserves the original TRUE/END DXE dependency section and FFS GUID.",
]

_replace_platform_pe32 = builder.replace_pe32


def extract_setup_ifr(image: Path, destination: Path) -> bytes:
    builder.run(
        [
            str(builder.UEFI_EXTRACT),
            str(image),
            SETUP_GUID,
            "-o",
            str(destination),
            "-m",
            "body",
            "-t",
            SETUP_IFR_SECTION,
        ]
    )
    body = destination / "body.bin"
    if not body.is_file():
        raise RuntimeError("UEFIExtract did not extract the Setup IFR section")
    return body.read_bytes()


def replace_once(data: bytes, old: bytes, new: bytes, name: str) -> bytes:
    if len(old) != len(new):
        raise ValueError(f"{name} patch changes the Setup IFR size")
    count = data.count(old)
    if count != 1:
        raise ValueError(f"Expected one {name} sequence, found {count}")
    return data.replace(old, new, 1)


def patch_setup_ifr(data: bytes) -> bytes:
    data = replace_once(
        data,
        bytes.fromhex("06 8E BB 05 BC 05 CE 00 01 00 65 02 10 01 29 02"),
        bytes.fromhex("03 08 BB 05 BC 05 BC 05 00 02 00 02 00 02 00 02"),
        "Smart Fan checkbox",
    )
    return replace_once(
        data,
        bytes.fromhex("12 06 CE 00 00 00"),
        bytes.fromhex("46 02 00 02 00 02"),
        "Smart Fan submenu condition",
    )


def replace_pe32_and_setup_ifr(driver: Path) -> None:
    _replace_platform_pe32(driver)
    with tempfile.TemporaryDirectory(
        prefix="nomadix-stage4ft-setup-"
    ) as temporary:
        workspace = Path(temporary)
        original = extract_setup_ifr(
            builder.OUTPUT, workspace / "original-setup-ifr"
        )
        patched = patch_setup_ifr(original)
        patched_path = workspace / "patched-setup-ifr.bin"
        patched_path.write_bytes(patched)
        replaced_image = workspace / "stage4ft-with-setup.bin"
        output = builder.run(
            [
                str(builder.UEFI_REPLACE),
                str(builder.OUTPUT),
                SETUP_GUID,
                SETUP_IFR_SECTION,
                str(patched_path),
                "-o",
                str(replaced_image),
            ]
        )
        if "File replaced" not in output:
            raise RuntimeError(
                f"UEFIReplace did not confirm Setup IFR replacement:\n{output}"
            )
        replaced_image.replace(builder.OUTPUT)
        verified = extract_setup_ifr(
            builder.OUTPUT, workspace / "verified-setup-ifr"
        )
        if verified != patched:
            raise ValueError(
                "Stage 4FT Setup IFR does not match the patched data"
            )


builder.replace_pe32 = replace_pe32_and_setup_ifr


if __name__ == "__main__":
    builder.main()
