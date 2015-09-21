#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "sock_util.h"


/**
 * Wrapper for @c socket()
 */
int
Socket(int domain, int type, int protocol)
{
    int n;

    if ((n = socket(domain, type, protocol)) < 0) {
        perror("Socket()");
        exit(1);
    }

    return n;
}


/**
 * Wrapper for @c bind()
 */
int
Bind(int sockfd, const SA *addr, socklen_t addrlen)
{
    int n;

    if ((n = bind(sockfd, addr, addrlen)) < 0) {
        perror("Bind()");
        exit(1);
    }

    return n;
}


/**
 * Wrapper for @c listen()
 */
int
Listen(int sockfd, int backlog)
{
    int n;

    if ((n = listen(sockfd, backlog) < 0)) {
        perror("Listen()");
        exit(1);
    }

    return n;
}


/**
 * Wrapper for @c accept()
 */
int
Accept(int sockfd, SA *addr, socklen_t *addrlen)
{
    int n;

    if ((n = accept(sockfd, addr, addrlen)) < 0)
        perror("Accpet()");

    return n;
}


/**
 *  Wrapper for @c connect()
 */
int 
Connect(int sockfd, const SA *addr, socklen_t addrlen)
{
    int n;

    if ((n = connect(sockfd, addr, addrlen)) < 0)
        perror("Connect()");

    return n;
}


/**
 * Complete Connect() within a specific time
 * 
 * @param sockfd      the socket file descriptor
 * @param addr        the address of the remote side
 * @param salen       the size of the @c addr
 * @param time        the time to complete in seconds
 * 
 * Note that Connect() may take quite a long time to complete if the remote 
 * side is unreachable. This function gives a chance to close the connection
 * quickly. Both Connect() and ConnectWithin() will block the process for a 
 * while, the differece between them is how long they will block.
 */
int
ConnectWithin(int sockfd, const SA *addr, socklen_t salen, int time)
{
    int n;

    int flags;
    if((flags = fcntl(sockfd, F_GETFL, 0)) < 0) {
        perror("ConnectWithin(), fcntl GETFL");
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("ConnectWithin(), fcntl SETFL O_NON_BLOCK");
        return -1;
    }

    if ((n = connect(sockfd, addr, salen)) < 0)
        if (errno != EINPROGRESS) {
            perror("ConnectWithin(), connect");
            return -1;
        }

    if (n == 0) {
        /* connect() succeeds immediately */
        if (fcntl(sockfd, F_SETFL, flags) < 0) {
            perror("ConnectWithin(), fnctl FSETFL");
            return -1;
        }
        return 0;
    }

    /* Complete the connect() asynchronously by selecting socket for writing */
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);

    struct timeval tv;
    tv.tv_sec = time;
    tv.tv_usec = 0;

    if ((n = select(sockfd + 1, NULL, &wset, NULL,
                    time ? &tv : NULL)) == 0) {
        perror("ConnectWithin(), select timeout");
        return -1;
    }

    if (n < 0) {
        perror("ConnectWithin(), select");
        return -1;
    }

    if (FD_ISSET(sockfd, &wset)) {
        int error;
        socklen_t len;

        error = 0;
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            perror("ConnectWithin(), getsockopt SO_ERROR");
            return -1;
        }

        if (error) {
            errno = error;
            perror("ConnectWithin(), socket error");
            return -1;
        }

        /* connect() succeeds asynchronously */
        if (fcntl(sockfd, F_SETFL, flags) < 0) {
            perror("ConnectWithin(), fnctl FSETFL");
            return -1;
        }
        return 0;
    }

    perror("ConnectWithin(), sockfd should be set");
    return -1;
}


/**
 * Wrapper for @c close()
 */
int
Close(int fd)
{
    int n;

    if ((n = close(fd)) < 0)
        perror("Close()");

    return n;
}


/**
 * Wrapper for @c read()
 */
ssize_t
Read(int fd, void *buf, size_t count)
{
    ssize_t n;

    if ((n = read(fd, buf, count)) < 0)
        perror("Read()");

    return n;
}


/**
 * Wrapper for @c write().
 */
ssize_t
Write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    if ((n = write(fd, buf, count)) < 0)
        perror("Write()");

    return n;
}


/**
 * Wrapper for @c getsockname()
 */
int
GetSockName(int sockfd, SA *addr, socklen_t *addrlen)
{
    int n;

    if ((n = getsockname(sockfd, addr, addrlen)) < 0)
        perror("GetSockName()");

    return n;
}


/**
 * @c inet_ntop() extension that converts port as well into presentation
 */
#define SOCK_ADDRSTRLEN (16 + 6) /* Format ddd.ddd.ddd.ddd:ddddd */
const char *
sock_ntop(const struct sockaddr_in *addr_ip, char *dst, socklen_t size)
{
    if (size < SOCK_ADDRSTRLEN) {
        perror("sock_ntop(), no enough dst");
        return NULL;
    }

    const char *n;
    if ((n = inet_ntop(AF_INET, &addr_ip->sin_addr, dst, size)) == NULL) {
        perror("sock_ntop(), inet_ntop");
        return NULL;
    }

    sprintf(dst + strlen(n), ":%d", ntohs(addr_ip->sin_port));
    return dst;
}
