
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#include "server.h"


volatile sig_atomic_t forest = NEED_TO_RUN;

/**
 * @brief SIGCHLD handler
 */
void sigchld_handler(int signo)
{
    // avoid overwriting errno
    int pre_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = pre_errno;
}


/**
 * @brief SIGTERM handler
 */
void sigterm_handler(int signo)
{
    forest = NEED_TO_STOP;

    sleep(1);

    exit(0);
}


/**
 * @brief SIGHUP handler
 */
void sighup_handler(int signo)
{
    forest = NEED_TO_STOP;

    sleep(1);

    exit(0);
}


int main(void)
{
    if(daemon(0, 0) == -1)
    {
        perror("daemon error");
        exit(1);
    }

    int socket_fd;                          // listen socket file descriptor
    int new_socket_fd;                      // new connection file descriptor
    struct addrinfo hints;                  // hints for own address
    struct addrinfo *server;                // filled address structure
    struct sockaddr_storage remote_addr;    // remote address
    socklen_t addr_size;                    // remote address size
    struct sigaction sa;                    // signal action
    int ret;                                // function return
    const int optval = 1;                   // set sock option

    // clear structs
    memset(&hints, 0, sizeof(hints));
    memset(&sa, 0, sizeof(sa));


    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    // remove dead processes
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction sigchld");
        exit(1);
    }

    // terminate
    sa.sa_handler = sigterm_handler;
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction sigterm");
        exit(1);
    }

    // restart
    sa.sa_handler = sighup_handler;
    if(sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction sighup");
        exit(1);
    }

    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // local IP

    // get IP info
    if((ret = getaddrinfo(NULL, PORT, &hints, &server)) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    // create socket
    if((socket_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol)) == -1) {
        perror("server: socket");
        exit(1);
    }

    // socket options
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
        close(socket_fd);
        perror("setsockopt");
        exit(1);
    }

    // bind socket
    if(bind(socket_fd, server->ai_addr, server->ai_addrlen) == -1) {
        close(socket_fd);
        perror("server: bind");
        exit(1);
    }

    // free memory
    freeaddrinfo(server);

    // prepare to accept connections
    if(listen(socket_fd, SERV_PARAM_BACKLOG) == -1) {
        close(socket_fd);
        perror("listen");
        exit(1);
    }

    // main part
    while(forest == NEED_TO_RUN) {
        addr_size = sizeof(remote_addr);

        // open new connection to communicate
        if((new_socket_fd = accept(socket_fd, (struct sockaddr*)&remote_addr, &addr_size)) == -1) {
            perror("accept");
            exit(1);
        }

        // separate process for each connection
        if(fork() == 0) {
            // child process
            // this socket is no longer needed here
            close(socket_fd);

            // get time for file name
            // struct timespec spec;
            char rfname[SERV_PARAM_FILE_NAME];
            // char rftime[SERV_PARAM_TIME_DIGITS];

            memset(&rfname, 0, sizeof(rfname));
            // memset(&rftime, 0, sizeof(rftime));

            // // create timestamp string
            // clock_gettime(CLOCK_REALTIME, &spec);
            // spec.tv_nsec %= SERV_PARAM_TIME_DIV;
            // sprintf(rftime, "%lu", spec.tv_nsec);

            // // create file name string like "\home\%USER%\receive_xxxxxx.txt"
            // strcpy(rfname, "/home/");
            // strcat(rfname, getlogin());
            // strcat(rfname, "/receive_");
            // strcat(rfname, rftime);
            // strcat(rfname, ".txt");

            strcpy(rfname, "/tmp/test.txt");

            // create new file
            FILE *fp = NULL;
            if((fp = fopen(rfname, "a")) == NULL) {
                perror("fopen");
                exit(1);
            }

            // receive data from client
            char recbuf[SERV_PARAM_BUF_LEN];
            int bsize_read = 0;
            int bsize_write = 0;
            memset(&recbuf, 0, sizeof(recbuf));

            // read received data
            while((bsize_read = recv(new_socket_fd, recbuf, SERV_PARAM_BUF_LEN, 0)) != 0) {
                if(bsize_read < 0) {
                    perror("Receive file error");
                    break;
                }

                // write received data to file
                bsize_write = fwrite(recbuf, sizeof(char), bsize_read, fp);

                if(bsize_write < bsize_read) {
                    perror("File write failed");
                    break;
                }

                memset(&recbuf, 0, sizeof(recbuf));
            }

            fclose(fp);

            // this socket is no longer needed here
            close(new_socket_fd);

            exit(0);
        }

        // parent
        close(new_socket_fd);
    }

    exit(0);
}
