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
#include <sys/wait.h>

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

    puts(mountopts);
    char my_pid[128];
    snprintf(my_pid, sizeof(my_pid), "%u", getpid());

    int child_pipe[2];
    pipe(child_pipe);

    pid_t fork_pid = fork();
    switch (fork_pid) {
        case -1:
            perror("fork");
            break;
        case 0:
            {
                char trash[1];
                read(child_pipe[0], trash, 1);
                char* uid_argv[] = { "newuidmap", my_pid, "0", "1000", "1", "1", "100000", "65536", NULL };
                char* gid_argv[] = { "newgidmap", my_pid, "0", "1000", "1", "1", "100000", "65536", NULL };
                pid_t child1, child2;
                posix_spawnp(&child1, uid_argv[0], NULL, NULL, uid_argv, environ);
                posix_spawnp(&child2, gid_argv[0], NULL, NULL, gid_argv, environ);
                int stat;
                waitpid(child1, &stat, 0);
                waitpid(child2, &stat, 0);
                return 0;
            }
    }

    if (unshare(CLONE_NEWUSER | CLONE_NEWNS)) {
        perror("unshare");
    }

    // we are in the namespace. child can now set our uid/gid map
    // unblock it and wait for its death (i. e. completion)
    char w = 'r';
    write(child_pipe[1], &w, 1);
    int stat;
    wait(&stat);


    // we are root in the unpriv user ns.
    if (mount("overlayfs ignores source", target, "overlay", 0, mountopts)) {
        perror("mount");
    }

    if (setgid(old_gid)) {
        perror("setgid");
    }

    if (setuid(old_uid)) {
        perror("setuid");
    }

    execvp(argv[3], argv + 3);
    perror("execvp");
}
