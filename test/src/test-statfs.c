#include "config.h"

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    struct statfs statfsb;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        exit(2);
    }

    if (statfs(argv[1], &statfsb)) {
        perror("statfs");
        exit(1);
    }

    printf("%ld\n", statfsb.f_blocks);

    return 0;
}
