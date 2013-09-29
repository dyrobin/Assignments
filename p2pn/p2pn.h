#ifndef UTIL_H
#define UTIL_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
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
enum DLEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

extern enum DLEVEL debuglv;

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
    unsigned int        bp;
    struct list_head    list;
};


/* Logging */
#define p2plog(debug_level, ...) p2plog_all(debug_level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)


/* Compare if the two node are equal */
#define node_eq(n1, n2) \
            (memcmp(&(n1)->ip, &(n2)->ip, sizeof(struct in_addr)) == 0 \
            && memcmp(&(n1)->lport, &(n2)->lport, sizeof(uint16_t)) == 0)

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

void
p2plog_all(enum DLEVEL lv, const char *file, const int line,
	   const char *function, char *fmt, ...);

/* wrapper of the malloc() */
void *Malloc(size_t size);

/* Get the message id from a P2P message */
uint32_t get_msgid(struct msgstore *ms);

/* Initialize a waiting node */
void wtn_init(struct wtnode_meta *wtn);

/* Search a waiting node by its socket descriptor */
struct wtnode_meta *wtn_find_by_connfd(int connfd);

/* Check if the waiting node with given IP and port number exists
 * in the waiting node list.
 */
int wtn_contains(struct in_addr *addr, int lport);

/* Initialize a neighbor(peer) node */
void nm_init(struct node_meta *nm);

/* Search a neighbor node by its socket descriptor */
struct node_meta *nm_find_by_connfd(int connfd);

/* Check if the neighbor node with given IP and port number exists
 * in the neighbor node list.
 */
int nm_contains(struct in_addr *addr, int lport);

/* A utility function to conver a 'sockaddr' structure to string */
int sock_pton(struct sockaddr_in *addr, char *addstr);

/* Add a new waiting node to the waiting node list */
void wt_list_add(struct wtnode_meta *new_wt);

/* Delete the waiting node from the waiting node list */
void wt_list_del(struct wtnode_meta *wt);

/* Add a new neighbor node to the neighbor node list */
void nm_list_add(struct node_meta *new_nm);

/* Delete the neighbor node from the neighbor node list */
void nm_list_del(struct node_meta *nm);

/* Create a new item for the msgstore list */
struct msgstore *msg_new(void *msg, unsigned int len, int fromfd);

/* Free the memory of a msgstore item */
void free_msg(struct msgstore *ms);

/* Find a stored message in a given msgstore list by its message id */
struct msgstore *find_stored_msg(struct msgstore *head, uint32_t msgid);

/* Garbage Collect (gc) a msgstore list.
 * Messages received for more than 10 seconds will be freed.
 */
void gc_msgstore(struct msgstore *head);

/* Add a message to the global msgstore for incoming messages */
int push_g_recv_msg(int connfd, void *msg, unsigned int len);

/* Create a new peer cache for a new socket descriptor */
int create_peer_cache(int connfd);

/* Remove a peer cache for a given socket descriptor */
int remove_peer_cache(int connfd);

/* Find the peer cache by the given socket descriptor */
struct peer_cache *get_peer_cache(int connfd);


#endif
