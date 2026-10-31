from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "build-inputs.json"

BUILD_STEPS = [
    "build_onda_release_base_image.py",
    "build_onda_microcode_image.py",
    "build_onda_platform_dxe_image.py",
    "build_onda_serial_diag_image.py",
    "build_onda_serial_io_image.py",
    "build_onda_serial_text_out_image.py",
    "build_onda_serial_text_in_out_image.py",
    "build_onda_serial_text_input_ex_image.py",
    "build_onda_serial_escape_scan_image.py",
    "build_onda_serial_setup_console_image.py",
    "build_onda_serial_clean_console_image.py",
    "build_onda_serial_ansi_input_fix_image.py",
    "build_onda_serial_bold_attribute_image.py",
    "build_onda_w83627_hii_formset_image.py",
    "build_onda_w83627_fan_profiles_image.py",
    "build_onda_w83627_persistent_fan_profile_image.py",
    "build_onda_foxnodix_smbios_branding_image.py",
    "build_onda_plcm_boot_message_image.py",
    "build_onda_plcm_lpt_enable_image.py",
    "build_onda_quiet_plcm_lpt_image.py",
    "build_onda_restore_ac_power_policy_image.py",
    "build_onda_quiet_final_image.py",
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"Cannot load build manifest {path}: {error}") from error
    if manifest.get("schema_version") != 1:
        raise ValueError("Unsupported build manifest schema")
    return manifest


def verify_file(root: Path, item: dict[str, Any]) -> None:
    path = root / Path(item["path"])
    if not path.is_file():
        raise FileNotFoundError(f"{item['id']}: {path}")
    size = path.stat().st_size
    if size != item["size"]:
        raise ValueError(
            f"{item['id']}: size {size}, expected {item['size']}"
        )
    actual = sha256(path)
    if actual != item["sha256"]:
        raise ValueError(
            f"{item['id']}: SHA-256 {actual}, expected {item['sha256']}"
        )


def preflight(manifest: dict[str, Any]) -> None:
    release = manifest["release"]
    for item in release["inputs"]:
        verify_file(ROOT, item)
        print(f"OK input {item['id']}")
    for raw_path in release["required_paths"]:
        path = Path(raw_path)
        if not path.is_file():
            raise FileNotFoundError(path)
        print(f"OK tool  {path}")
    for script in BUILD_STEPS:
        path = ROOT / "tools" / script
        if not path.is_file():
            raise FileNotFoundError(path)


def run_step(script: str, index: int, total: int) -> None:
    path = ROOT / "tools" / script
    print(f"[{index:02d}/{total:02d}] {script}", flush=True)
    process = subprocess.run(
        [sys.executable, str(path)],
        cwd=ROOT,
        check=False,
    )
    if process.returncode:
        raise RuntimeError(
            f"{script} failed with exit code {process.returncode}"
        )


def verify_output(manifest: dict[str, Any]) -> None:
    output = manifest["release"]["output"]
    verify_file(ROOT, {"id": "stage4gh-output", **output})
    print(f"Verified final image: {output['sha256']}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Rebuild the exact hardware-tested Stage 4GH image.",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help="Build input manifest",
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Verify all external inputs and tools without building",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    manifest = load_manifest(args.manifest.resolve())
    preflight(manifest)
    if args.check_only:
        print("Release build preflight passed.")
        return
    total = len(BUILD_STEPS)
    for index, script in enumerate(BUILD_STEPS, start=1):
        run_step(script, index, total)
    verify_output(manifest)


if __name__ == "__main__":
    main()
