#define _XOPEN_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    char *path;
    int fd;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/ptmx\n", argv[0]);
        exit(2);
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    if ((path = ptsname(fd)) == NULL) {
        perror("ptsname");
        exit(1);
    }

    printf("%s\n", path);

    return 0;
}
