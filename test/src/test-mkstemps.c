#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    char *template;
    int suffixlen;
    int fd;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s template suffixlen\n", argv[0]);
        exit(2);
    }

    template = argv[1];
    suffixlen = atoi(argv[2]);

    if ((fd = mkstemps(template, suffixlen)) == -1 || !*template) {
        perror("mkstemps");
        exit(1);
    }
    printf("%s\n", template);

    return 0;
}
