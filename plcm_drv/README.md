# plcm_drv

Lanner Parallel LCM driver for the FW-8758 and similar x86 appliances.

The driver uses the legacy PC parallel-port I/O addresses directly. It is
therefore limited to x86 targets with the LCM enabled in firmware. The module
creates `/dev/plcm_drv` through the misc-device subsystem.

## OpenWrt

This repository is an OpenWrt package feed. In a fresh SDK, create
`feeds.conf` from the supplied defaults and append this feed:

```sh
cd /path/to/openwrt-sdk
test -f feeds.conf || cp feeds.conf.default feeds.conf
grep -qxF 'src-git plcm_drv https://github.com/CallMeFoxie/openwrt-plcm_drv.git' feeds.conf || \
	echo 'src-git plcm_drv https://github.com/CallMeFoxie/openwrt-plcm_drv.git' >> feeds.conf
./scripts/feeds update -a
./scripts/feeds install -a -p plcm_drv
```

Keep the standard `base`, `packages`, `luci`, and other entries from
`feeds.conf.default`; a custom `feeds.conf` replaces, rather than extends, the
defaults. There must be only one entry named `plcm_drv`.

The `src-git` entry builds the latest commit pushed to GitHub. To build
uncommitted local changes, replace it with `src-link` instead:

```sh
sed -i '/^src-.* plcm_drv /d' feeds.conf
echo 'src-link plcm_drv /path/to/plcm_drv' >> feeds.conf
./scripts/feeds update plcm_drv
./scripts/feeds install -a -p plcm_drv
```

Enable `Kernel modules > Other modules > kmod-plcm-drv`,
`Utilities > plcm-status`, and `LuCI > Applications > luci-app-plcm-status`,
then build them. The LuCI application expects LuCI to be installed on the
target system.

```sh
make menuconfig
make package/feeds/plcm_drv/plcm-drv/compile V=s
make package/feeds/plcm_drv/luci-app-plcm-status/compile V=s
```

The packages are written below `bin/targets/<target>/<subtarget>/packages/`
and `bin/packages/<architecture>/plcm_drv/`. Build and install the kernel
module against the exact OpenWrt release and target kernel used by the router;
kernel modules from a different SDK generally cannot be installed.

The resulting packages provide:

- `kmod-plcm-drv`: installs `plcm_drv.ko`, loads it at boot, and creates
	`/dev/plcm_drv`.
- `plcm-status`: renders configurable rotating pages and runs under procd.
- `luci-app-plcm-status`: adds the page editor under **Services > PLCM Display**.

Pages contain left and right slots on both display lines. One configured slot
uses all 20 characters; when both slots are configured, each receives 10.
Pages can be reordered in LuCI. Available widgets are static text, hostname,
clock, date, uptime, CPU, RAM, load, IPv4 address, RX rate, and TX rate.

The network widgets use their argument as the Linux interface name. Static text
uses it as the displayed value, while clock and date accept an optional
`strftime` format. The equivalent UCI configuration is:

```sh
uci set plcm-status.main.refresh='1'
uci set plcm-status.@page[1].line1_left='ipv4'
uci set plcm-status.@page[1].line1_left_arg='br-lan'
uci commit plcm-status
/etc/init.d/plcm-status restart
```

Saving and applying the LuCI form restarts the service through its procd UCI
trigger. If another parallel-port driver owns addresses `0x378`, `0x278`, or
`0x3bc`, unload or disable that driver first.

## External Kbuild

The root Makefile also supports a prepared kernel tree directly:

```sh
make -C plcm-drv/src KDIR=/path/to/linux-6.18
```

Pass the target `ARCH` and `CROSS_COMPILE` variables when cross-compiling.
The optional user-space diagnostics and status daemon are built separately
with `make -C plcm-drv/src tools`.
