#define _SVID_SOURCE
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    struct dirent **namelist;
    int i, n;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s chdir scandir\n", argv[0]);
        exit(2);
    }

    if (*argv[1] && chdir(argv[1]) == -1) {
        perror("chdir");
        exit(1);
    }

    n = scandir(argv[2], &namelist, 0, alphasort);
    if (n < 0) {
        perror("scandir");
        exit(1);
    }
    else {
        for (i=0; i<n; i++) {
            printf("%s\n", namelist[i]->d_name);
            free(namelist[i]);
        }
        free(namelist);
    }

    return 0;
}

