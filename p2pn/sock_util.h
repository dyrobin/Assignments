#ifndef SOCK_UTIL_H
#define SOCK_UTIL_H

#include <sys/socket.h>
#include <netinet/in.h>


typedef struct sockaddr SA;

int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const SA *addr, socklen_t addrlen);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, SA *addr, socklen_t *addrlen);

int Connect(int sockfd, const SA *addr, socklen_t addrlen);

int ConnectWithin(int sockfd, const SA *addr, socklen_t salen, int time);

int Close(int fd);

ssize_t Read(int fd, void *buf, size_t count);

ssize_t Write(int fd, const void *buf, size_t count);

int GetSockName(int sockfd, SA *addr, socklen_t *addrlen);

const char *sock_ntop(const struct in_addr *addr, const unsigned short port);

int sock_pton(const char *str, struct in_addr *addr, uint16_t *port);

#endif
