#include "config.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <sys/statvfs.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    struct statvfs statvfsb;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        exit(2);
    }

    if (statvfs(argv[1], &statvfsb)) {
        perror("statvfs");
        exit(1);
    }

    printf("%ld\n", statvfsb.f_bsize);

    return 0;
}
