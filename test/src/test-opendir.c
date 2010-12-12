#include <sys/types.h>
#include <dirent.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main (int argc, char *argv[]) {
    DIR *d;
    struct dirent *ent;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        exit(2);
    }

    if ((d = opendir(argv[1])) == NULL) {
        perror("opendir");
        exit(1);
    }

    while ((ent = readdir(d)) != NULL) {
        printf("%s\n", ent->d_name);
    }

    if ((closedir(d)) == -1) {
        perror("closedir");
        return 1;
    }

    return 0;
}
