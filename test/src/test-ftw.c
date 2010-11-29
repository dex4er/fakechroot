#include <ftw.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


static int callback(const char *fpath, const struct stat *sb, int typeflag) {
    printf("%s\n", fpath);
    return 0;
}


int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        exit(2);
    }

    if (ftw(argv[1], callback, 16) == -1) {
        perror("ftw");
        exit(1);
    }

    return 0;
}
