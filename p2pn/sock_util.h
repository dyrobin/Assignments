#ifndef SOCK_UTIL_H
#define SOCK_UTIL_H

#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>


typedef struct sockaddr SA;

int
connect_pto(int fd, SA * addr, socklen_t size, int timeout_sec);

int
Socket(int family, int type, int protocol);

int
Bind(int fd, const SA *addr, socklen_t addrlen);

int
Listen(int sockfd, int backlog);

int
Accept(int sockfd, SA *addr, socklen_t *addrlen);

int
Close(int sockfd);

ssize_t
Read(int fd, void *ptr, size_t nbytes);

void
Write(int fd, void *ptr, size_t nbytes);

char *
sock_ntop(struct sockaddr_in *addr);

char *
sock_ntop2(struct in_addr *ip, int port);

#endif
