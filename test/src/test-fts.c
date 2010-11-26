#include <fts.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main (int argc, char *argv[]) {
    FTS *tree;
    FTSENT *node;

    char **paths = argv + 1;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path...]\n", argv[0]);
        exit(2);
    }

    if ((tree = fts_open(paths, 0, 0)) == NULL) {
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
