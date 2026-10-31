# Nomadix FW-7582 OS and package requirements

This is the living manifest for a future `nomadix-platform` OpenWrt package.
It records hardware-verified requirements separately from items that still
need validation against the selected OpenWrt release and APK feeds.

A feed-ready implementation now lives in
[`nomadix-platform`](nomadix-platform). Its production default continuously
verifies full-speed PWM 255; the example dynamic curve remains opt-in until
its minimum safe duty is established under controlled Xeon load. Watchdog and
RAS are reported by the status command but are intentionally not enabled
until their target-kernel behavior is hardware-tested.

## Package split

Use two packages so the production appliance does not expose firmware-write
capabilities unnecessarily:

1. `nomadix-platform`
   - board initialization not yet performed by firmware;
   - UART setup;
   - temperature monitoring and fail-safe dynamic fan control;
   - service supervision and configuration.
2. `nomadix-platform-maintenance`
   - `flashrom` and in-system firmware access;
   - firmware backup, hash verification, and recovery tooling;
   - optional diagnostics.

Do not make the maintenance package a dependency of the runtime package.

## Hardware-verified platform facts

- Intel H61 LPC bridge: PCI `00:1f.0`, device `8086:1c5c`.
- Super-I/O configuration ports: `0x2e/0x2f`.
- Super-I/O ID/revision: `0xb0/0x73`, Winbond W83627DHG-compatible.
- COM1: LDN `0x02`, `0x3f8`, IRQ 4.
- COM2: LDN `0x03`, `0x2f8`, IRQ 3.
- Hardware monitor: LDN `0x0b`, base `0x290`.
- Hardware-monitor index/data ports: `0x295/0x296`.
- Stage 4C leaves PCH COM decode disabled (`LPC_EN=0x1400`).
- Enabling `LPC_EN` bits 0 and 1 makes COM1 accessible.
- Stage 2B uses:
  - `LPC_IO_DEC=0x0010`;
  - `LPC_EN=0x3f07`;
  - `GEN1_DEC=0x000c0291`;
  - HWM LDN `0x0b` active at `0x290`;
  - HWM LDN registers `F0=0x80`, `F2=0x80`.
- The Stage 4C runtime initializer safely uses free `GEN2_DEC` instead of
  replacing ONDA `GEN1_DEC`.
- PWM `0xff` produces fail-safe maximum fan speed.
- PWM `0x0a` is quiet and keeps the fan rotating, but is not thermally safe
  as a fixed unattended setting.

References:

- [`../tools/probe_superio_devport.sh`](../tools/probe_superio_devport.sh)
- [`../tools/dump_w83627_hwm.sh`](../tools/dump_w83627_hwm.sh)
- [`../tools/enable_w83627_stage4c.sh`](../tools/enable_w83627_stage4c.sh)
- [`../stage2b/lpc-stage2b.txt`](../stage2b/lpc-stage2b.txt)
- [`../stage2b/sio-stage2b.txt`](../stage2b/sio-stage2b.txt)
- [`../stage2b/w83627.txt`](../stage2b/w83627.txt)

## Runtime initialization

Until Stage 4F2 performs this in firmware, the package must initialize the
platform before serial and fan services start:

1. Enable the required PCH LPC decode.
2. Preserve existing ONDA generic decode windows.
3. Use the free generic decode window for `0x290-0x29f`.
4. Activate Winbond LDN `0x0b` at `0x290`.
5. Set HWM LDN registers `F0=0x80` and `F2=0x80`.
6. Start hardware monitoring.
7. Set all PWM outputs to fail-safe `0xff`.
8. Only reduce PWM after valid temperature inputs have been read.

The final package should use a small native helper rather than shell
byte-at-a-time `/dev/port` access. The helper must:

- verify the expected PCI and Super-I/O IDs before writing;
- preserve unrelated register bits and generic decode windows;
- report every initialization error;
- set PWM to `0xff` on startup, shutdown, invalid sensor data, and fatal error;
- be idempotent.

If Stage 4F2 is hardware-verified, the OS helper should detect that the
firmware already initialized LPC/HWM and avoid unnecessary reprogramming.

## Serial console

Required kernel capabilities:

- `CONFIG_SERIAL_8250=y` or equivalent module;
- `CONFIG_SERIAL_8250_CONSOLE=y`;
- support for legacy I/O-port UARTs.

Temporary pre-Stage-4F2 bootloader action:

```text
insmod setpci
setpci -s 00:1f.0 82.w=0003:0003
```

Recommended kernel command line:

```text
console=tty1 console=ttyS0,115200n8 earlycon=uart8250,io,0x3f8,115200 ignore_loglevel
```

Do not configure multiple `ttyS*` devices as simultaneous kernel consoles.

Runtime tools currently used:

- `setpci`, provided by the `pciutils` package;
- `setserial`, candidate package name `setserial`.

`setserial` should not be required if firmware, kernel probing, and device
publication are correct. Keep it as a diagnostic dependency until Stage 4F2
is tested.

## Temperature and fan control

Preferred implementation:

1. Load `coretemp`.
2. Load a W83627-compatible hwmon driver if it supports the detected chip and
   the initialized `0x290` hardware-monitor interface.
3. Run a supervised fan-control service with hysteresis.

Packages confirmed available in the target OpenWrt APK feed:

- `kmod-hwmon-coretemp-6.12.94-r1`;
- `kmod-hwmon-w83627ehf-6.12.94-r1`;
- `kmod-hwmon-w83627hf-6.12.94-r1`.

The target already contains:

```text
/lib/modules/6.12.94/w83627ehf.ko
```

After installing/loading the confirmed packages, hardware testing shows:

- `coretemp` binds and exports package plus four core temperatures;
- `w83627ehf` binds as `w83627dhg-isa-0000`;
- `lm-sensors-3.6.2-r6` is available and `sensors` reads the chip;
- no `fancontrol` package is present in the target feed.

Observed at the quiet `lanner` setting:

- all four PWM outputs report 4%, corresponding to raw duty `0x0a`;
- fan4 reports approximately 4115 RPM;
- fan1 reported 0 RPM during the quiet test because that fan was physically
  disconnected; after connection it reports approximately 16071 RPM at 100%;
- fan4 reports approximately 17307 RPM at 100%;
- fan1 through fan4 are valid board fan channels but may be physically
  disconnected during bench testing; fan5 is not used;
- Winbond temp1 and temp3 report approximately 36 C;
- Winbond temp2 reports invalid -128 C and must never be used for control;
- `coretemp` package temperature is approximately 49-50 C at the observed
  idle state.

The Winbond driver cannot initialize the Stage 4C hardware before platform
initialization. The observed ordering failure is:

```text
w83627ehf: Refusing to enable a Super-I/O device with a base I/O port 0
```

After running `enable_w83627_stage4c.sh`, a fresh driver probe reports:

```text
w83627ehf: Found W83627DHG-P chip at 0x290
```

Required pre-Stage-4F2 service order:

1. Do not autoload `w83627ehf`.
2. Program PCH LPC decode.
3. Activate Winbond HWM LDN `0x0b` at `0x290`.
4. Set fail-safe PWM `0xff`.
5. Load `w83627ehf`.
6. Verify that the `w83627dhg` hwmon device and fan4 tachometer appear.
7. Start `nomadix-fand`.

If an early module autoload already attempted and failed, platform init must
explicitly unload and reload `w83627ehf`, or otherwise trigger a fresh driver
probe. Stage 4F2 should make the HWM device valid before Linux starts and
remove this boot-order dependency.

Before freezing dependencies, collect:

```sh
apk search '*w83627*'
apk search '*coretemp*'
find /lib/modules -iname '*w83627*' -o -iname '*coretemp*'
modprobe coretemp
modprobe w83627ehf
find /sys/class/hwmon -maxdepth 2 -type f
```

The standard `w83627ehf` driver binds successfully, but the feed has no
`fancontrol` package. Provide a small `nomadix-fand` daemon that controls the
driver's sysfs PWM interfaces. Its minimum safety behavior:

- use `coretemp` as the control temperature source;
- require valid, fresh sensor readings;
- begin and fail at PWM `0xff`;
- enforce a safe minimum duty established by hardware testing;
- use hysteresis to avoid oscillation;
- supervise with `procd` respawn;
- restore PWM `0xff` on `SIGTERM`, normal exit, sensor loss, and internal
  error;
- log temperature, selected duty, sensor failures, and fail-safe transitions;
- never use fixed `0x0a` unattended or under load.

Hardware-autonomous fan control was evaluated using the Linux 6.12
`w83627ehf` documentation. The chip and driver expose Thermal Cruise through
`pwm1_enable` to `pwm4_enable`, but the temperature mapping is:

```text
temp1 -> pwm1
temp2 -> pwm2
temp3 -> pwm3
programmable source -> pwm4
```

The driver does not support configuring the programmable pwm4 source. On the
target, temp2 is invalid at -128 C, while the required policy is one shared
duty for pwm1 through pwm4 based on Linux `coretemp`. Therefore keep all four
outputs in manual mode (`pwm*_enable=1`) and let `nomadix-fand` write one
shared duty value.

W83627DHG-P supports PECI in principle. Enabling and validating its CPU
temperature input could make a future hardware-autonomous mode possible, but
the required source-selection/register programming is not exposed by this
driver and must remain a separate firmware/hardware experiment.

The temperature curve and minimum safe duty remain to be established with
the Xeon E3-1220 v2 under controlled load.

Initial conservative prototype:

- implementation: [`../tools/nomadix_fand.sh`](../tools/nomadix_fand.sh);
- SHA-256:
  `12f1e71a68b24e8b336eb2be047e38b0c96ce8bd64c34a3fedfe16af8b80557f`;
- sample interval: 2 seconds;
- control input: `coretemp` label `Package id 0`;
- informational tachometer inputs: Winbond `fan1_input` through `fan4_input`;
- outputs: Winbond `pwm1` through `pwm4`, manual mode, always set to one
  shared duty value;
- raw 10 through 45 C;
- raw 48 above 45 C;
- raw 80 from 50 C;
- raw 112 from 55 C;
- raw 160 from 60 C;
- raw 208 from 65 C;
- raw 240 from 70 C;
- 100%/raw 255 from 75 C;
- 2 C downward hysteresis;
- raw 255 on startup, CPU temperature failure, write failure, signal, or
  process exit.

Zero RPM does not trigger fail-safe because fans are intentionally connected
and disconnected during bench testing. Synthetic validation confirms all four
PWM outputs receive raw 10 together at 44 C despite zero tachometers, all four
receive raw 255 at 76 C, and all four return to raw 255 after process exit.
Controlled hardware testing is still required before production use.

## CPU microcode

Stage 4F1 updates CPUID `306A9` firmware microcode from revision `0x1c` to
`0x21`. The OS should still expose and log the active revision:

```sh
grep -m1 microcode /proc/cpuinfo
dmesg | grep -Ei 'microcode|mce|machine check|hardware error'
grep . /sys/devices/system/cpu/vulnerabilities/mds
grep . /sys/devices/system/cpu/vulnerabilities/srbds
```

An OS microcode package can be retained as defense in depth, but it must be
verified to load early enough for the chosen OpenWrt kernel. The target feed
provides `intel-microcode-20251111-r1`. If the firmware update is deployed,
the package is not required to reach revision `0x21`.

## Runtime package dependencies

Confirmed:

- `pciutils-3.14.0-r2` while PCH initialization uses `setpci`;
- `kmod-hwmon-coretemp-6.12.94-r1`;
- `kmod-hwmon-w83627ehf-6.12.94-r1`;
- `setserial-2.17-r4` while legacy serial diagnostics remain necessary;
- `lm-sensors-3.6.2-r6` for diagnostics.

Available but still optional or pending runtime validation:

- `intel-microcode-20251111-r1`.

There is no `fancontrol` package in the observed APK feed.

Avoid a runtime dependency on:

- `bash`: package scripts and init services should be POSIX `sh`;
- `od`: diagnostics already support BusyBox `hexdump`;
- `flashrom`: keep it in the maintenance package;
- `/dev/mem`: runtime LPC/SIO/fan handling should not require physical-memory
  access.

## Maintenance image/package

For `flashrom -p internal`, check:

```sh
zcat /proc/config.gz 2>/dev/null |
    grep -E 'CONFIG_(DEVMEM|STRICT_DEVMEM|IO_STRICT_DEVMEM)'
```

Requirements:

- `flashrom-1.3.0-r2`; the feed also provides matching `flashrom-pci`,
  `flashrom-spi`, and `flashrom-usb` subpackages;
- kernel `CONFIG_DEVMEM=y`;
- `/dev/mem` character device, major 1 minor 1;
- possibly kernel command line `iomem=relaxed`, depending on strict I/O-memory
  policy and the kernel version.

Observed on the target:

- `/dev/port` exists as character device major 1 minor 4;
- `/dev/mem`, `/dev/mtd*`, and `/dev/spidev*` were not shown;
- `/proc/mtd` does not exist;
- neither `/proc/config.gz` nor `/boot/config-*` is exposed;
- reading `/proc/config.gz` produced no output, so `CONFIG_DEVMEM` status is
  still unknown rather than confirmed disabled.

If `CONFIG_DEVMEM=y` but the device node is absent:

```sh
mknod /dev/mem c 1 1
chmod 600 /dev/mem
```

`CONFIG_STRICT_DEVMEM` and `CONFIG_IO_STRICT_DEVMEM` may still prevent
internal flashing. Test with probe/read operations before any write.

Security policy:

- do not enable `iomem=relaxed` in the normal production image;
- do not create a generally accessible `/dev/mem`;
- do not install the maintenance package by default;
- use a separate maintenance boot entry or image where practical;
- always read a backup and verify hashes before writing;
- do not reboot until flash verification succeeds.

If `CONFIG_DEVMEM` is disabled, use an external programmer, a dedicated
maintenance kernel, or another trusted live environment. Do not implement
ad-hoc SPI programming through `/dev/port`.

## Network configuration

The board exposes six Intel `e1000e` interfaces. Interface enumeration under
the ONDA stack may differ from the original Lanner firmware. The package may
provide a board-specific mapping helper, but it must not install arbitrary
production IP addresses or firewall policy by default.

Before finalizing defaults, record permanent MAC address to physical-port
mapping for `eth0` through `eth5`.

## RAS, machine checks, and ECC

The future OS should include persistent hardware-error monitoring. Prefer
`rasdaemon` on the 6.12 kernel; retain `mcelog` only if the target feed
provides it and it adds useful Ivy Bridge compatibility not covered by
`rasdaemon`.

Requested user-space components:

- `rasdaemon`, including persistent event storage if its package supports it;
- `mcelog` as an optional diagnostic/compatibility tool;
- EDAC utilities if packaged separately.

Candidate kernel capabilities and modules, to verify:

- `CONFIG_RAS`;
- `CONFIG_MCE` and `CONFIG_X86_MCE`;
- `CONFIG_TRACEPOINTS`, `CONFIG_TRACING`, and mounted tracefs for `rasdaemon`;
- `CONFIG_EDAC`;
- `CONFIG_EDAC_LEGACY_SYSFS`;
- `CONFIG_EDAC_DECODE_MCE`;
- `edac_core`;
- `ie31200_edac` or the matching Ivy Bridge Xeon E3 memory-controller
  driver exposed by the target kernel.

The observed OpenWrt image contains no EDAC module, so absence of `mc0` is
currently inconclusive. x86 machine-check support may be built into the
kernel and therefore have no `.ko` file. Verify tracepoints and sysfs before
concluding that MCE support is absent.

Observed ECC test hardware:

- host bridge `8086:0158`, identified as Xeon E3-1200 v2/Ivy Bridge DRAM
  Controller;
- 8 GB Micron `18KSF1G72AZ-1G6P1`, dual-rank DDR3-1600 ECC UDIMM;
- SMBIOS Type 16 reports `Error Correction Type: None`;
- SMBIOS Type 17 reports inconsistent `Total Width: 128 bits` and
  `Data Width: 64 bits`, so ONDA SMBIOS cannot be treated as authoritative;
- no EDAC module, machinecheck sysfs directory, or MCE tracefs event directory
  is present in the current OpenWrt image.

Linux 6.12 `ie31200_edac` explicitly supports PCI device `8086:0158`. Its
initial capability test reads byte `CAPID0+3`, PCI config offset `0xe7`, and
rejects the controller when bit 1 is set. A read-only check is:

```sh
setpci -s 00:00.0 e4.l
setpci -s 00:00.0 e7.b
```

Interpret `e7.b`:

- `(value & 0x02) == 0`: the Linux driver considers the controller
  ECC-capable;
- `(value & 0x02) != 0`: the Linux driver reports `No ECC support`.

This capability bit is stronger evidence than the broken SMBIOS width, but
the final proof still requires a loaded `ie31200_edac` driver and active EDAC
controller/counters.

Observed under Stage 4C with the Xeon E3-1220 v2 and Micron ECC UDIMM:

```text
CAPID0 0xe4.l = e200ea92
CAPID0 0xe7.b = e2
e2 & 02 = 02
```

Therefore Linux 6.12 `ie31200_edac` would reject the current controller state
with `No ECC support`. The installed ECC UDIMM is operating as ordinary
64-bit data memory in this boot configuration; merely adding the EDAC module
would not enable ECC.

Do not attempt to clear CAPID0 with `setpci`. Treat it as a capability/status
result, not a runtime tuning register. Establishing whether a different
firmware/MRC/PCH combination can expose ECC would require a separate firmware
investigation and comparison of the same register under that stack.

RAS monitoring remains useful for CPU machine checks even without memory
EDAC. Package reporting must distinguish CPU/MCE coverage from active memory
ECC and must not claim ECC protection when no EDAC memory controller exists.

The Intel Xeon E3-1220 v2 supports ECC memory, but that does not by itself
prove platform ECC operation. The Lanner DIMM wiring, host bridge, selected
UDIMMs, and ONDA MRC/firmware memory policy must all support and enable ECC.
In particular, Stage 4C uses ONDA memory initialization rather than the
original Lanner memory policy.

Do not treat any of these as sufficient proof of active ECC:

- the system boots with ECC UDIMMs;
- SMBIOS labels a module as ECC;
- `dmidecode` reports a non-zero total width;
- `rasdaemon` or `mcelog` starts without an error.

Require these runtime facts before claiming ECC is active:

- a supported EDAC memory controller appears under
  `/sys/devices/system/edac/mc`;
- the driver reports ECC-enabled mode for the active memory controller;
- corrected and uncorrected error counters are readable;
- boot logs contain no indication that ECC was requested but disabled;
- DIMM width/type data agrees with the physical ECC UDIMMs.

Target evidence to collect:

```sh
apk search rasdaemon
apk search mcelog
apk search '*edac*'
find /lib/modules -iname '*edac*' -o -iname '*ie31200*'
lsmod | grep -Ei 'edac|mce'
dmesg | grep -Ei 'edac|ecc|mce|machine check|hardware error'
ls -la /sys/devices/system/edac/mc 2>/dev/null
find /sys/devices/system/edac/mc -maxdepth 3 -type f \
    \( -name 'ce_count' -o -name 'ue_count' -o -name 'size_mb' \
       -o -name 'dimm_mem_type' -o -name 'dimm_edac_mode' \) \
    -print -exec cat {} \; 2>/dev/null
dmidecode -t 16 -t 17
lspci -nn -s 00:00.0
```

After installing ECC UDIMMs, compare those results against the current
non-ECC baseline. Do not deliberately inject memory errors on the production
system unless a separately reviewed, reversible test procedure is prepared.

Packaging policy:

- run `rasdaemon` as the primary supervised RAS service;
- persist and rotate its event database/logs;
- expose a package status command summarizing MCE, corrected ECC, and
  uncorrected ECC counts;
- generate a prominent system log entry for every new corrected error;
- trigger an administrator-visible critical alert for any uncorrected error;
- do not silently clear EDAC counters or the `rasdaemon` event database;
- avoid running two competing services that both attempt to consume the same
  machine-check event source.

## Packaging requirements

The eventual package should contain:

- an APK/OpenWrt package manifest with explicit runtime dependencies;
- a native LPC/Super-I/O helper if Stage 4F2 is not sufficient;
- a `procd`-managed fan daemon or a validated hwmon/fancontrol configuration;
- `/etc/config/nomadix-platform`;
- an init service with explicit ordering;
- a status/diagnostic command;
- a supervised RAS service and persistent hardware-error reporting;
- log messages for every degraded or fail-safe state;
- no CRLF shell scripts;
- uninstall behavior that leaves fans at `0xff`;
- upgrade behavior that never temporarily selects low fixed PWM.

## Evidence still needed from the target OS

Still capture:

```sh
uname -r
ls -l /proc/config.gz /boot/config-* 2>/dev/null
grep -E 'CONFIG_(DEVMEM|STRICT_DEVMEM|IO_STRICT_DEVMEM|SERIAL_8250|MICROCODE|SENSORS_CORETEMP)' \
    /boot/config-* 2>/dev/null
cat /proc/mtd
ls -l /dev/mem /dev/port /dev/mtd* /dev/spidev* 2>/dev/null
apk info -e kmod-hwmon-w83627ehf kmod-hwmon-coretemp
apk search lm-sensors
apk search fancontrol
```
