#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE
#include <sys/types.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

extern char **environ;

int main (int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s cmd arg\n", argv[0]);
        exit(2);
    }

    {
        pid_t pid;
        char *newargv[] = { argv[1], argv[2], NULL };
        int status;

        status = posix_spawn(&pid, argv[1], NULL, NULL, newargv, environ);

        if (status == 0) {
            if (waitpid(pid, &status, 0) != -1) {
                return status;
            } else {
                perror("waitpid");
            }
        } else {
            fprintf(stderr, "posix_spawn() failed: %s\n", strerror(status));
            return status;
        }
    }

    exit(1);

    /* Execution should never reach here */
    return 1;
}
