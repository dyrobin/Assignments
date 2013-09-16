#ifndef UTIL_H
#define UTIL_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include "list.h"
#include "proto.h"
#include "sock_util.h"


/*--------------- P2PN global variable declarations ----------------------*/

/* The listening socket for the P2PN */
extern int listen_fd;

/* Current the listening address */
extern struct sockaddr_in ltn_addr;

/* The list of neighbor nodes */
extern struct node_meta neighbors;
/* The size of the neighbor node list */
extern int nm_size;

/* The list of waiting nodes */
extern struct wtnode_meta waiting_nodes;
/* The size of the waiting node list */
extern int wt_size;

/* Cache received data to distinguish the message boundary */
extern struct peer_cache pr_cache;

/* Save messages to prevent loop in forwarding */
extern struct msgstore g_recvmsgs;
extern struct msgstore sentmsgs;

/* The key/value pairs in current node */
extern struct keyval localdata;

/* number of peers presented in the peer advertisement */
extern int n_peer_ad;

/* Debug Level */
extern enum DLEVEL debuglv;
enum DLEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

#define LISTENQ 5
#define MAXLINE 4096
#define SLEN    128
#define MLEN    256
#define LLEN    512

#define KEYLEN  128

/* A list to store P2P messages */
struct msgstore {
    struct timeval ts;
    struct list_head list;
    void *msg;
    int len;
    int fromfd;
};

/* store local key/value pairs */
struct keyval {
    char key[KEYLEN];
    uint32_t value;
    struct list_head list;
};

/* The information of connected nodes
 * which are neighbors (or peers)
 */
struct node_meta {
    int                 connfd;
    struct in_addr      ip;
    uint16_t            lport;
    struct msgstore     msgs;
    struct list_head    list;
};

/* The information of nodes which are in waiting list.
 * These nodes are not neighbors yet, but can be
 * pick and establish connection.
 */
struct wtnode_meta {
    struct in_addr      ip;
    uint16_t            lport;
    /* Times of request already sent */
    int                 nrequest;
    /* Is it an urgent one? eg. for bootstrap */
    int                 urgent;
    int                 connfd;
    struct list_head    list;
};

/* Store received bytes for each peer */
struct peer_cache {
    struct in_addr      ip;
    uint16_t            lport;
    int                 connfd;
    unsigned char       recvbuf[MAXLINE];
    int                 bp;
    struct list_head    list;
};


/* Logging */
#define p2plog(debug_level, ...) p2plog_all(debug_level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

static inline void
p2plog_all(enum DLEVEL lv, const char *file, const int line,
	   const char *function, char *fmt, ...)
{
    va_list ap;
    FILE *target = (lv == ERROR) ? stderr : stdout;

    if (lv >= debuglv) {
        va_start(ap, fmt);
	fprintf(target, "%s:%d::%s() ",
		file, line, function);
        vfprintf(target, fmt, ap);
        va_end(ap);
    }
    return;
}

/* wrapper of the malloc() */
static inline void *
Malloc(size_t size)
{
    void *res;

    if ( (res = malloc(size)) == NULL) {
        perror("malloc error");
        exit(1);
    }

    return res;
}

/* Get the message id from a P2P message */
static inline uint32_t
get_msgid(struct msgstore *ms)
{
    struct P2P_h *ph;
    ph = (struct P2P_h *) (ms->msg);
    return ph->msg_id;
}

/* Compare if the two node are equal */
#define node_eq(n1, n2) \
            (bcmp(&(n1)->ip, &(n2)->ip, sizeof(struct in_addr)) == 0 \
            && bcmp(&(n1)->lport, &(n2)->lport, sizeof(uint16_t)) == 0)

/* Check if the waiting node is connected */
#define wtn_conn_established(wtn) \
            ((wtn)->connfd != 0)

/* Check if the waiting node requires a urgent connection */
#define wtn_urgent(wtn) \
    ((wtn)->urgent == 1)

/* Turn on the urgent flag of the waiting node */
#define wtn_urgent_set(wtn) \
    ((wtn)->urgent = 1)

/* Turn off the urgent flag of the waiting node */
#define wtn_urgent_reset(wtn) \
    ((wtn)->urgent = 0)

/* Check if the JOIN message has been sent to the waiting node */
#define wtn_requested(wtn) \
    ((wtn)->nrequest > -1)

/* Initialize a waiting node */
static inline void
wtn_init(struct wtnode_meta *wtn)
{
    bzero(wtn, sizeof(struct wtnode_meta));
    wtn->nrequest = -1;
}

/* Search a waiting node by its socket descriptor */
static inline struct wtnode_meta *
wtn_find_by_connfd(int connfd)
{
    struct wtnode_meta *wtn, *wtn_tgt;

    wtn_tgt = NULL;
    list_for_each_entry(wtn, &waiting_nodes.list, list) {
        if (wtn->connfd == connfd) {
            wtn_tgt = wtn;
            break;
        }
    }

    return wtn_tgt;
}

/* Check if the waiting node with given IP and port number exists
 * in the waiting node list.
 */
static inline int
wtn_contains(struct in_addr *addr, int lport)
{
    struct wtnode_meta *wtn;
    struct wtnode_meta tmp;
    int match;

    match = 0;
    tmp.ip = *addr;
    tmp.lport = lport;

    list_for_each_entry(wtn, &waiting_nodes.list, list) {
        if (node_eq(wtn, &tmp)) {
            match = 1;
            break;
        }
    }

    return match;
}

/* Initialize a neighbor(peer) node */
static inline void
nm_init(struct node_meta *nm)
{
    bzero(nm, sizeof(struct node_meta));
    INIT_LIST_HEAD(&nm->msgs.list);
}

/* Search a neighbor node by its socket descriptor */
static inline struct node_meta *
nm_find_by_connfd(int connfd)
{
    struct node_meta *nm, *nm_tgt;

    nm_tgt = NULL;
    list_for_each_entry(nm, &neighbors.list, list) {
        if (nm->connfd == connfd) {
            nm_tgt = nm;
            break;
        }
    }
    return nm_tgt;
}

/* Check if the neighbor node with given IP and port number exists
 * in the neighbor node list.
 */
static inline int
nm_contains(struct in_addr *addr, int lport)
{
    struct node_meta *nm;
    struct node_meta tmp;
    int match;

    match = 0;
    tmp.ip = *addr;
    tmp.lport = lport;

    list_for_each_entry(nm, &neighbors.list, list) {
        if (node_eq(nm, &tmp)) {
            match = 1;
            break;
        }
    }

    return match;
}

/* A utility function to conver a 'sockaddr' structure to string */
static inline int
sock_pton(struct sockaddr_in *addr, char *addstr)
{
    int read_fail, iplen;
    char *sp;
    char buf[SLEN];

    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    read_fail = 0;
    if (addstr != NULL) {
        if((sp = strstr(addstr, ":")) != NULL) {
            iplen = sp - addstr;
            if (iplen < 50 && iplen > 0 && iplen < strlen(addstr)) {
                strncpy(buf, addstr, iplen);
                buf[iplen] = '\0';
                if ((inet_pton(AF_INET, buf, &addr->sin_addr)) == -1)
                    read_fail = 1;
                addr->sin_port = htons(atoi(sp+1));
            } else
                read_fail = 1;
        } else
            read_fail = 1;
    } else {
        read_fail = 1;
    }

    return read_fail;
}

/* Add a new waiting node to the waiting node list */
static inline void
wt_list_add(struct wtnode_meta *new_wt)
{
    list_add(&new_wt->list, &waiting_nodes.list);
    wt_size++;
}

/* Delete the waiting node from the waiting node list */
static inline void
wt_list_del(struct wtnode_meta *wt)
{
    list_del(&wt->list);
    wt_size--;
    free(wt);
}

/* Add a new neighbor node to the neighbor node list */
static inline void
nm_list_add(struct node_meta *new_nm)
{
    list_add(&new_nm->list, &neighbors.list);
    nm_size++;
}

/* Delete the neighbor node from the neighbor node list */
static inline void
nm_list_del(struct node_meta *nm)
{
    list_del(&nm->list);
    nm_size--;
    free(nm);
}

/* Create a new item for the msgstore list */
static inline struct msgstore *
msg_new(void *msg, int len, int fromfd)
{
    struct msgstore *nwmsg;

    nwmsg = (struct msgstore *) Malloc(sizeof(struct msgstore));
    bzero(nwmsg, sizeof(struct msgstore));

    gettimeofday(&nwmsg->ts, NULL);
    nwmsg->len = len;
    nwmsg->fromfd = fromfd;
    nwmsg->msg = (unsigned char *) Malloc(len);
    bcopy(msg, nwmsg->msg, len);

    return nwmsg;
}

/* Free the memory of a msgstore item */
static inline void
free_msg(struct msgstore *ms)
{
    free(ms->msg);
    free(ms);
}

/* Find a stored message in a given msgstore list by its message id */
static inline struct msgstore *
find_stored_msg(struct msgstore *head, uint32_t msgid)
{
    struct msgstore *ms;

    list_for_each_entry(ms, &head->list, list) {
        if (get_msgid(ms) == msgid)
            return ms;
    }
    return NULL;
}

/* Garbage Collect (gc) a msgstore list.
 * Messages received for more than 10 seconds will be freed.
 */
static inline void
gc_msgstore(struct msgstore *head)
{
    struct msgstore *ms, *mstp;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    list_for_each_entry_safe(ms, mstp, &head->list, list) {
        if (tv.tv_sec - ms->ts.tv_sec > 10) {
	    p2plog(DEBUG, "free msg %08X in the msg storage\n", get_msgid(ms));
            list_del(&ms->list);
            free_msg(ms);
        }
    }
}

/* Add a message to the global msgstore for incoming messages */
static inline int
push_g_recv_msg(int connfd, void *msg, int len)
{
    struct msgstore *ms;

    ms = msg_new(msg, len, connfd);
    list_add(&ms->list, &g_recvmsgs.list);

    return 0;
}

/* Create a new peer cache for a new socket descriptor */
static inline int
create_peer_cache(int connfd)
{
    struct peer_cache *pc;
    p2plog(DEBUG, "create new cache for socket fd %d\n", connfd);
    pc = (struct peer_cache *)Malloc(sizeof(struct peer_cache));
    bzero(pc, sizeof(struct peer_cache));
    pc->connfd = connfd;
    list_add(&pc->list, &pr_cache.list);
    return 0;
}

/* Remove a peer cache for a given socket descriptor */
static inline int
remove_peer_cache(int connfd)
{
    struct peer_cache *pc;

    list_for_each_entry(pc, &pr_cache.list, list) {
        if (pc->connfd == connfd) {
	    p2plog(DEBUG, "delete cache for socket fd %d\n", connfd);
	    list_del(&pc->list);
	    free(pc);
	    break;
        }
    }
    return 0;
}

/* Find the peer cache by the given socket descriptor */
static inline struct peer_cache *
get_peer_cache(int connfd)
{
    struct peer_cache *pc;

    list_for_each_entry(pc, &pr_cache.list, list) {
        if (pc->connfd == connfd)
            return pc;
    }
    return NULL;
}

#endif
