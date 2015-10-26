#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    char *template, *path;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s template\n", argv[0]);
        exit(2);
    }

    template = argv[1];

    if ((path = mktemp(template)) == NULL || !*path) {
        perror("mktemp");
        exit(1);
    }
    printf("%s\n", path);

    return 0;
}
