#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main (int argc, char *argv[]) {
    FTS *tree;
    FTSENT *node;

    int options;
    char **paths;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s options path [path...]\n", argv[0]);
        exit(2);
    }

    options = atoi(argv[1]);
    paths = argv + 2;

    if ((tree = fts_open(paths, options, 0)) == NULL) {
        perror("fts_open");
        exit(1);
    }

    while ((node = fts_read(tree)) != NULL) {
        if (node->fts_info & (FTS_F|FTS_D)) {
            printf("%s\n", node->fts_path);
        }
    }
    if (errno) {
        perror("fts_read");
        exit(1);
    }

    if ((fts_close(tree)) == -1) {
        perror("fts_close");
        return 1;
    }

    return 0;
}
