#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int sockfd, servlen, n;
    struct sockaddr_un serv_addr;
    char buffer[82];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s path message\n", argv[0]);
        exit(2);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, argv[1]);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("connect");
        exit(1);
    }

    memset(buffer, 0, 82);
    strncpy(buffer, argv[2], 80);
    write(sockfd, buffer, strlen(buffer));

    n = read(sockfd, buffer, 80);
    write(1, buffer, n);
}
