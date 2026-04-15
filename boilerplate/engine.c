#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include "monitor_ioctl.h"
#include <sys/ioctl.h>

#define STACK_SIZE (1024 * 1024)
#define LOG_CHUNK_SIZE 4096

// ---------------- CHILD ----------------

int child_fn(void *arg) {
    char **argv = (char **)arg;

    char *id = argv[1];
    char *rootfs = argv[2];
    char *cmd = argv[3];
    int pipe_fd = atoi(argv[4]);

    if (chroot(rootfs) != 0) {
        perror("chroot failed");
        exit(1);
    }

    chdir("/");
    sethostname(id, strlen(id));
    mount("proc", "/proc", "proc", 0, NULL);

    dup2(pipe_fd, STDOUT_FILENO);
    dup2(pipe_fd, STDERR_FILENO);
    close(pipe_fd);

    char *args[] = {cmd, NULL};
    execvp(cmd, args);

    perror("exec failed");
    return 1;
}

// ---------------- CONTAINER ----------------

void run_container(char *id, char *rootfs, char *command, int soft_mib, int hard_mib) {
    int pipefd[2];
    pipe(pipefd);

    char *stack = malloc(STACK_SIZE);

    char fd_str[10];
    sprintf(fd_str, "%d", pipefd[1]);

    char *argv[] = {"container", id, rootfs, command, fd_str, NULL};

    pid_t pid = clone(child_fn, stack + STACK_SIZE,
                      CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                      argv);

    if (pid < 0) {
        perror("clone failed");
        exit(1);
    }

    //  REGISTER WITH KERNEL MODULE
    int fd = open("/dev/container_monitor", O_RDWR);
    if (fd >= 0) {
        struct monitor_request req;

        req.pid = pid;
        strncpy(req.container_id, id, sizeof(req.container_id));
        req.container_id[sizeof(req.container_id) - 1] = '\0';

        req.soft_limit_bytes = soft_mib * 1024 * 1024;
        req.hard_limit_bytes = hard_mib * 1024 * 1024;

        ioctl(fd, MONITOR_REGISTER, &req);
        close(fd);
    }

    close(pipefd[1]);

    printf("Container %s started (PID %d)\n", id, pid);

    FILE *f = fopen("containers.txt", "a");
    if (f) {
        fprintf(f, "%s %d running\n", id, pid);
        fclose(f);
    }

    char buf[LOG_CHUNK_SIZE];
    char log_path[100];
    snprintf(log_path, sizeof(log_path), "logs/%s.log", id);

    FILE *logf = fopen(log_path, "w");

    while (1) {
        int n = read(pipefd[0], buf, sizeof(buf));
        if (n <= 0) break;

        fwrite(buf, 1, n, logf);
        fflush(logf);

        printf("Buffered %d bytes for %s\n", n, id);
    }

    fclose(logf);

    waitpid(pid, NULL, 0);

    printf("Container %s exited\n", id);
    //  UNREGISTER FROM KERNEL MODULE
int fd_unreg = open("/dev/container_monitor", O_RDWR);
if (fd_unreg >= 0) {
    struct monitor_request req;

    req.pid = pid;
    strncpy(req.container_id, id, sizeof(req.container_id));
    req.container_id[sizeof(req.container_id) - 1] = '\0';

    ioctl(fd_unreg, MONITOR_UNREGISTER, &req);
    close(fd_unreg);
}

    f = fopen("containers.txt", "a");
    if (f) {
        fprintf(f, "%s %d exited\n", id, pid);
        fclose(f);
    }
}

// ---------------- COMMANDS ----------------

//  RUN (foreground with limits)
int cmd_run(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: run <id> <rootfs> <cmd> [--soft-mib X --hard-mib Y]\n");
        return 1;
    }

    char *id = argv[2];
    char *rootfs = argv[3];
    char *cmd = argv[4];

    int soft_mib = 10;
    int hard_mib = 100;

    // parse optional args
    for (int i = 5; i < argc; i++) {
        if (strcmp(argv[i], "--soft-mib") == 0 && i + 1 < argc) {
            soft_mib = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--hard-mib") == 0 && i + 1 < argc) {
            hard_mib = atoi(argv[++i]);
        }
    }

    run_container(id, rootfs, cmd, soft_mib, hard_mib);
    return 0;
}

//  START (background)
int cmd_start(int argc, char *argv[]) {
    if (argc < 5) return 1;

    pid_t pid = fork();

    if (pid == 0) {
        run_container(argv[2], argv[3], argv[4], 10, 100);
        exit(0);
    } else {
        printf("Started container %s (PID %d)\n", argv[2], pid);
    }
    return 0;
}

int cmd_ps() {
    FILE *f = fopen("containers.txt", "r");
    char id[32], state[32];
    int pid;

    printf("ID\tPID\tSTATE\n");
    while (f && fscanf(f, "%s %d %s", id, &pid, state) == 3)
        printf("%s\t%d\t%s\n", id, pid, state);

    if (f) fclose(f);
    return 0;
}

int cmd_logs(int argc, char *argv[]) {
    char path[100];
    snprintf(path, sizeof(path), "logs/%s.log", argv[2]);

    FILE *f = fopen(path, "r");
    char buf[256];

    while (f && fgets(buf, sizeof(buf), f))
        printf("%s", buf);

    if (f) fclose(f);
    return 0;
}

// ---------------- MAIN ----------------

int main(int argc, char *argv[]) {
    mkdir("logs", 0755);

    if (argc < 2) {
        printf("Usage: run/start/ps/logs\n");
        return 1;
    }

    if (strcmp(argv[1], "run") == 0)
        return cmd_run(argc, argv);
    else if (strcmp(argv[1], "start") == 0)
        return cmd_start(argc, argv);
    else if (strcmp(argv[1], "ps") == 0)
        return cmd_ps();
    else if (strcmp(argv[1], "logs") == 0)
        return cmd_logs(argc, argv);

    return 0;
}