#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s cmd arg\n", argv[0]);
        exit(2);
    }

    execlp(argv[1], argv[1], argv[2], NULL);

    perror("execlp");
    exit(1);

    /* Execution should never reach here */
    return 1;
}
