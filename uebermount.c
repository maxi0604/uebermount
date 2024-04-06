#define _GNU_SOURCE
#define PATH_LENGTH 1024
#include <sched.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>

extern char** environ;
void proc_setgroups_write(void) {
    char setgroups_path[PATH_MAX];
    int fd;

    snprintf(setgroups_path, PATH_MAX, "/proc/self/setgroups");

    fd = open(setgroups_path, O_RDWR);
    if (fd == -1) {
        perror("open");
    }

    if (write(fd, "deny", strlen("deny")) == -1) {
        perror("write");
    }

    close(fd);
}

void wrmap(char* target, char* str) {
    // "/proc/self/uid_map"
    puts(target);
    puts(str);
    int map = open(target, O_RDWR);
    if (map < 0) {
        perror("open");
    }

    if (write(map, str, strlen(str)) < 0) {
        perror("write");
    }

    if (close(map)) {
        perror("close");
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
    char mountopts[PATH_LENGTH];
    char uid_entry[PATH_LENGTH];
    char gid_entry[PATH_LENGTH];

    uid_t old_uid = geteuid();
    uid_t old_gid = getegid();

    snprintf(mountopts, sizeof(mountopts), "lowerdir=%s,upperdir=%s,workdir=%s", source, target, "tempdir");
    snprintf(uid_entry, sizeof(uid_entry), "1000 %u 1\n", old_uid);
    snprintf(gid_entry, sizeof(gid_entry), "1000 %u 1\n", old_gid);

    if (unshare(CLONE_NEWUSER | CLONE_NEWNS)) {
        perror("unshare");
    }

    wrmap("/proc/self/uid_map", uid_entry);
    proc_setgroups_write();
    wrmap("/proc/self/gid_map", gid_entry);

    if (mount("overlayfs ignores source", target, "overlay", 0, mountopts)) {
        perror("mount");
    }

    if (setuid(old_uid)) {
        perror("seteuid");
    }

    setegid(old_gid);
    execvp(argv[3], argv + 3);
    perror("execvp");
}
