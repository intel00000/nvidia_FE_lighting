/* service.c - installs and removes the systemd boot service that applies the saved settings. */
#include "service.h"
#include "device.h"
#include "diag.h"
#include "nvapi_loader.h"
#include "profile.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static const char unit_text[] =
    "[Unit]\n"
    "Description=Apply the saved NVIDIA FE lighting settings\n"
    "After=nvidia-persistenced.service nvidia-resume.service\n"
    "After=suspend.target hibernate.target hybrid-sleep.target suspend-then-hibernate.target\n"
    "\n"
    "[Service]\n"
    "Type=oneshot\n"
    "ExecStart=" SERVICE_BIN_PATH " startup " SERVICE_CONF_PATH "\n"
    "\n"
    "[Install]\n"
    "WantedBy=multi-user.target suspend.target hibernate.target hybrid-sleep.target suspend-then-hibernate.target\n";

static int make_dir(const char *path)
{
    if (mkdir(path, 0755) == 0 || errno == EEXIST)
        return 0;
    log_err("felight: cannot create %s: %s\n", path, strerror(errno));
    return EXIT_FILE;
}

/* Writes next to the destination and renames into place, so nothing ever sees half a file. */
static int write_file(const char *path, const char *data, size_t len, mode_t mode)
{
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
        log_err("felight: cannot write %s: %s\n", tmp, strerror(errno));
        return EXIT_FILE;
    }
    size_t done = 0;
    while (done < len)
    {
        ssize_t n = write(fd, data + done, len - done);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        done += (size_t)n;
    }
    int err = done < len ? errno : 0;
    if (!err && fchmod(fd, mode) != 0)
        err = errno;
    if (close(fd) != 0 && !err)
        err = errno;
    if (!err && rename(tmp, path) != 0)
        err = errno;
    if (err)
    {
        log_err("felight: cannot write %s: %s\n", path, strerror(err));
        unlink(tmp);
        return EXIT_FILE;
    }
    return 0;
}

/* The service runs its own copy of felight. */
static int install_binary(const char *self)
{
    if (strcmp(self, SERVICE_BIN_PATH) == 0)
        return 0;
    int fd = open("/proc/self/exe", O_RDONLY);
    struct stat st;
    if (fd < 0 || fstat(fd, &st) != 0)
    {
        log_err("felight: cannot read %s: %s\n", self, strerror(errno));
        if (fd >= 0)
            close(fd);
        return EXIT_FILE;
    }
    char *buf = malloc((size_t)st.st_size);
    size_t done = 0;
    while (buf && done < (size_t)st.st_size)
    {
        ssize_t n = read(fd, buf + done, (size_t)st.st_size - done);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        done += (size_t)n;
    }
    close(fd);
    int rc = buf && done == (size_t)st.st_size ? write_file(SERVICE_BIN_PATH, buf, done, 0755) : EXIT_FILE;
    if (!buf || done != (size_t)st.st_size)
        log_err("felight: cannot read %s\n", self);
    free(buf);
    return rc;
}

static int systemctl(const char *verb, const char *unit)
{
    printf("systemctl %s%s%s\n", verb, unit ? " " : "", unit ? unit : "");
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0)
    {
        log_err("felight: cannot run systemctl: %s\n", strerror(errno));
        return EXIT_SERVICE;
    }
    if (pid == 0)
    {
        execlp("systemctl", "systemctl", verb, unit, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
        ;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return 0;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
        log_err("felight: systemctl not found; this system does not seem to use systemd\n");
    else
        log_err("felight: systemctl %s failed\n", verb);
    return EXIT_SERVICE;
}

static int remove_file(const char *path)
{
    if (unlink(path) == 0)
    {
        printf("removed %s\n", path);
        return 0;
    }
    if (errno == ENOENT)
        return 0;
    log_err("felight: cannot remove %s: %s\n", path, strerror(errno));
    return EXIT_FILE;
}

int service_enable(const char *self, NvU32 gpu, int delay_seconds)
{
    static zone_t zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
    int rc = nv_open(&g_nv);
    if (rc)
        return rc;
    if (!check_gpu_index(&g_nv, (long)gpu))
        return EXIT_USAGE;
    gpu_t g;
    NvU32 n = 0;
    if ((rc = read_gpu(&g_nv, gpu, &g)) || (rc = read_zones(&g_nv, gpu, 0, zones, &n)))
        return rc;
    if ((rc = make_dir(SERVICE_CONF_DIR)) || (rc = profile_save(SERVICE_CONF_PATH, &g, zones, n, delay_seconds)))
        return rc;
    printf("saved GPU %u (%s, %u zone(s)) to %s\n", gpu, g.name, n, SERVICE_CONF_PATH);
    if ((rc = make_dir(SERVICE_BIN_DIR)) || (rc = install_binary(self)))
        return rc;
    printf("installed %s\n", SERVICE_BIN_PATH);
    if ((rc = write_file(SERVICE_UNIT_PATH, unit_text, sizeof unit_text - 1, 0644)))
        return rc;
    printf("wrote %s\n", SERVICE_UNIT_PATH);
    if ((rc = systemctl("daemon-reload", NULL)) || (rc = systemctl("enable", SERVICE_NAME)))
        return rc;
    printf("boot service enabled: these settings are applied at boot and after every resume (log: journalctl -u %s)\n", APP_DIR_NAME);
    return 0;
}

int service_disable(void)
{
    struct stat st;
    int rc;
    if (stat(SERVICE_UNIT_PATH, &st) == 0)
    {
        if ((rc = systemctl("disable", SERVICE_NAME)) || (rc = remove_file(SERVICE_UNIT_PATH)) || (rc = systemctl("daemon-reload", NULL)))
            return rc;
    }
    if ((rc = remove_file(SERVICE_CONF_PATH)) || (rc = remove_file(SERVICE_BIN_PATH)))
        return rc;
    rmdir(SERVICE_CONF_DIR);
    rmdir(SERVICE_BIN_DIR);
    printf("boot service removed\n");
    return 0;
}

static void print_quoted(FILE *f, const char *s)
{
    if (*s && s[strspn(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_./+-")] == '\0')
    {
        fputs(s, f);
        return;
    }
    fputc('\'', f);
    for (; *s; s++)
    {
        if (*s == '\'')
            fputs("'\\''", f);
        else
            fputc(*s, f);
    }
    fputc('\'', f);
}

void service_print_enable_steps(FILE *f, const char *self, long gpu, long delay_seconds)
{
    fputs("sudo install -D -m 0755 ", f);
    print_quoted(f, self);
    fputs(" " SERVICE_BIN_PATH "\n", f);
    fputs("sudo install -d " SERVICE_CONF_DIR "\n", f);
    fprintf(f, "sudo " SERVICE_BIN_PATH " save --gpu %ld --delay %ld " SERVICE_CONF_PATH "\n", gpu, delay_seconds);
    fputs("sudo tee " SERVICE_UNIT_PATH " > /dev/null <<'EOF'\n", f);
    fputs(unit_text, f);
    fputs("EOF\n", f);
    fputs("sudo systemctl daemon-reload\n", f);
    fputs("sudo systemctl enable " SERVICE_NAME "\n", f);
}

void service_print_disable_steps(FILE *f)
{
    fputs("sudo systemctl disable " SERVICE_NAME "\n"
          "sudo rm -f " SERVICE_UNIT_PATH "\n"
          "sudo systemctl daemon-reload\n"
          "sudo rm -rf " SERVICE_CONF_DIR " " SERVICE_BIN_DIR "\n",
          f);
}
