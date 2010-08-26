#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s command\n", argv[0]);
        exit(2);
    }

    if (system(argv[1]) == -1) {
        perror("system");
        exit(1);
    }

    return 0;
}
