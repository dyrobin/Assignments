#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "proto.h"

/* wrapper of the malloc() */
void *Malloc(size_t size);


/* Debug Level */
enum DLEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR
};
/* Logging */
#define p2plog(debug_level, ...) \
        p2plog_all(debug_level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
void p2plog_all(enum DLEVEL lv, const char *file, const int line,
                const char *function, char *fmt, ...);


/* A list to store P2P messages */
struct msgstore {
    int fromfd;
    void *msg;
    int len;
    struct timeval ts;
    struct list_head list;
};

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

/* Get the message id from a P2P message */
uint32_t get_msgid(struct msgstore *ms);


/* The information of nodes which are in waiting list.
 * These nodes are not neighbors yet, but can be
 * pick and establish connection.
 */
struct wtnode_meta {
    int                 connfd;
    struct in_addr      ip;
    uint16_t            lport;
    int                 urgent;     /* Is it an urgent one? eg. for bootstrap */
    int                 nrequest;   /* 0: new peer that connected to us, 
                                          but no Join Request yet.
                                       1: discovered peer, Join Request sent.
                                       2: we are waiting for Join 
                                          Request/Accept. */
/*    uint32_t            join_msgid;*/ /* For self loop detection. Not a good 
                                       solution and the better one is to detect
                                       loop in handle_pong_message(). */
    struct list_head    list;
};

/* Check if the waiting node is connected */
#define wtn_conn_established(wtn) ((wtn)->connfd != 0)

/* Check if the waiting node requires a urgent connection */
#define wtn_urgent(wtn)         ((wtn)->urgent == 1)

/* Turn on the urgent flag of the waiting node */
#define wtn_urgent_set(wtn)     ((wtn)->urgent = 1)

/* Turn off the urgent flag of the waiting node */
#define wtn_urgent_reset(wtn)   ((wtn)->urgent = 0)

/* Check if the JOIN message has been sent to the waiting node */
#define wtn_requested(wtn)      ((wtn)->nrequest > -1)

/* Initialize a waiting node */
void wtn_init(struct wtnode_meta *wtn);

/* Search a waiting node by its socket descriptor */
struct wtnode_meta *wtn_find_by_connfd(int connfd);

/* Search a waiting node by our Join Message ID */
/* struct wtnode_meta *wtn_find_by_joinid(uint32_t msgid); */

/* Check if the waiting node with given IP and port number exists
 * in the waiting node list.
 */
int wtn_contains(struct in_addr *addr, uint16_t lport);

/* Add a new waiting node to the waiting node list */
void wt_list_add(struct wtnode_meta *new_wt);

/* Delete the waiting node from the waiting node list */
void wt_list_del(struct wtnode_meta *wt);



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

/* Compare if the two node are equal */
#define node_eq(n1, n2) \
    (memcmp(&(n1)->ip, &(n2)->ip, sizeof(struct in_addr)) == 0 && \
     memcmp(&(n1)->lport, &(n2)->lport, sizeof(uint16_t)) == 0)

/* Initialize a neighbor(peer) node */
void nm_init(struct node_meta *nm);

/* Search a neighbor node by its socket descriptor */
struct node_meta *nm_find_by_connfd(int connfd);

/* Check if the neighbor node with given IP and port number exists
 * in the neighbor node list.
 */
int nm_contains(struct in_addr *addr, uint16_t lport);

/* Add a new neighbor node to the neighbor node list */
void nm_list_add(struct node_meta *new_nm);

/* Delete the neighbor node from the neighbor node list */
void nm_list_del(struct node_meta *nm);


#define MAXLINE 4096
/* Store received bytes for each peer */
struct peer_cache {
    int                 connfd;
    struct in_addr      ip;
    uint16_t            lport;
    unsigned char       recvbuf[MAXLINE];
    unsigned int        bp;
    struct list_head    list;
};

/* Create a new peer cache for a new socket descriptor */
int create_peer_cache(int connfd);

/* Remove a peer cache for a given socket descriptor */
int remove_peer_cache(int connfd);

/* Find the peer cache by the given socket descriptor */
struct peer_cache *get_peer_cache(int connfd);


#define KEYLEN  128
/* store local key/value pairs */
struct keyval {
    char key[KEYLEN];
    uint32_t value;
    struct list_head list;
};

uint32_t search_localdata(struct P2P_h *ph, unsigned int msglen);

#endif
