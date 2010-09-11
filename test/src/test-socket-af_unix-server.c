#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SUN_LEN
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, servlen, n;
    socklen_t clilen;
    struct sockaddr_un cli_addr, serv_addr, name_addr;
    socklen_t namelen;
    char buffer[80];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s path\n", argv[0]);
        exit(2);
    }

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, argv[1]);
    servlen = SUN_LEN(&serv_addr);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("bind");
        exit(1);
    }

    namelen = SUN_LEN(&name_addr);
    if (getsockname(sockfd, (struct sockaddr*)&name_addr, &namelen) < 0) {
        perror("getsockname");
        exit(1);
    }

    listen(sockfd, 1);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        perror("accept");
        exit(1);
    }

    n = read(newsockfd, buffer, 80);
    if (write(1, buffer, n) < 0) {
        perror("write(1)");
        exit(1);
    }
    if (write(newsockfd, buffer, n) < 0) {
        perror("write(newsockfd)");
        exit(1);
    }

    return 0;
}
