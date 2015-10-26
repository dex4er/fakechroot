#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    char *path, *resolved_path, *returned_path;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s path [resolved_path]\n", argv[0]);
        exit(2);
    }

    path = argv[1];
    resolved_path = argc > 2 ? argv[2] : NULL;

    if ((returned_path = realpath(path, resolved_path)) == NULL) {
        perror("realpath");
        exit(1);
    }
    printf("%s\n", returned_path);

    return 0;
}

