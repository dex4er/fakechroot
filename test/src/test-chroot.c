#include "config.h"

#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef DEBUG_TEST_SRC
#define debug(msg) fprintf(stderr, msg)
#else
#define debug(msg)
#endif

int main (int argc, char *argv[]) {
    char *ls_cmd = BIN_LS;
    char *ls_id_root_argv[] = { ls_cmd, "-id", "/", NULL };
    char *ls_id_root_env[] = { NULL };

    pid_t child_pid1, child_pid2, w1, w2;
    int wstatus1, wstatus2;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s chdir_1 chroot_1 chdir_2 chroot_2\n", argv[0]);
        exit(2);
    }

    debug("*** chdir[1]\n");
    if (*argv[1] && chdir(argv[1]) == -1) {
        perror("chdir[1]");
        exit(1);
    }

    debug("*** chroot[1]\n");
    if (chroot(argv[2]) == -1) {
        perror("chroot[1]");
        exit(1);
    }

    debug("*** fork[1]\n");
    if ((child_pid1 = fork()) == -1) {
        perror("fork[1]");
        exit(1);
    }

    if (child_pid1 == 0) {
        debug("*** execve[1]\n");
        execve(ls_cmd, ls_id_root_argv, ls_id_root_env);
        perror("execve[1]");
        exit(1);
    }

    do {
        debug("*** waitpid[1]\n");
        w1 = waitpid(child_pid1, &wstatus1, WUNTRACED | WCONTINUED);
        if (w1 == -1) {
            perror("waitpid[1]");
            exit(1);
        }
    } while (!WIFEXITED(wstatus1) && !WIFSIGNALED(wstatus1));

    debug("*** chdir[2]\n");
    if (*argv[3] && chdir(argv[3]) == -1) {
        perror("chdir[2]");
        exit(1);
    }

    debug("*** chroot[2]\n");
    if (chroot(argv[4]) == -1) {
        perror("chroot[2]");
        exit(1);
    }

    debug("*** fork[2]\n");
    if ((child_pid2 = fork()) == -1) {
        perror("fork[2]");
        exit(1);
    }

    if (child_pid2 == 0) {
        debug("*** execve[2]\n");
        execve(ls_cmd, ls_id_root_argv, ls_id_root_env);
        perror("execve[2]");
        exit(1);
    }

    do {
        debug("*** waitpid[2]\n");
        w2 = waitpid(child_pid2, &wstatus2, WUNTRACED | WCONTINUED);
        if (w2 == -1) {
            perror("waitpid[2]");
            exit(1);
        }
    } while (!WIFEXITED(wstatus2) && !WIFSIGNALED(wstatus2));

    return 0;
}
