#!/bin/sh

set -eu

MODE=${1:-full}
DEVPORT=${DEVPORT:-/dev/port}
SIO_INDEX=46
SIO_DATA=47
HWM_INDEX=661
HWM_DATA=662

case "$MODE" in
    full)
        pwm=255
        ;;
    lanner)
        pwm=10
        ;;
    *)
        echo "Usage: $0 [full|lanner]" >&2
        exit 1
        ;;
esac

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this initializer as root." >&2
    exit 1
fi

if [ ! -c "$DEVPORT" ]; then
    echo "$DEVPORT is unavailable; the kernel needs CONFIG_DEVPORT=y." >&2
    exit 1
fi

if ! command -v setpci >/dev/null 2>&1; then
    echo "setpci is unavailable; install pciutils." >&2
    exit 1
fi

write_port()
{
    port=$1
    value=$2
    octal=$(printf '%03o' "$value")
    printf "\\$octal" |
        dd of="$DEVPORT" bs=1 seek="$port" count=1 conv=notrunc 2>/dev/null
}

sio_write()
{
    write_port "$SIO_INDEX" "$1"
    write_port "$SIO_DATA" "$2"
}

hwm_write()
{
    write_port "$HWM_INDEX" "$1"
    write_port "$HWM_DATA" "$2"
}

# Preserve the ONDA 0xA00 generic window in GEN1 and use free GEN2 for HWM.
setpci -s 00:1f.0 80.w=0010
setpci -s 00:1f.0 82.w=3f07
setpci -s 00:1f.0 88.l=000c0291

write_port "$SIO_INDEX" 135
write_port "$SIO_INDEX" 135
sio_write 7 11
sio_write 96 2
sio_write 97 144
sio_write 240 128
sio_write 242 128
sio_write 48 1
write_port "$SIO_INDEX" 170

# Bank 0, monitoring enabled, manual PWM mode on all four channels.
hwm_write 78 0
hwm_write 64 3
hwm_write 4 0
hwm_write 18 120
hwm_write 98 0
hwm_write 1 "$pwm"
hwm_write 3 "$pwm"
hwm_write 17 "$pwm"
hwm_write 97 "$pwm"

echo "PCH LPC and W83627 HWM initialized; PWM duty is $pwm/255."
echo "A power cycle restores the firmware state."
