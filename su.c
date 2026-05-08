#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SUBIN       "/data/adb/su"
#define POLICY_PATH "/data/adb/rootstarter.cil"
#define LOG_PATH    "/data/adb/rootstarter.log"
#define SHELL       "/system/bin/sh"

void log_msg(const char *msg) {
    FILE *f = fopen(LOG_PATH, "a");
    if (f) {
        fprintf(f, "[su] %s\n", msg);
        fclose(f);
    }
}

int inject_policy() {
    int policy_fd = open(POLICY_PATH, O_RDONLY);
    if (policy_fd < 0) {
        log_msg("Failed to open policy file");
        return -1;
    }

    // FIX #1: Read entire file into memory first.
    // selinuxfs/load requires one atomic write — chunked writes corrupt the load.
    struct stat st;
    if (fstat(policy_fd, &st) < 0) {
        log_msg("Failed to stat policy file");
        close(policy_fd);
        return -1;
    }

    char *buf = malloc(st.st_size);
    if (!buf) {
        log_msg("Failed to allocate policy buffer");
        close(policy_fd);
        return -1;
    }

    ssize_t total = 0;
    while (total < st.st_size) {
        ssize_t n = read(policy_fd, buf + total, st.st_size - total);
        if (n <= 0) {
            log_msg("Failed to read policy file");
            free(buf);
            close(policy_fd);
            return -1;
        }
        total += n;
    }
    close(policy_fd);

    int selinux_fd = open("/sys/fs/selinux/load", O_WRONLY);
    if (selinux_fd < 0) {
        log_msg("Failed to open selinuxfs — may lack permission");
        free(buf);
        return -1;
    }

    ssize_t written = write(selinux_fd, buf, total);
    free(buf);
    close(selinux_fd);

    if (written != total) {
        log_msg("Failed to write policy (partial write)");
        return -1;
    }

    log_msg("SELinux policy injected successfully");
    return 0;
}

int check_mounted() {
    return (access(SUBIN, F_OK) == 0);
}

void spawn_shell(int argc, char *argv[]) {
    if (setuid(0) != 0 || setgid(0) != 0) {
        fprintf(stderr, "su: permission denied (uid escalation failed)\n");
        log_msg("UID escalation failed");
        exit(1);
    }

    setenv("PATH", "/sbin:/system/sbin:/system/bin:/system/xbin:/data/adb", 1);
    setenv("USER", "root", 1);
    setenv("HOME", "/data", 1);
    setenv("SHELL", SHELL, 1);

    // FIX #2: Detach from caller's session so we get a clean terminal
    setsid();

    if (argc > 1) {
        if (strcmp(argv[1], "-c") == 0 && argc > 2) {
            execl(SHELL, SHELL, "-c", argv[2], NULL);
        } else {
            execvp(argv[1], &argv[1]);
        }
    } else {
        // FIX #3: Leading dash on argv[0] = login shell, proper env init
        execl(SHELL, "-sh", NULL);
    }

    perror("su: exec failed");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--inject-policy") == 0) {
        log_msg("Boot-triggered policy injection");
        if (check_mounted()) {
            log_msg("Binary confirmed at /data/adb/su");
            return inject_policy();
        } else {
            log_msg("Binary not found at expected location");
            return 1;
        }
    }

    // FIX #4: Actually check inject_policy() return value
    if (check_mounted()) {
        if (inject_policy() != 0) {
            log_msg("Warning: policy re-injection failed — continuing with existing policy");
        }
    }

    spawn_shell(argc, argv);
    return 0;
}