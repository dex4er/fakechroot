#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, servlen, clilen, n;
    struct sockaddr_un cli_addr, serv_addr;
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
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0) {
        perror("bind");
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
    write(1, buffer, n);
    write(newsockfd, buffer, n);
}
