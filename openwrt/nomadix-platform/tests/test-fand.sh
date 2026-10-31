#!/bin/sh

set -eu

binary="${1:-/tmp/nomadix-fand}"
root="$(mktemp -d /tmp/nomadix-fand-test.XXXXXX)"
daemon_pid=""

cleanup()
{
	if [ -n "$daemon_pid" ]; then
		kill "$daemon_pid" 2>/dev/null || true
		wait "$daemon_pid" 2>/dev/null || true
	fi
	case "$root" in
	/tmp/nomadix-fand-test.*) rm -rf "$root" ;;
	esac
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$root/hwmon/hwmon0" "$root/hwmon/hwmon1"
mkdir -p "$root/pci/0000:00:1f.0"
printf 'coretemp\n' > "$root/hwmon/hwmon0/name"
printf 'Package id 0\n' > "$root/hwmon/hwmon0/temp1_label"
printf '50000\n' > "$root/hwmon/hwmon0/temp1_input"
printf 'w83627dhg\n' > "$root/hwmon/hwmon1/name"

index=1
while [ "$index" -le 4 ]; do
	printf '0\n' > "$root/hwmon/hwmon1/pwm$index"
	printf '0\n' > "$root/hwmon/hwmon1/pwm${index}_enable"
	printf '%s\n' "$((4000 + index))" \
		> "$root/hwmon/hwmon1/fan${index}_input"
	index=$((index + 1))
done

printf '0x8086\n' > "$root/pci/0000:00:1f.0/vendor"
printf '0x1c5c\n' > "$root/pci/0000:00:1f.0/device"
index=1
while [ "$index" -le 6 ]; do
	bus="$(printf '%02x' "$index")"
	mkdir -p "$root/pci/0000:$bus:00.0"
	printf '0x8086\n' > "$root/pci/0000:$bus:00.0/vendor"
	printf '0x150c\n' > "$root/pci/0000:$bus:00.0/device"
	index=$((index + 1))
done

"$binary" -H "$root/hwmon" -P "$root/pci" -i 1 -N 2 \
	> "$root/normal.log" 2>&1 &
daemon_pid=$!

attempt=0
while [ "$(cat "$root/hwmon/hwmon1/pwm1")" != 80 ]; do
	attempt=$((attempt + 1))
	[ "$attempt" -lt 30 ] || {
		cat "$root/normal.log" >&2
		echo "daemon did not select expected PWM 80" >&2
		exit 1
	}
	sleep 0.1
done

index=1
while [ "$index" -le 4 ]; do
	[ "$(cat "$root/hwmon/hwmon1/pwm$index")" = 80 ]
	[ "$(cat "$root/hwmon/hwmon1/pwm${index}_enable")" = 1 ]
	index=$((index + 1))
done

wait "$daemon_pid"
daemon_pid=""
index=1
while [ "$index" -le 4 ]; do
	[ "$(cat "$root/hwmon/hwmon1/pwm$index")" = 255 ]
	index=$((index + 1))
done

printf '50000\n' > "$root/hwmon/hwmon0/temp1_input"
"$binary" -H "$root/hwmon" -P "$root/pci" -i 1 \
	> "$root/hysteresis.log" 2>&1 &
daemon_pid=$!
attempt=0
while [ "$(cat "$root/hwmon/hwmon1/pwm1")" != 80 ]; do
	attempt=$((attempt + 1))
	[ "$attempt" -lt 30 ] || exit 1
	sleep 0.1
done
printf '49000\n' > "$root/hwmon/hwmon0/temp1_input"
sleep 1.2
[ "$(cat "$root/hwmon/hwmon1/pwm1")" = 80 ]
printf '47000\n' > "$root/hwmon/hwmon0/temp1_input"
attempt=0
while [ "$(cat "$root/hwmon/hwmon1/pwm1")" != 48 ]; do
	attempt=$((attempt + 1))
	[ "$attempt" -lt 30 ] || exit 1
	sleep 0.1
done
kill -TERM "$daemon_pid"
wait "$daemon_pid"
daemon_pid=""
index=1
while [ "$index" -le 4 ]; do
	[ "$(cat "$root/hwmon/hwmon1/pwm$index")" = 255 ]
	index=$((index + 1))
done

printf 'invalid\n' > "$root/hwmon/hwmon0/temp1_input"
"$binary" -H "$root/hwmon" -P "$root/pci" -i 1 -N 1 \
	> "$root/sensor-failure.log" 2>&1
index=1
while [ "$index" -le 4 ]; do
	[ "$(cat "$root/hwmon/hwmon1/pwm$index")" = 255 ]
	index=$((index + 1))
done

rm -rf "$root/hwmon/hwmon0"
"$binary" -F -H "$root/hwmon" -P "$root/pci" -i 1 -N 1 \
	> "$root/full-speed.log" 2>&1
index=1
while [ "$index" -le 4 ]; do
	[ "$(cat "$root/hwmon/hwmon1/pwm$index")" = 255 ]
	index=$((index + 1))
done

printf '0xffff\n' > "$root/pci/0000:06:00.0/device"
if "$binary" -H "$root/hwmon" -P "$root/pci" -i 1 -N 1 \
	> "$root/platform-failure.log" 2>&1; then
	echo "daemon accepted an invalid platform identity" >&2
	exit 1
fi

if "$binary" -S -H "$root/hwmon" -P "$root/pci" \
	-c '0:80,70000:200' -i 1 -N 1 \
	> "$root/unsafe-curve.log" 2>&1; then
	echo "daemon accepted a curve without a PWM 255 ceiling" >&2
	exit 1
fi

if "$binary" -S -H "$root/hwmon" -P "$root/pci" \
	-c '0:1,75000:255' -i 1 -N 1 \
	> "$root/unsafe-minimum.log" 2>&1; then
	echo "daemon accepted a PWM duty below 10" >&2
	exit 1
fi

echo "nomadix-fand integration tests passed"
