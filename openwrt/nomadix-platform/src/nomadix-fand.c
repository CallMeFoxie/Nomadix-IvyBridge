#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_CURVE_POINTS 16
#define MIN_PWM_DUTY 10
#define PWM_COUNT 4

struct curve_point {
    long temperature;
    int duty;
};

struct config {
    const char *hwmon_root;
    const char *pci_root;
    unsigned int interval;
    long hysteresis;
    unsigned long iterations;
    bool verify_platform;
    bool full_speed;
    struct curve_point curve[MAX_CURVE_POINTS];
    size_t curve_count;
};

struct devices {
    char temperature[PATH_MAX];
    char pwm[PWM_COUNT][PATH_MAX];
    char pwm_enable[PWM_COUNT][PATH_MAX];
    char fan_input[PWM_COUNT][PATH_MAX];
};

static volatile sig_atomic_t stopping;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}

static bool join_path(char *output, size_t output_size, const char *left,
                      const char *right)
{
    int length = snprintf(output, output_size, "%s/%s", left, right);
    return length >= 0 && (size_t)length < output_size;
}

static bool read_line(const char *path, char *output, size_t output_size)
{
    FILE *stream;
    size_t length;

    stream = fopen(path, "r");
    if (stream == NULL)
        return false;
    if (fgets(output, (int)output_size, stream) == NULL) {
        fclose(stream);
        return false;
    }
    if (fclose(stream) != 0)
        return false;

    length = strlen(output);
    while (length > 0 &&
           (output[length - 1] == '\n' || output[length - 1] == '\r'))
        output[--length] = '\0';
    return true;
}

static bool read_long(const char *path, long *value)
{
    char buffer[64];
    char *end;
    long parsed;

    if (!read_line(path, buffer, sizeof(buffer)))
        return false;
    errno = 0;
    parsed = strtol(buffer, &end, 10);
    if (errno != 0 || end == buffer || *end != '\0')
        return false;
    *value = parsed;
    return true;
}

static bool write_and_verify(const char *path, int value)
{
    char buffer[16];
    int fd;
    int length;
    ssize_t written;
    long read_back;

    length = snprintf(buffer, sizeof(buffer), "%d\n", value);
    if (length < 0 || (size_t)length >= sizeof(buffer))
        return false;

    fd = open(path, O_WRONLY | O_CLOEXEC | O_TRUNC);
    if (fd < 0)
        return false;
    written = write(fd, buffer, (size_t)length);
    if (close(fd) != 0 || written != length)
        return false;
    return read_long(path, &read_back) && read_back == value;
}

static bool starts_with(const char *value, const char *prefix)
{
    return strncmp(value, prefix, strlen(prefix)) == 0;
}

static bool find_hwmon(const char *root, const char *expected, bool prefix,
                       char *result, size_t result_size)
{
    DIR *directory;
    struct dirent *entry;
    char candidate[PATH_MAX];
    char name_path[PATH_MAX];
    char name[128];
    bool found = false;

    directory = opendir(root);
    if (directory == NULL)
        return false;

    while ((entry = readdir(directory)) != NULL) {
        if (!starts_with(entry->d_name, "hwmon"))
            continue;
        if (!join_path(candidate, sizeof(candidate), root, entry->d_name) ||
            !join_path(name_path, sizeof(name_path), candidate, "name") ||
            !read_line(name_path, name, sizeof(name)))
            continue;
        if ((prefix && starts_with(name, expected)) ||
            (!prefix && strcmp(name, expected) == 0)) {
            if (snprintf(result, result_size, "%s", candidate) >=
                (int)result_size)
                break;
            found = true;
            break;
        }
    }
    closedir(directory);
    return found;
}

static bool find_package_temperature(const char *coretemp, char *result,
                                     size_t result_size)
{
    DIR *directory;
    struct dirent *entry;
    char label_path[PATH_MAX];
    char label[128];
    char input_name[NAME_MAX + 1];
    size_t name_length;
    bool found = false;

    directory = opendir(coretemp);
    if (directory == NULL)
        return false;
    while ((entry = readdir(directory)) != NULL) {
        if (!starts_with(entry->d_name, "temp") ||
            !strstr(entry->d_name, "_label"))
            continue;
        if (!join_path(label_path, sizeof(label_path), coretemp,
                       entry->d_name) ||
            !read_line(label_path, label, sizeof(label)) ||
            strcmp(label, "Package id 0") != 0)
            continue;

        name_length = strlen(entry->d_name);
        if (name_length <= strlen("_label") ||
            name_length - strlen("_label") + strlen("_input") >=
                sizeof(input_name))
            continue;
        memcpy(input_name, entry->d_name, name_length - strlen("_label"));
        strcpy(input_name + name_length - strlen("_label"), "_input");
        if (join_path(result, result_size, coretemp, input_name))
            found = true;
        break;
    }
    closedir(directory);
    return found;
}

static bool discover_devices(const struct config *config,
                             struct devices *devices)
{
    char coretemp[PATH_MAX];
    char winbond[PATH_MAX];
    char name[32];
    unsigned int index;

    if (!find_hwmon(config->hwmon_root, "w83627", true, winbond,
                    sizeof(winbond))) {
        syslog(LOG_ERR, "W83627 hwmon device not found under %s",
               config->hwmon_root);
        return false;
    }
    for (index = 0; index < PWM_COUNT; ++index) {
        snprintf(name, sizeof(name), "pwm%u", index + 1);
        if (!join_path(devices->pwm[index], sizeof(devices->pwm[index]),
                       winbond, name))
            return false;
        snprintf(name, sizeof(name), "pwm%u_enable", index + 1);
        if (!join_path(devices->pwm_enable[index],
                       sizeof(devices->pwm_enable[index]), winbond, name))
            return false;
        snprintf(name, sizeof(name), "fan%u_input", index + 1);
        if (!join_path(devices->fan_input[index],
                       sizeof(devices->fan_input[index]), winbond, name))
            return false;
    }
    for (index = 0; index < PWM_COUNT; ++index) {
        if (access(devices->pwm[index], R_OK | W_OK) != 0 ||
            access(devices->pwm_enable[index], R_OK | W_OK) != 0) {
            syslog(LOG_ERR, "required PWM channel %u is unavailable",
                   index + 1);
            return false;
        }
    }
    if (config->full_speed)
        return true;
    if (!find_hwmon(config->hwmon_root, "coretemp", false, coretemp,
                    sizeof(coretemp))) {
        syslog(LOG_ERR, "coretemp hwmon device not found under %s",
               config->hwmon_root);
        return true;
    }
    if (!find_package_temperature(coretemp, devices->temperature,
                                  sizeof(devices->temperature))) {
        syslog(LOG_ERR, "Package id 0 temperature input not found");
        devices->temperature[0] = '\0';
    }
    return true;
}

static bool set_all_pwm(const struct devices *devices, int duty)
{
    bool success = true;
    unsigned int index;

    for (index = 0; index < PWM_COUNT; ++index) {
        if (!write_and_verify(devices->pwm[index], duty))
            success = false;
    }
    return success;
}

static void request_fail_safe(const struct devices *devices)
{
    if (!set_all_pwm(devices, 255))
        syslog(LOG_CRIT, "failed to apply fail-safe PWM 255 to every channel");
}

static bool select_manual_mode(const struct devices *devices)
{
    unsigned int index;

    for (index = 0; index < PWM_COUNT; ++index) {
        if (!write_and_verify(devices->pwm_enable[index], 1))
            return false;
    }
    return true;
}

static bool read_identity_value(const char *pci_root, const char *name,
                                char *value, size_t value_size)
{
    char path[PATH_MAX];
    return join_path(path, sizeof(path), pci_root, name) &&
           read_line(path, value, value_size);
}

static bool verify_platform(const struct config *config)
{
    char bridge[PATH_MAX];
    char nic[PATH_MAX];
    char nic_name[32];
    char vendor[32];
    char device[32];
    unsigned int bus;

    if (!join_path(bridge, sizeof(bridge), config->pci_root,
                   "0000:00:1f.0") ||
        !read_identity_value(bridge, "vendor", vendor, sizeof(vendor)) ||
        !read_identity_value(bridge, "device", device, sizeof(device))) {
        syslog(LOG_ERR, "cannot read LPC bridge identity");
        return false;
    }
    if (strcasecmp(vendor, "0x8086") != 0 ||
        strcasecmp(device, "0x1c5c") != 0) {
        syslog(LOG_ERR, "unsupported LPC bridge %s:%s, expected 0x8086:0x1c5c",
               vendor, device);
        return false;
    }
    for (bus = 1; bus <= 6; ++bus) {
        snprintf(nic_name, sizeof(nic_name), "0000:%02x:00.0", bus);
        if (!join_path(nic, sizeof(nic), config->pci_root, nic_name) ||
            !read_identity_value(nic, "vendor", vendor, sizeof(vendor)) ||
            !read_identity_value(nic, "device", device, sizeof(device)) ||
            strcasecmp(vendor, "0x8086") != 0 ||
            strcasecmp(device, "0x150c") != 0) {
            syslog(LOG_ERR, "expected Intel 82583V not found at %s",
                   nic_name);
            return false;
        }
    }
    return true;
}

static bool parse_unsigned(const char *text, unsigned long minimum,
                           unsigned long maximum, unsigned long *result)
{
    char *end;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        value < minimum || value > maximum)
        return false;
    *result = value;
    return true;
}

static bool parse_curve(struct config *config, const char *text)
{
    char *copy;
    char *save_pointer = NULL;
    char *token;
    size_t count = 0;
    long previous_temperature = -1;
    unsigned long previous_duty = 0;

    copy = strdup(text);
    if (copy == NULL)
        return false;
    token = strtok_r(copy, ",", &save_pointer);
    while (token != NULL) {
        char *separator = strchr(token, ':');
        unsigned long temperature;
        unsigned long duty;

        if (separator == NULL || count == MAX_CURVE_POINTS) {
            free(copy);
            return false;
        }
        *separator = '\0';
        if (!parse_unsigned(token, 0, 125000, &temperature) ||
            !parse_unsigned(separator + 1, MIN_PWM_DUTY, 255, &duty) ||
            (long)temperature <= previous_temperature ||
            duty < previous_duty) {
            free(copy);
            return false;
        }
        config->curve[count].temperature = (long)temperature;
        config->curve[count].duty = (int)duty;
        previous_temperature = (long)temperature;
        previous_duty = duty;
        ++count;
        token = strtok_r(NULL, ",", &save_pointer);
    }
    free(copy);
    if (count == 0 || config->curve[0].temperature != 0 ||
        config->curve[count - 1].duty != 255)
        return false;
    config->curve_count = count;
    return true;
}

static size_t curve_index(const struct config *config, long temperature)
{
    size_t index;
    size_t selected = 0;

    for (index = 1; index < config->curve_count; ++index) {
        if (temperature < config->curve[index].temperature)
            break;
        selected = index;
    }
    return selected;
}

static size_t apply_hysteresis(const struct config *config, long temperature,
                               size_t current, size_t desired)
{
    long release;

    if (desired >= current || current == 0)
        return desired;
    release = config->curve[current].temperature - config->hysteresis;
    return temperature < release ? desired : current;
}

static void log_state(const struct devices *devices, long temperature,
                      int duty)
{
    char fans[256] = "";
    size_t used = 0;
    unsigned int index;

    for (index = 0; index < PWM_COUNT && used < sizeof(fans); ++index) {
        long rpm;
        int length;

        if (read_long(devices->fan_input[index], &rpm))
            length = snprintf(fans + used, sizeof(fans) - used,
                              " fan%u=%ldRPM", index + 1, rpm);
        else
            length = snprintf(fans + used, sizeof(fans) - used,
                              " fan%u=unavailable", index + 1);
        if (length < 0 || (size_t)length >= sizeof(fans) - used)
            break;
        used += (size_t)length;
    }
    syslog(LOG_NOTICE, "package=%ldmC pwm=%d%s", temperature, duty, fans);
}

static bool wait_interval(unsigned int seconds)
{
    struct timespec request = { .tv_sec = (time_t)seconds, .tv_nsec = 0 };

    while (!stopping && nanosleep(&request, &request) != 0) {
        if (errno != EINTR)
            return false;
    }
    return !stopping;
}

static void usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [-S] [-i seconds] [-y millidegrees] "
            "[-F | -c temp:duty,...] [-H hwmon-root] [-P pci-root] "
            "[-N iterations]\n",
            program);
}

int main(int argc, char **argv)
{
    struct config config = {
        .hwmon_root = "/sys/class/hwmon",
        .pci_root = "/sys/bus/pci/devices",
        .interval = 2,
        .hysteresis = 2000,
        .iterations = 0,
        .verify_platform = true,
        .full_speed = false,
    };
    struct devices devices = { 0 };
    const char *curve =
        "0:10,45001:48,50000:80,55000:112,60000:160,"
        "65000:208,70000:240,75000:255";
    struct sigaction action = { 0 };
    unsigned long parsed;
    unsigned long iteration = 0;
    size_t current;
    bool degraded = false;
    int option;
    int result = EXIT_FAILURE;

    while ((option = getopt(argc, argv, "SFi:y:c:H:P:N:h")) != -1) {
        switch (option) {
        case 'S':
            config.verify_platform = false;
            break;
        case 'F':
            config.full_speed = true;
            break;
        case 'i':
            if (!parse_unsigned(optarg, 1, 3600, &parsed)) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            config.interval = (unsigned int)parsed;
            break;
        case 'y':
            if (!parse_unsigned(optarg, 0, 30000, &parsed)) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            config.hysteresis = (long)parsed;
            break;
        case 'c':
            curve = optarg;
            break;
        case 'H':
            config.hwmon_root = optarg;
            break;
        case 'P':
            config.pci_root = optarg;
            break;
        case 'N':
            if (!parse_unsigned(optarg, 1, ULONG_MAX, &parsed)) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            config.iterations = parsed;
            break;
        default:
            usage(argv[0]);
            return option == 'h' ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }
    if (optind != argc ||
        (!config.full_speed && !parse_curve(&config, curve))) {
        fprintf(stderr, "invalid command line or temperature curve\n");
        return EXIT_FAILURE;
    }

    openlog("nomadix-fand", LOG_PID | LOG_PERROR, LOG_DAEMON);
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);

    if (config.verify_platform && !verify_platform(&config))
        goto done;
    if (!discover_devices(&config, &devices))
        goto done;
    if (!select_manual_mode(&devices)) {
        syslog(LOG_CRIT, "failed to select manual mode on every PWM channel");
        request_fail_safe(&devices);
        goto done;
    }
    if (!set_all_pwm(&devices, 255)) {
        syslog(LOG_CRIT, "failed to apply and verify initial PWM 255");
        request_fail_safe(&devices);
        goto done;
    }

    if (!config.full_speed && devices.temperature[0] == '\0') {
        syslog(LOG_CRIT,
               "temperature sensor unavailable; retaining fail-safe PWM 255");
        goto done;
    }
    current = config.full_speed ? 0 : config.curve_count - 1;
    if (config.full_speed)
        syslog(LOG_NOTICE,
               "started in verified full-speed mode; interval=%us",
               config.interval);
    else
        syslog(LOG_NOTICE,
               "started with fail-safe PWM 255; temperature=%s interval=%us",
               devices.temperature, config.interval);

    while (!stopping) {
        long temperature;
        size_t desired;
        int duty;

        if (!wait_interval(config.interval))
            break;
        if (config.full_speed) {
            if (!set_all_pwm(&devices, 255)) {
                syslog(LOG_CRIT, "failed to maintain full-speed PWM 255");
                request_fail_safe(&devices);
                goto done;
            }
            if (config.iterations != 0 && ++iteration >= config.iterations) {
                result = EXIT_SUCCESS;
                break;
            }
            continue;
        }
        if (!read_long(devices.temperature, &temperature) ||
            temperature < 0 || temperature > 125000) {
            if (!degraded) {
                request_fail_safe(&devices);
                current = config.curve_count - 1;
                degraded = true;
                syslog(LOG_CRIT,
                       "invalid package temperature; requested PWM 255");
            }
        } else {
            desired = curve_index(&config, temperature);
            desired = apply_hysteresis(&config, temperature, current,
                                       desired);
            duty = config.curve[desired].duty;
            if (degraded || desired != current) {
                if (!set_all_pwm(&devices, duty)) {
                    syslog(LOG_CRIT,
                           "failed to apply and verify PWM %d", duty);
                    request_fail_safe(&devices);
                    goto done;
                }
                current = desired;
                degraded = false;
                log_state(&devices, temperature, duty);
            }
        }
        if (config.iterations != 0 && ++iteration >= config.iterations) {
            result = EXIT_SUCCESS;
            break;
        }
    }
    if (stopping)
        result = EXIT_SUCCESS;

done:
    if (devices.pwm[0][0] != '\0') {
        request_fail_safe(&devices);
        syslog(LOG_NOTICE, "stopped; requested fail-safe PWM 255");
    }
    closelog();
    return result;
}
