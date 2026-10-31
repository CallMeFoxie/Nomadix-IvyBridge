from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "analysis" / "python-libs"))

import capstone  # type: ignore[import-not-found]
import pefile  # type: ignore[import-not-found]


INTERESTING_PORTS = {
    0x60: "KBC data",
    0x61: "speaker/NMI",
    0x64: "KBC command/status",
    0x70: "CMOS index/NMI",
    0x71: "CMOS data",
    0x80: "POST checkpoint",
    0x92: "Fast A20/reset",
    0x42: "PIT channel 2",
    0x43: "PIT control",
    0xCF9: "reset control",
}


def instruction_port(instruction: capstone.CsInsn) -> int | None:
    if instruction.mnemonic not in {"in", "out"}:
        return None
    for operand in instruction.operands:
        if operand.type == capstone.x86.X86_OP_IMM:
            return operand.imm & 0xFFFF
    return None


def analyze(path: Path) -> None:
    pe = pefile.PE(str(path), fast_load=False)
    machine = pe.FILE_HEADER.Machine
    mode = capstone.CS_MODE_32 if machine == 0x14C else capstone.CS_MODE_64
    decoder = capstone.Cs(capstone.CS_ARCH_X86, mode)
    decoder.detail = True
    image_base = pe.OPTIONAL_HEADER.ImageBase
    entry = image_base + pe.OPTIONAL_HEADER.AddressOfEntryPoint

    print(f"\n## {path.parent.name}")
    print(
        f"machine=0x{machine:04X} image_base=0x{image_base:X} "
        f"entry=0x{entry:X} sections={len(pe.sections)}"
    )

    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        section_name = section.Name.rstrip(b"\0").decode(errors="replace")
        code = section.get_data()
        address = image_base + section.VirtualAddress
        instructions = list(decoder.disasm(code, address))
        print(
            f"section={section_name} address=0x{address:X} "
            f"raw=0x{len(code):X} instructions={len(instructions)}"
        )
        for index, instruction in enumerate(instructions):
            port = instruction_port(instruction)
            context_port: int | None = port
            if instruction.mnemonic in {"in", "out"} and port is None:
                for previous in reversed(instructions[max(0, index - 6) : index]):
                    if (
                        previous.mnemonic == "mov"
                        and previous.operands[0].type == capstone.x86.X86_OP_REG
                        and previous.reg_name(previous.operands[0].reg) == "dx"
                        and previous.operands[1].type == capstone.x86.X86_OP_IMM
                    ):
                        context_port = previous.operands[1].imm & 0xFFFF
                        break
            if context_port in INTERESTING_PORTS:
                start = max(0, index - 4)
                end = min(len(instructions), index + 5)
                print(
                    f"  I/O 0x{context_port:X} ({INTERESTING_PORTS[context_port]}) "
                    f"at 0x{instruction.address:X}"
                )
                for nearby in instructions[start:end]:
                    marker = ">" if nearby.address == instruction.address else " "
                    print(
                        f"   {marker} {nearby.address:08X}: "
                        f"{nearby.mnemonic:8} {nearby.op_str}"
                    )

    raw = path.read_bytes()
    strings = [
        match.group().decode("ascii", errors="replace")
        for match in re.finditer(rb"[ -~]{6,}", raw)
    ]
    interesting = [
        value
        for value in strings
        if re.search(
            r"beep|speaker|watchdog|reset|memory|dimm|spd|keyboard|a20|error",
            value,
            re.IGNORECASE,
        )
    ]
    for value in interesting[:50]:
        print(f"  string: {value}")


def main() -> None:
    if len(sys.argv) < 2:
        raise SystemExit("Usage: analyze_pei_io.py <body.bin> [...]")
    for argument in sys.argv[1:]:
        analyze(Path(argument))


if __name__ == "__main__":
    main()
