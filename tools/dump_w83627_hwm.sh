#!/bin/sh

set -eu

DEVPORT=${DEVPORT:-/dev/port}
HWM_INDEX=${HWM_INDEX:-661}
HWM_DATA=$((HWM_INDEX + 1))

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this dump as root." >&2
    exit 1
fi

if [ ! -c "$DEVPORT" ]; then
    echo "$DEVPORT is unavailable; the kernel needs CONFIG_DEVPORT=y." >&2
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

read_port()
{
    port=$1
    if command -v hexdump >/dev/null 2>&1; then
        value=$(dd if="$DEVPORT" bs=1 skip="$port" count=1 2>/dev/null |
            hexdump -v -e '1/1 "%02x"')
    elif busybox hexdump </dev/null >/dev/null 2>&1; then
        value=$(dd if="$DEVPORT" bs=1 skip="$port" count=1 2>/dev/null |
            busybox hexdump -v -e '1/1 "%02x"')
    elif command -v od >/dev/null 2>&1; then
        value=$(dd if="$DEVPORT" bs=1 skip="$port" count=1 2>/dev/null |
            od -An -tx1 | tr -d ' \n')
    elif command -v xxd >/dev/null 2>&1; then
        value=$(dd if="$DEVPORT" bs=1 skip="$port" count=1 2>/dev/null |
            xxd -p | tr -d '\n')
    else
        echo "Install coreutils-od or provide hexdump/xxd." >&2
        exit 1
    fi
    if [ -z "$value" ]; then
        echo "Read from I/O port $port failed." >&2
        exit 1
    fi
    printf '%s' "$value"
}

read_register()
{
    register=$1
    write_port "$HWM_INDEX" "$register"
    read_port "$HWM_DATA"
}

select_bank()
{
    bank=$1
    write_port "$HWM_INDEX" 78
    write_port "$HWM_DATA" "$bank"
}

dump_range()
{
    bank=$1
    first=$2
    last=$3
    register=$first

    select_bank "$bank"
    printf 'BANK %02x REG %02x:' "$bank" "$first"
    while [ "$register" -le "$last" ]; do
        if [ "$register" -ne "$first" ] &&
            [ $((register % 16)) -eq 0 ]; then
            printf '\nBANK %02x REG %02x:' "$bank" "$register"
        fi
        value=$(read_register "$register")
        printf ' %s' "$value"
        register=$((register + 1))
    done
    printf '\n'
}

write_port "$HWM_INDEX" 78
original_bank_hex=$(read_port "$HWM_DATA")
original_bank=$((0x$original_bank_hex))
bank_prefix=$((original_bank & 248))

echo "W83627 hardware monitor at index 0x295/data 0x296"
printf 'Original bank register: %s\n' "$original_bank_hex"

dump_range "$bank_prefix" 0 127
for bank in 1 2 3 4 5 6; do
    dump_range $((bank_prefix | bank)) 80 95
done

select_bank "$original_bank"
