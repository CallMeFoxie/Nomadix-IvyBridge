#!/bin/sh

set -u

INTERVAL="${INTERVAL:-2}"
MIN_DUTY="${MIN_DUTY:-10}"
FAN_INPUTS="${FAN_INPUTS:-fan1_input fan2_input fan3_input fan4_input}"
LOGGER_TAG="${LOGGER_TAG:-nomadix-fand}"
HWMON_ROOT="${HWMON_ROOT:-/sys/class/hwmon}"

find_hwmon()
{
    expected="$1"
    for directory in "$HWMON_ROOT"/hwmon*; do
        [ -r "$directory/name" ] || continue
        name="$(cat "$directory/name")" || continue
        case "$name" in
            $expected)
                printf '%s\n' "$directory"
                return 0
                ;;
        esac
    done
    return 1
}

find_package_temp()
{
    coretemp="$1"
    for label in "$coretemp"/temp*_label; do
        [ -r "$label" ] || continue
        [ "$(cat "$label")" = "Package id 0" ] || continue
        input="${label%_label}_input"
        [ -r "$input" ] || return 1
        printf '%s\n' "$input"
        return 0
    done
    [ -r "$coretemp/temp1_input" ] || return 1
    printf '%s\n' "$coretemp/temp1_input"
}

set_pwm()
{
    duty="$1"
    found=0
    for pwm in "$winbond"/pwm[1-4]; do
        [ -e "$pwm" ] || continue
        found=1
        printf '%s\n' "$duty" > "$pwm" || return 1
    done
    [ "$found" -eq 1 ]
}

set_manual_mode()
{
    found=0
    for enable in "$winbond"/pwm[1-4]_enable; do
        [ -e "$enable" ] || continue
        found=1
        printf '1\n' > "$enable" || return 1
    done
    [ "$found" -eq 1 ]
}

log()
{
    logger -t "$LOGGER_TAG" "$*"
}

fail_safe()
{
    set_pwm 255 2>/dev/null || true
}

stop()
{
    fail_safe
    log "stopped; requested fail-safe PWM 255"
    trap - EXIT HUP INT TERM
    exit 0
}

select_duty()
{
    temperature="$1"
    if [ "$temperature" -ge 75000 ]; then
        printf '255\n'
    elif [ "$temperature" -ge 70000 ]; then
        printf '240\n'
    elif [ "$temperature" -ge 65000 ]; then
        printf '208\n'
    elif [ "$temperature" -ge 60000 ]; then
        printf '160\n'
    elif [ "$temperature" -ge 55000 ]; then
        printf '112\n'
    elif [ "$temperature" -ge 50000 ]; then
        printf '80\n'
    elif [ "$temperature" -gt 45000 ]; then
        printf '48\n'
    else
        printf '%s\n' "$MIN_DUTY"
    fi
}

apply_downward_hysteresis()
{
    temperature="$1"
    current="$2"
    desired="$3"
    [ "$desired" -lt "$current" ] || {
        printf '%s\n' "$desired"
        return
    }

    case "$current" in
        255) release=73000 ;;
        240) release=68000 ;;
        208) release=63000 ;;
        160) release=58000 ;;
        112) release=53000 ;;
        80)  release=48000 ;;
        48)  release=43000 ;;
        *)   release=0 ;;
    esac
    if [ "$temperature" -lt "$release" ]; then
        printf '%s\n' "$desired"
    else
        printf '%s\n' "$current"
    fi
}

case "$INTERVAL" in
    ''|*[!0-9]*) echo "INTERVAL must be a positive integer" >&2; exit 2 ;;
    0) echo "INTERVAL must be greater than zero" >&2; exit 2 ;;
esac

case "$MIN_DUTY" in
    ''|*[!0-9]*) echo "MIN_DUTY must be an integer from 10 to 255" >&2; exit 2 ;;
esac
if [ "$MIN_DUTY" -lt 10 ] || [ "$MIN_DUTY" -gt 255 ]; then
    echo "MIN_DUTY must be an integer from 10 to 255" >&2
    exit 2
fi

coretemp="$(find_hwmon coretemp)" || {
    echo "coretemp hwmon device not found" >&2
    exit 1
}
winbond="$(find_hwmon 'w83627*')" || {
    echo "W83627 hwmon device not found" >&2
    exit 1
}
package_temp="$(find_package_temp "$coretemp")" || {
    echo "Package id 0 temperature input not found" >&2
    exit 1
}
fan_inputs=""
for input in $FAN_INPUTS; do
    path="$winbond/$input"
    [ -r "$path" ] && fan_inputs="$fan_inputs $path"
done

trap fail_safe EXIT
trap stop HUP INT TERM

set_manual_mode || {
    echo "Failed to select manual PWM mode" >&2
    exit 1
}
set_pwm 255 || {
    echo "Failed to request initial fail-safe PWM" >&2
    exit 1
}

current=255
state="starting"
log "started; coretemp=$package_temp winbond=$winbond fans=$FAN_INPUTS"
sleep "$INTERVAL"

while :; do
    temperature="$(cat "$package_temp" 2>/dev/null)" || temperature=""
    case "$temperature" in
        ''|*[!0-9]*)
            if [ "$state" != "sensor-failure" ]; then
                fail_safe
                current=255
                state="sensor-failure"
                log "invalid package temperature; requested fail-safe PWM 255"
            fi
            sleep "$INTERVAL"
            continue
            ;;
    esac

    fan_summary=""
    for fan_input in $fan_inputs; do
        fan_rpm="$(cat "$fan_input" 2>/dev/null)" || fan_rpm=""
        fan_summary="$fan_summary $(basename "$fan_input")=${fan_rpm:-invalid}RPM"
    done

    if [ "$temperature" -gt 125000 ]; then
        if [ "$state" != "sensor-failure" ]; then
            fail_safe
            current=255
            state="sensor-failure"
            log "unsafe sensors: package=${temperature}mC$fan_summary; requested PWM 255"
        fi
        sleep "$INTERVAL"
        continue
    fi

    desired="$(select_duty "$temperature")"
    desired="$(apply_downward_hysteresis "$temperature" "$current" "$desired")"
    if [ "$desired" -ne "$current" ] || [ "$state" != "normal" ]; then
        if ! set_pwm "$desired"; then
            echo "Failed to set PWM duty $desired" >&2
            exit 1
        fi
        current="$desired"
        state="normal"
        log "package=${temperature}mC$fan_summary pwm=$current"
    fi
    sleep "$INTERVAL"
done
