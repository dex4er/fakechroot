#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
	struct passwd *pwd;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s username\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	pwd = getpwnam(argv[1]);
	if (pwd == NULL) {
		if (errno == 0) {
			printf("Not found\n");
		} else {
			perror("getpwnam");
		}
		exit(EXIT_FAILURE);
	}

	printf("%jd\n", (intmax_t)(pwd->pw_uid));
	exit(EXIT_SUCCESS);
}
