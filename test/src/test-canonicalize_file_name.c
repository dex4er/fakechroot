#include "config.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#ifndef HAVE_CANONICALIZE_FILE_NAME
char *canonicalize_file_name (const char *path) {
    char *resolved = malloc(PATH_MAX * 2);
#ifdef HAVE___REALPATH_CHK
    return __realpath_chk(path, resolved, PATH_MAX*2);
#else
    return realpath(path, resolved);
#endif
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
