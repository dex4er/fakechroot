#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

int main (int argc, char *argv[]) {
    struct stat stab;
    char buf[1024];
    ssize_t sz;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s /path/to/symlink\n", argv[0]);
        exit(2);
    }

    if (lstat(argv[1], &stab)) {
        perror("lstat");
        exit(1);
    }

#ifdef HAVE_LUTIMES
    struct timeval tv[2];
    if (lutimes(argv[1], tv)) {
        perror("lutimes");
        exit(1);
    }
#endif

    if ((stab.st_mode & S_IFMT) == S_IFREG) {
        printf("regular file size: %d\n", (int)stab.st_size);
        return 0;
    }

    if ((stab.st_mode & S_IFMT) != S_IFLNK) {
        fprintf(stderr, "not a symlink\n");
        exit(1);
    }

    if ((sz = readlink(argv[1], buf, sizeof(buf))) < 0) {
        perror("readlink");
        exit(1);
    }
    buf[sz] = '\0';

    printf("%s -> %s\n", argv[1], buf);
    printf("readlink size: %d\n", (int)sz);
    printf("stat size: %d\n", (int)stab.st_size);

    return 0;
}

