
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"


int main(int argc, char* argv[])
{
    int socket_fd;                  // socket file descriptor
    struct addrinfo hints;          // addrinfo data
    struct addrinfo *server;        // getaddrinfo return
    int ret;                        // function return
    char tfbuf[CLI_PARAM_BUF_LEN];            // buffer for text file

    memset(&hints, 0, sizeof(hints));     // clear struct

    // in case wrong program usage
    if(argc != 3) {
        printf("usage: [client hostname] [file path]\n");
        exit(1);
    }

    hints.ai_family = AF_INET;            // IPv4
    hints.ai_socktype = SOCK_STREAM;      // TCP

    // get info from user input
    if((ret = getaddrinfo(argv[1], PORT, &hints, &server)) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    // create socket
    if((socket_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol)) == -1) {
        perror("client: socket");
        exit(1);
    }

    // connect to socket
    if(connect(socket_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("client: connect");
        close(socket_fd);
        exit(1);
    }

    // free memory
    freeaddrinfo(server);

    FILE *fp = NULL;

    fp = fopen(argv[2], "r");
    if(fp == NULL) {
        perror("Bad file path");
        close(socket_fd);
        exit(1);
    }

    // clear buffer
    bzero(tfbuf, CLI_PARAM_BUF_LEN);

    int bsize = 0;

    // send file
    while((bsize = fread(tfbuf, sizeof(char), CLI_PARAM_BUF_LEN, fp)) > 0) {
        if(send(socket_fd, tfbuf, bsize, 0) < 0) {
            perror("Failed to send file");
            close(socket_fd);
            exit(1);
        }
    }

    close(socket_fd);

    exit(0);
}
