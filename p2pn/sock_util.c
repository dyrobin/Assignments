#include "sock_util.h"


/**
 * Initiate a TCP connection with an optional timeout value.
 * 
 * @param fd          the socket descriptor
 * @param addr        the address of the remote side
 * @param salen       the size of the @c addr
 * @param timeout_sec timeout in seconds
 * @return            -1 on error, 0 otherwise.
 */
int
connect_pto(int fd, SA *addr, socklen_t salen, int timeout_sec)
{
    int flags, n, error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    error = 0;
    if((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        perror("connect_pto, fcntl GETFL");
        return (-1);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("connect_pto, fcntl SETFL O_NON_BLOCK");
        return (-1);
    }

    if ((n = connect(fd, addr, salen)) < 0)
        if (errno != EINPROGRESS)
            return (-1);

    if (n == 0)
        goto done;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    wset = rset;
    tval.tv_sec = timeout_sec;
    tval.tv_usec = 0;

    if ((n = select(fd + 1,
                    &rset, &wset, NULL,
                    timeout_sec ? &tval : NULL)) == 0) {
    /* SELECT TIME OUT */
        close(fd);
        errno = ETIMEDOUT;
        return (-1);
    }

    if (n < 0) {
    /* SELECT ERROR */
        perror("connect_pto, connect");
        return (-1);
    }

    if (FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset)) {
        len = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return (-1);
    } else {
        printf("select error: sockfd not set");
    }

done:
    if (fcntl(fd, F_SETFL, flags) < 0) {
    /* remove the non_block setting */
        perror("connect_pto, fnctl FSETFL");
        return (-1);
    }

    if (error) {
        close(fd);
        errno = error;
        return (-1);
    }

    return (0);
}

/* ===========START of wrap socket function ========*/

/**
 * Wrapper for @c socket().
 *
 * @param family
 * @param type
 * @param protocol
 * @return
 */
int
Socket(int family, int type, int protocol)
{
    int n;

    if ((n = socket(family, type, protocol)) < 0) {
        perror("socket error");
        exit(1);
    }

    return n;
}

/**
 * Wrapper for @c bind().
 *
 * @param fd
 * @param addr
 * @param addrlen
 * @return
 */
int
Bind(int fd, const SA *addr, socklen_t addrlen)
{
    int n;

    if ((n = bind(fd, addr, addrlen)) < 0) {
        perror("bind error");
        exit(1);
    }

    return n;
}

/**
 * Wrapper for @c listen().
 *
 * @param sockfd
 * @param backlog
 * @return
 */
int
Listen(int sockfd, int backlog)
{
    int n;

    if ((n = listen(sockfd, backlog) < 0)) {
        perror("listen error");
        exit(1);
    }

    return n;
}

/**
 * Wrapper for @c accept().
 *
 * @param sockfd
 * @param addr
 * @param addrlen
 * @return
 */
int
Accept(int sockfd, SA *addr, socklen_t *addrlen)
{
    int n;

    if ((n = accept(sockfd, addr, addrlen)) < 0)
        perror("accpet error");

    return n;
}

/**
 * Wrapper for @c close().
 *
 * @param sockfd
 * @return
 */
int
Close(int sockfd)
{
    int n;

    if ((n = close(sockfd)) < 0)
        perror("close error");

    return n;
}

/**
 * Wrapper for @c read().
 *
 * @param fd
 * @param ptr
 * @param nbytes
 * @return
 */
ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = read(fd, ptr, nbytes)) == -1) {
        perror("read error");
    }

    return n;
}

/**
 * Wrapper for @c write().
 *
 * @param fd
 * @param ptr
 * @param nbytes
 * @return
 */
void
Write(int fd, void *ptr, size_t nbytes)
{
    if (write(fd, ptr, nbytes) != nbytes) {
        perror("write error");
    }
}
/* ===========END of wrap socket function ========*/

char *
sock_ntop(struct sockaddr_in *addr)
{
    static char buf[128];
    inet_ntop(AF_INET, &addr->sin_addr, buf, 128);
    sprintf(buf + strlen(buf), ":%d", ntohs(addr->sin_port));
    return buf;
}

char *
sock_ntop2(struct in_addr *ip, int port)
{
    struct sockaddr_in addr;
    addr.sin_addr = *ip;
    addr.sin_port = port;
    return sock_ntop(&addr);
}
