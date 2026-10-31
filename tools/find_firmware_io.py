from __future__ import annotations

import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis" / "python-libs"))

import capstone  # type: ignore[import-not-found]
import pefile  # type: ignore[import-not-found]


PORTS = {
    0x2E: "Super I/O index",
    0x2F: "Super I/O data",
    0x42: "PIT channel 2",
    0x43: "PIT control",
    0x4E: "alternate Super I/O index",
    0x4F: "alternate Super I/O data",
    0x60: "KBC data",
    0x61: "speaker/NMI",
    0x64: "KBC command/status",
    0x70: "CMOS index/NMI",
    0x71: "CMOS data",
    0x80: "POST checkpoint",
    0x92: "Fast A20/reset",
    0x2E8: "COM4",
    0x2F8: "COM2",
    0x3E8: "COM3",
    0x3F8: "COM1",
    0xCF9: "reset control",
}


def scan(path: Path) -> dict[int, list[int]]:
    try:
        if path.read_bytes()[:2] != b"MZ":
            return {}
        pe = pefile.PE(str(path), fast_load=False)
    except (OSError, pefile.PEFormatError):
        return {}
    mode = capstone.CS_MODE_32 if pe.FILE_HEADER.Machine == 0x14C else capstone.CS_MODE_64
    decoder = capstone.Cs(capstone.CS_ARCH_X86, mode)
    decoder.detail = True
    found: dict[int, list[int]] = defaultdict(list)
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        address = pe.OPTIONAL_HEADER.ImageBase + section.VirtualAddress
        instructions = list(decoder.disasm(section.get_data(), address))
        for index, instruction in enumerate(instructions):
            if instruction.mnemonic not in {"in", "out"}:
                continue
            port: int | None = None
            for operand in instruction.operands:
                if operand.type == capstone.x86.X86_OP_IMM:
                    port = operand.imm & 0xFFFF
                    break
            if port is None:
                for previous in reversed(instructions[max(0, index - 8) : index]):
                    if (
                        previous.mnemonic == "mov"
                        and previous.operands[0].type == capstone.x86.X86_OP_REG
                        and previous.reg_name(previous.operands[0].reg) in {"dx", "edx"}
                        and previous.operands[1].type == capstone.x86.X86_OP_IMM
                    ):
                        port = previous.operands[1].imm & 0xFFFF
                        break
            if port in PORTS:
                found[port].append(instruction.address)
    return found


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: find_firmware_io.py <UEFIExtract dump directory>")
    dump = Path(sys.argv[1]).resolve()
    scanned = 0
    matched = 0
    for path in dump.rglob("body.bin"):
        result = scan(path)
        if path.read_bytes()[:2] == b"MZ":
            scanned += 1
        if not result:
            continue
        matched += 1
        relative = path.relative_to(dump)
        print(f"\n{relative}")
        for port, addresses in sorted(result.items()):
            rendered = ", ".join(f"0x{address:X}" for address in addresses[:20])
            print(f"  0x{port:X} {PORTS[port]}: {rendered}")
    print(f"\nScanned PE images: {scanned}; matches: {matched}")


if __name__ == "__main__":
    main()
