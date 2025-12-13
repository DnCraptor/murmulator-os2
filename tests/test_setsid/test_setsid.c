#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--child") == 0) {
        pid_t pid = getpid();
        pid_t sid = getsid(0);
        if (pid != sid) {
            printf("child sid mismatch\n");
            return 1;
        }
        return 0;
    }

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);

    char *child_argv[] = {
        argv[0],
        "--child",
        NULL
    };

    pid_t pid;
    int r = posix_spawnp(&pid, argv[0], NULL, &attr, child_argv, 0);
    if (r != 0) {
        printf("posix_spawnp failed: %d\n", r);
        return 1;
    }

    int status;
    if (waitpid(pid, &status, 0) != pid) {
        printf("waitpid failed\n");
        return 1;        
    }
    if ((status >> 8) != 0) {
        printf("child exited with %d\n", status >> 8);
        return 1;
    }

    printf("setsid OK\n");
    return 0;
}
