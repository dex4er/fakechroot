#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s chdir_1 chroot_1 system_1 chdir_2 chroot_2 system_2\n", argv[0]);
        exit(2);
    }

    fprintf(stderr, "*** chdir[1]\n");
    if (*argv[1] && chdir(argv[1]) == -1) {
        perror("chdir[1]");
        exit(1);
    }

    fprintf(stderr, "*** chroot[1]\n");
    if (chroot(argv[2]) == -1) {
        perror("chroot[1]");
        exit(1);
    }

    fprintf(stderr, "*** system[1]\n");
    if (system(argv[3]) == -1) {
        perror("system[1]");
        exit(1);
    }

    fprintf(stderr, "*** chdir[2]\n");
    if (*argv[4] && chdir(argv[4]) == -1) {
        perror("chdir[2]");
        exit(1);
    }

    fprintf(stderr, "*** chroot[1]\n");
    if (chroot(argv[5]) == -1) {
        perror("chroot[2]");
        exit(1);
    }

    fprintf(stderr, "*** system[1]\n");
    if (system(argv[6]) == -1) {
        perror("system[2]");
        exit(1);
    }

    return 0;
}
