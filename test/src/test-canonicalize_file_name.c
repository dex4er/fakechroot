#include "config.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

#ifndef HAVE_CANONICALIZE_FILE_NAME
char *canonicalize_file_name (const char *path) {
        return realpath(path, NULL);
}
#endif

int main (int argc, char *argv[]) {
        char *path;

        if (argc != 2) {
        fprintf(stderr, "Usage: %s command\n", argv[0]);
        exit(2);
    }

    if ((path = canonicalize_file_name(argv[1])) == NULL) {
        perror("canonicalize_file_name");
        exit(1);
    }

    printf("%s\n", path);

    return 0;
}
