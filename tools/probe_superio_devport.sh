#!/bin/sh

set -eu

DEVPORT=${DEVPORT:-/dev/port}

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this probe as root." >&2
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
    index_port=$1
    data_port=$2
    register=$3
    write_port "$index_port" "$register"
    read_port "$data_port"
}

probe_winbond()
{
    index_port=$1
    data_port=$((index_port + 1))

    write_port "$index_port" 135
    write_port "$index_port" 135
    device_id=$(read_register "$index_port" "$data_port" 32)
    device_revision=$(read_register "$index_port" "$data_port" 33)
    global_22=$(read_register "$index_port" "$data_port" 34)

    printf '0x%02x Winbond/Nuvoton/Fintek unlock: device=%s revision=%s reg22=%s\n' \
        "$index_port" "$device_id" "$device_revision" "$global_22"

    if [ "$device_id" != "ff" ] && [ "$device_id" != "00" ]; then
        if [ "${DUMP_ALL:-0}" = "1" ]; then
            ldn_list="0 1 2 3 4 5 6 7 8 9 10 11 12"
        else
            ldn_list="2 3 11"
        fi
        for ldn in $ldn_list; do
            write_port "$index_port" 7
            write_port "$data_port" "$ldn"
            active=$(read_register "$index_port" "$data_port" 48)
            base_high=$(read_register "$index_port" "$data_port" 96)
            base_low=$(read_register "$index_port" "$data_port" 97)
            irq=$(read_register "$index_port" "$data_port" 112)
            f0=$(read_register "$index_port" "$data_port" 240)
            f1=$(read_register "$index_port" "$data_port" 241)
            f2=$(read_register "$index_port" "$data_port" 242)
            printf '  LDN %02x: active=%s base=%s%s irq=%s f0=%s f1=%s f2=%s\n' \
                "$ldn" "$active" "$base_high" "$base_low" "$irq" \
                "$f0" "$f1" "$f2"
        done
    fi

    write_port "$index_port" 170
}

probe_ite()
{
    index_port=$1
    data_port=$((index_port + 1))

    write_port "$index_port" 135
    write_port "$index_port" 1
    write_port "$index_port" 85
    if [ "$index_port" -eq 46 ]; then
        write_port "$index_port" 85
    else
        write_port "$index_port" 170
    fi

    id_high=$(read_register "$index_port" "$data_port" 32)
    id_low=$(read_register "$index_port" "$data_port" 33)
    revision=$(read_register "$index_port" "$data_port" 34)

    write_port "$index_port" 2
    write_port "$data_port" 2

    printf '0x%02x ITE unlock: ID=%s%s revision=%s\n' \
        "$index_port" "$id_high" "$id_low" "$revision"
}

probe_uart()
{
    base=$1
    printf 'UART 0x%03x registers 0-7:' "$base"
    offset=0
    while [ "$offset" -lt 8 ]; do
        value=$(read_port $((base + offset)))
        printf ' %s' "$value"
        offset=$((offset + 1))
    done
    printf '\n'
}

echo "Read-only Super I/O identification probe"
probe_winbond 46
probe_winbond 78
probe_ite 46
probe_ite 78
probe_uart 1016
probe_uart 760
probe_uart 1000
probe_uart 744
