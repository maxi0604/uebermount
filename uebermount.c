#define _GNU_SOURCE
#include <sched.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>
#include <stdlib.h>

void die(const char* error) {
    perror(error);
    exit(1);
}

void proc_setgroups_write(void) {
    int fd = open("/proc/self/setgroups", O_RDWR);
    if (fd == -1) {
        die("open");
    }

    if (write(fd, "deny", strlen("deny")) == -1) {
        die("write");
    }

    close(fd);
}

void wrmap(char* target, char* str) {
    int map = open(target, O_RDWR);
    if (map < 0) {
        die("open");
    }

    if (write(map, str, strlen(str)) < 0) {
        die("write");
    }

    if (close(map)) {
        die("close");
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s source target program\n", argv[0]);
        return 1;
    }

    char* source = argv[1];
    char* target = argv[2];
    char mountopts[4 * PATH_MAX];
    char uid_entry[128];
    char gid_entry[128];

    uid_t old_uid = geteuid();
    uid_t old_gid = getegid();

    snprintf(mountopts, sizeof(mountopts), "lowerdir=%s,upperdir=%s,workdir=%s", source, target, "tempdir");
    snprintf(uid_entry, sizeof(uid_entry), "%u %u 1\n", old_uid, old_uid);
    snprintf(gid_entry, sizeof(gid_entry), "%u %u 1\n", old_gid, old_gid);

    if (unshare(CLONE_NEWUSER | CLONE_NEWNS)) {
        die("unshare");
    }

    wrmap("/proc/self/uid_map", uid_entry);
    proc_setgroups_write();
    wrmap("/proc/self/gid_map", gid_entry);

    if (mount("overlayfs ignores source", target, "overlay", 0, mountopts)) {
        die("mount");
    }

    execvp(argv[3], argv + 3);
    perror("execvp");
}
