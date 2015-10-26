#include "config.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif

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

#ifdef __clang__
# pragma clang diagnostic ignored "-Wformat"
#endif

    printf((sizeof(statvfsb.f_bsize) == 8 ? "%ld\n" : "%d\n"), statvfsb.f_bsize);

    return 0;
}
