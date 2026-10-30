# Building and porting Nomadix firmware

This guide distinguishes three different operations:

1. building the verified ME8 Stage 1 image from locally supplied inputs;
2. rebuilding the complete Stage 4C-to-4GH release chain from the mandatory
   post-Stage1 hardware read-back;
3. using the same methodology for a different motherboard.

They are not interchangeable. The scripts contain deliberate hash, address,
GUID, chipset, and hardware checks and should fail rather than silently produce
an image for an unknown platform.

## Safety requirements

Before building or testing any image, have:

- at least two identical full-chip reads from the target board;
- a known-working 3.3 V external SPI programmer;
- a tested recovery procedure;
- a Sandy Bridge CPU known to work with the original firmware;
- serial POST capture where available;
- a way to verify the complete chip immediately after writing.

Never use a partial-region command on a stock board when the intended image
also changes the Intel ME region. Never copy a flash descriptor, GbE region,
NVRAM, UUID, serial number, or MAC address from an unrelated machine without
understanding the consequences.

## Reproducibility boundary

The release is reproducible in two automated phases separated by one real
hardware operation:

```text
original FW-7582 dump + ASRock donor + FITC
    -> deterministic Stage 1 ME8 image
    -> flash, first boot, external read-back
    -> deterministic 22-step Stage 4GH build
```

The first ME8 boot initializes Intel ME runtime data and changes active NVRAM.
Those bytes are hardware state, not build output. The resulting
`post-stage1-boot-readback.bin` is therefore a required machine-specific input
to the release phase.

All required paths, sizes, and hashes are defined in
[build-inputs.json](build-inputs.json).

## Building Stage 1 ME8

Required inputs for the verified FW-7582:

```text
lanner-read1.bin
SHA-256 F0231B4E134E50CC7290B8BF8FBE3AAB8FA1482289816497321E427D8B3A5120

ref bios\ASRock H61M 2.00
SHA-256 899958FE4A32745B566379393DC9B5138E56FE448760091234DF0FD1606685D2

Intel FITC 8.1.40.1456 fitc.exe
SHA-256 D78F66E70949C33C7F6E589F3E13596C0AF7AA808CE8E3819A5A784CD3A4EF3E
```

Run:

```powershell
py -3 tools\build_lanner_me8_stage1.py
```

The builder decomposes the pinned ASRock donor with FITC and generates the
Lanner configuration programmatically. Relative to the ASRock FITC export it
changes:

- flash component 1 density to 8 MiB;
- BIOS region size to 4 MiB;
- BIOS input to the original Lanner BIOS region;
- `LAN Power Well Config` from `0` to the Lanner value `3`.

It then restores the original Lanner descriptor byte-for-byte and verifies
that the original BIOS region is unchanged.

Expected output:

```text
output\TEST_ONLY_stage1_livebase_me8.bin
SHA-256 782B5713D7DC40E54BA75184379D5A0B03C7CD602362C8C8343B5E47A5BC176A
```

Flash only the ME region as documented in the staged recovery notes. Boot once
with the original supported Sandy Bridge CPU, shut down, and take a complete
external read-back:

```text
post-stage1-boot-readback.bin
SHA-256 DD6BDB258D0F9DB0457B5E522402873C953ACFA3DC70CA58E58FE34DFB3F127B
```

That hash is specific to the verified development appliance. Another physical
board is expected to have different NVRAM and may have different initialized
ME runtime bytes; do not force its dump to match this machine by copying
identity data.

## Rebuilding the verified Stage 4GH output

The release orchestrator starts from the pinned post-Stage1 read-back, creates
Stage 4C directly from the ONDA donor, and runs the exact verified Stage
4F1-to-4GH lineage.

### Required environment

- 64-bit Windows;
- Python 3;
- Visual Studio 2019 Community with the C/C++ x64 build tools at:

  ```text
  C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat
  ```

- UEFIExtract A76 at:

  ```text
  tools\UEFIExtract_NE_A76_win64\UEFIExtract.exe
  ```

- UEFIReplace at:

  ```text
  tools\UEFIReplace.exe
  ```

- the exact locally obtained post-Stage1 hardware read-back:

  ```text
  post-stage1-boot-readback.bin
  SHA-256 DD6BDB258D0F9DB0457B5E522402873C953ACFA3DC70CA58E58FE34DFB3F127B
  ```

Generated images, donor firmware, Microsoft build products, and downloaded
UEFI utilities are excluded from Git and must be supplied locally.

### Command

Verify all external inputs first:

```powershell
py -3 tools\build_release.py --check-only
```

Then build the release:

```powershell
py -3 tools\build_release.py
```

The orchestrator runs 22 checked builders. It first creates the exact Stage 4C
base directly from the post-Stage1 read-back and ONDA donor. This direct
builder reproduces SHA-256
`8523A25786DA11675E5DE769E77DFE2347DFEC818E6E83D4E4737068B79FB4B7`
without the historical Angxun X7-VB Stage 2 chain.

Every DXE builder compiles the freestanding x86-64 EFI driver twice, requires
byte-identical output, enables `/W4 /WX`, checks PE32+, relocations, imports,
dependencies, protected regions, microcode identity, extracted driver bytes,
and its exact predecessor hash.

Expected outputs:

```text
output\onda-exact\TEST_ONLY_stage4gh_quiet-final.bin
output\onda-exact\TEST_ONLY_stage4gh_quiet-final.efi
output\onda-exact\TEST_ONLY_stage4gh_quiet-final.txt
output\onda-exact\TEST_ONLY_stage4gh_quiet-final.bin.report.txt
```

Expected final hashes:

```text
Full 8 MiB image:
3D843DADDBB95664D51341B76BA8AD8E707F26FC2D48747AEAA61A895AA0FB42

Nomadix EFI driver:
7A93BE59567E839A5C46D883228EF20696FCBB832897DFEC7101F1DE7F17A640

Boot stack, 0x420000-0x7FFFFF:
5650F9F79998B3690FFDE568E9181242132628B242AAFDA71B06F4ECB490D0FB
```

The `.efi` output is the embedded DXE driver, not a complete flash image.

### Why the exact release chain is required

Do not skip from Stage 4F1 directly to Stage 4GH. Although the cumulative
source compiles, repeated section replacements in the verified release lineage
affect FFS placement. A direct experimental 4F1-to-4GH replacement produced a
different full-image hash and a different microcode FFS offset. That layout
has not been hardware-tested.

Reproducibility here means recreating the verified bytes, not merely compiling
functionally similar source.

## Donor inputs for a complete FW-7582 reproduction

| Input | Purpose |
|---|---|
| Two matching 8 MiB reads from the target FW-7582 | Descriptor, machine-specific NVRAM, board identity, original ME configuration, and recovery image. |
| [ASRock H61M-ITX](https://theretroweb.com/motherboards/s/asrock-h61m-itx) BIOS 2.00 | Source lineage for Intel ME 8.0.13.1502, 1.5 MiB Consumer. The ASRock BIOS is not directly flashed to the Lanner. |
| [ONDA H61 ITX Ver 1.0](https://theretroweb.com/motherboards/s/onda-h61-itx-ver-1-0) | Matched Ivy Bridge SEC/PEI/DXE/Setup/ACPI donor. |
| Official [Intel processor microcode data files](https://github.com/intel/Intel-Linux-Processor-Microcode-Data-Files) | CPUID `306A9`, revision `0x21`, file `intel-ucode/06-3a-09`. |
| Intel FITC from the correct ME8 System Tools generation | Rebuilding the ME8 region with target-board settings. This proprietary utility is not included. |
| UEFIExtract and UEFIReplace | Firmware inspection and checked FFS section replacement. |

Pinned public donor hashes used by this project:

```text
ONDA H61ITX01.BIN, 4 MiB:
5F5E0AB78A2D736F443940F6691C0BA5EACD2D6DADAAC1A5284B5B3F32E20E72

Intel intel-ucode/06-3a-09:
CA2FA408851D20E4F9993F5A3B60D41F17A6AE3B8C4BFA0B730FD162ABAEA885
```

The release path no longer uses the experimental
[Angxun ANGTHUMB XLZ X7-VB V2.0](https://theretroweb.com/motherboards/s/angxun-angthumb-xlz-x7-vb-v2-0-h61)
Stage 2A-2H images. The donor contains `X7VB1001.rom`, while this particular
BIOS build identifies itself through SMBIOS as
`AISURIX H61 Motherboard V2.0`, consistent with a rebranded or later firmware
identity. Stage 4C erases `0x420000-0x57FFFF`, which contains the only X7
change that survived into the old Stage 3 base. The new release-base builder
directly constructs the same verified Stage 4C bytes from the post-Stage1
Lanner image and ONDA donor.

The historical scripts remain available as research evidence, but
`tools/build_release.py` is the authoritative release sequence. Missing inputs
or a hash mismatch must be investigated, not bypassed by editing out
validation.

## Porting the method to another motherboard

Stage 4GH is not a generic H61 Ivy Bridge update. It contains assumptions
specific to the FW-7582 and the selected ONDA donor. A different board needs a
new port even when it uses the same socket and chipset.

### 1. Establish whether the platform is a plausible candidate

Record at minimum:

- CPU socket, PCH model, revision, and PCI IDs;
- original Intel ME generation, SKU, region size, and firmware status;
- SPI flash size and descriptor region map;
- memory topology, DIMM count, SPD wiring, and maximum fitted memory;
- Super-I/O vendor, device ID, revision, configuration ports, and logical
  devices;
- UART, GPIO, hardware-monitor, watchdog, LPT, and front-panel wiring;
- onboard NIC, storage, PCIe, display, and optional devices;
- ACPI power behavior and Setup variable layout.

An H61 label alone is insufficient. Ivy MRC must understand the board's memory
topology, the ME firmware must match the chipset configuration, and early PEI
must not depend on donor-only GPIOs, EC firmware, or power sequencing.

### 2. Capture a trustworthy baseline

Make at least two full-chip reads and compare them. Preserve:

- the complete untouched dump;
- a UEFITool/UEFIExtract report;
- Intel ME Analyzer output;
- FITC configuration export where legally and technically possible;
- DMI, PCI inventory, ACPI tables, kernel log, and Super-I/O register dump;
- photographs or schematics identifying the flash chip and recovery points.

Separate static firmware differences from ME runtime and NVRAM changes before
declaring two reads inconsistent.

### 3. Choose donors by subsystem, not by brand

Prefer a donor with the same:

- PCH family and stepping;
- ME generation, SKU, and region size;
- Aptio generation and boot architecture;
- CPU generation support;
- memory topology;
- firmware volume physical addresses and DXE IPL contract.

Keep SEC, PEI core, MemoryInit/MRC, CPU/PCH/SA PEIMs, DXE IPL, and the DXE
environment as a matched set whenever possible. The FW-7582 work only became
reliable after abandoning arbitrary individual PEIM mixtures and using the
matched ONDA boot stack.

### 4. Treat Intel ME migration as a board-specific operation

Do not copy the ASRock ME region blindly. Compare and deliberately migrate:

- ICC/clock configuration;
- chipset and SKU settings;
- flash region sizes and descriptor permissions;
- LAN power-well and integrated-LAN configuration;
- board straps and feature settings;
- HECI/UMA behavior expected by the selected PEI stack.

First test a rebuilt ME image with the original supported CPU. Verify ME status,
cold boot, reboot, power removal, network behavior, and absence of recovery
mode before changing PEI.

### 5. Map the firmware volumes exactly

For each FV record:

- flash offset and physical address;
- volume length, block map, attributes, and header checksum;
- file order and free space;
- volume-top SEC placement;
- nested FV relationships;
- microcode copies;
- the DXE IPL method used to discover and load DXE Core.

Moving a valid PEI FV to a different physical address can still break
MemoryInit, SEC handoff, or hard-coded donor assumptions.

### 6. Adapt or replace board-specific DXE behavior

The Nomadix driver is cumulative, but these values are not portable:

- expected PCH LPC PCI ID;
- LPC generic-decode register allocation;
- Super-I/O unlock sequence, ID, logical-device numbers, bases, and IRQs;
- HWM register banks, fan count, tachometer divisors, and PWM mode;
- safe initial PWM values;
- UART bases and IRQs;
- LPT/PLCM protocol and GPIO polarity;
- Setup variable GUID, size, offsets, and value meanings;
- PCH power-management register offsets and bit polarity;
- SMBIOS strings and board identity;
- target FFS GUID selected for PE32 replacement.

Start with diagnostics enabled. Refuse to touch hardware if identity or
read-back checks fail. Fan initialization must fail safe to full speed.

### 7. Use small, recoverable hardware stages

A recommended progression is:

1. new ME with original CPU and original BIOS;
2. matched donor SEC/PEI with original or minimal DXE;
3. matched full boot stack;
4. official CPU microcode;
5. serial diagnostic marker;
6. LPC and Super-I/O initialization;
7. serial EFI protocols and Setup interaction;
8. hardware monitor read-only display;
9. fan writes with read-back and fail-safe;
10. NVRAM policy;
11. optional front-panel devices and cosmetic branding;
12. quiet build only after the diagnostic build passes.

Give every stage a unique image hash and record the exact regions it changes.
Keep a known-good recovery image and the programmer connected only while board
power is safely isolated.

### 8. Validate the resulting platform, not only POST

Test:

- repeated cold boots and warm reboots;
- every supported DIMM slot and representative memory sizes;
- original Sandy Bridge and intended Ivy Bridge CPUs;
- all cores, CPU frequency states, virtualization, and microcode revision;
- ME normal mode and HECI communication;
- every NIC, storage controller, USB port, and PCIe slot;
- temperatures, tachometers, every fan channel, and fan fail-safe;
- serial console and Setup save/load behavior;
- NVRAM persistence and reset-to-default behavior;
- ACPI tables, kernel resource warnings, shutdown, and power button;
- AC removal/restoration for every exposed Setup policy;
- watchdog and long-duration idle/load operation.

A successful POST is evidence of progress, not proof that a donor stack is
safe for unattended use.

## What would be needed for a generic builder

A future generalized tool should accept an explicit per-board manifest rather
than changing constants in place. Such a manifest would need:

- input paths and hashes;
- flash-region and FV maps;
- donor-to-target copy ranges;
- protected machine-specific ranges;
- expected PCH and Super-I/O identities;
- serial, HWM, fan, LPT, and GPIO configuration;
- Setup-variable mappings;
- target FFS GUID and dependency expectations;
- output invariants and region hashes.

Until those inputs and validations exist, manually removing hash checks or
running the FW-7582 scripts against another dump is not a supported porting
method.
