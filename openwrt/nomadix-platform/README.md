# Nomadix FW-7582 OpenWrt package

This directory is an OpenWrt feed source. It builds two packages:

- `nomadix-platform`: fail-safe fan control and platform diagnostics;
- `nomadix-platform-maintenance`: optional read-only SPI flash backup tooling.

The runtime package never writes the SPI flash or Intel NIC NVM.

## Add the feed

Add the repository's `openwrt` directory to the OpenWrt build tree's
`feeds.conf` using a Linux path:

```text
src-link nomadix /absolute/path/to/nomadix/openwrt
```

Then run:

```sh
./scripts/feeds update nomadix
./scripts/feeds install -a -p nomadix
make menuconfig
```

Select `Utilities -> nomadix-platform`. Select
`nomadix-platform-maintenance` only for a dedicated maintenance image.

Build only these packages with:

```sh
make package/feeds/nomadix/nomadix-platform/compile V=s
```

## Safe default

The installed UCI configuration defaults to:

```text
option mode 'full'
```

This keeps all four PWM channels at 255. The example temperature curve is
present but is not activated automatically because its minimum safe duty has
not yet been established under controlled Xeon E3-1220 v2 load.

After completing a thermal load test, enable dynamic control with:

```sh
uci set nomadix-platform.main.mode='curve'
uci commit nomadix-platform
/etc/init.d/nomadix-platform restart
```

The daemon:

- verifies the FW-7582 H61 LPC bridge and all six Intel 82583V PCI functions
  by default;
- keeps full-speed mode operational without a `coretemp` sensor and
  periodically verifies that all PWM outputs remain at 255;
- requires the W83627 hwmon device in every mode and `coretemp` package
  temperature only in dynamic curve mode;
- selects manual mode on PWM1 through PWM4;
- applies and verifies PWM 255 before reading temperature;
- verifies every PWM write through sysfs;
- returns every channel to PWM 255 on sensor loss, write failure, signal,
  normal exit, package removal, or service restart;
- is supervised and automatically respawned by `procd`.

Use `nomadix-platform-status` to inspect the daemon, temperatures, PWM,
tachometers, microcode, watchdog, NIC mapping, and EDAC visibility.

## Configuration

`/etc/config/nomadix-platform` supports:

- `enabled`: start the service;
- `mode`: `full` or `curve`;
- `verify_platform`: reject an unexpected LPC bridge unless explicitly
  disabled;
- `interval`: sensor period in seconds;
- `hysteresis`: downward hysteresis in millidegrees Celsius;
- `curve`: comma-separated `temperature_mC:PWM` points in ascending order.

Every curve must begin at zero, PWM duties must not decrease, and the final
point must use fail-safe PWM 255. PWM values are from 10 through 255; lower
values are rejected because they are below the lowest observed rotating-fan
setting.

## Maintenance package

`nomadix-flash-readback OUTPUT.bin` performs two read-only internal flashrom
backups, requires two byte-identical 8 MiB results, restricts the retained
output permissions, and prints its SHA-256 hash. It refuses to overwrite an
existing path.

The command can still fail when the production kernel does not expose
`/dev/mem` or enforces strict memory access. Do not weaken the production
kernel with `iomem=relaxed`; use an external SPI programmer or a separate
maintenance image instead.

## Deliberately not configured automatically

The package reports watchdog and EDAC visibility but does not enable them:

- the hardware watchdog reset path must be tested on the final OpenWrt kernel
  before `procd` is allowed to rely on it;
- ECC is not active in the observed ONDA MRC configuration;
- `rasdaemon` availability and its required kernel tracepoints are still
  unverified in the selected feed.

These are explicit follow-up validation items rather than success-shaped
fallbacks in the runtime package.
