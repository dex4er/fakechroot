#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif


int main (int argc, char *argv[]) {
    FILE *fp;
    char path[PATH_MAX];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s cmd\n", argv[0]);
        exit(2);
    }

    fp = popen(argv[1], "r");
    if (fp == NULL) {
        perror("popen");
        exit(1);
    }

    while (fgets(path, PATH_MAX, fp) != NULL) {
        printf("%s", path);
    }

    pclose(fp);

    return 0;
}
