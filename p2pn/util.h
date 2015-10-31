#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "proto.h"

#define  XS_LEN      32
#define   S_LEN      64
#define   M_LEN     128
#define   L_LEN     256

#define KEY_MAX      64
#define MSG_MAX    2048
#define BUF_MAX    4096

/******************************************************************************/
/* Logging Level */
enum LOGLEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR
};
/* Logging */
#define p2plog(log_level, ...) \
        p2plog_all(log_level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
void p2plog_all(enum LOGLEVEL lv, const char *file, const int line,
                const char *function, char *fmt, ...);
void p2plog_env();


/******************************************************************************/
/* The structure of key/value pairs */
struct key_value {
    char key[KEY_MAX];
    uint32_t value;
    struct list_head list;
};

int g_kv_list_load_from_file(char *filename);

/* search value by key obtained from QUERY message */
uint32_t g_kv_list_search(void *msg, unsigned int len);


/******************************************************************************/
/* The structure of peer cache */
struct peer_cache {
    int                 connfd;
    unsigned char       recvbuf[BUF_MAX];
    unsigned int        bp;
    struct list_head    list;
};

/* Create a new peer cache for a new socket descriptor */
struct peer_cache * pc_new(int connfd);

/* Add a new peer cache to global peer cache list */
void g_pc_list_add(struct peer_cache *pc);

/* Delete a peer cache from global peer cache list */
void g_pc_list_del(struct peer_cache *pc);

/* Search a peer cache by its socket descriptor in global peer cache list */
struct peer_cache * g_pc_list_find_by_connfd(int connfd);

/* Delete a peer cache found by socket descriptor from global peer cache list */
void g_pc_list_remove_by_connfd(int connfd);

/******************************************************************************/
/* The structure of stored messages */
struct message {
    void *content;
    int len;
    int fromfd;                         /* zero:     from itself. 
                                         * Non-zero: from others. */
    struct timeval tv;
    struct list_head list;
};

/* Create a new message */
struct message * msg_new(void *content, unsigned int len, int fromfd);

/* Free the memory of a message */
void msg_free(struct message *msg);

/* Add a message to global message list */
void g_msg_list_add(struct message *msg);

/* Garbage Collect (gc) global message list.
 * Messages received for more than 10 seconds will be freed.
 */
void g_msg_list_gc();

/* Find a message by its message id in global message list*/
struct message * g_msg_list_find_by_id(uint32_t msgid);


/******************************************************************************/
/* The structure of waiting node
 * These nodes are not neighbors yet, but can be picked and to establish 
 * connection.
 */
struct wt_node {
    int                 connfd;
    struct in_addr      ip;
    uint16_t            lport;
    int                 urgent;     /* Is it an urgent one? eg. for bootstrap */
    int                 status;     /* 0: new peer that connected to us, 
                                          but no JOIN Request yet.
                                       1: peer discovered, JOIN Request sent.
                                       2: we are waiting for JOIN 
                                          Request/Accept. */
    time_t              ts;         /* Timestamp */
    struct list_head    list;
};

/* Check if the waiting node is connected */
#define wt_connected(wt) ((wt)->connfd > 0)

/* Check if the waiting node requires a urgent connection */
#define wt_urgent(wt)         ((wt)->urgent == 1)

/* Turn on the urgent flag of the waiting node */
#define wt_urgent_set(wt)     ((wt)->urgent = 1)

/* Turn off the urgent flag of the waiting node */
#define wt_urgent_reset(wt)   ((wt)->urgent = 0)

/* Check if the JOIN message has been sent to the waiting node */
#define wt_requested(wt)      ((wt)->status > 0)

/* Create a new waiting node */
struct wt_node * wt_new(int connfd, struct in_addr *ipaddr, uint16_t lport);

/* Add a new waiting node to global waiting list */
void g_wt_list_add(struct wt_node *wt);

/* Delete the waiting node from global waiting list */
void g_wt_list_del(struct wt_node *wt);

/* Search a waiting node by its socket descriptor in global waiting list*/
struct wt_node * g_wt_list_find_by_connfd(int connfd);

/* Search a waiting node by peer's IP address and port in global waiting list */
struct wt_node * g_wt_list_find_by_peer(struct in_addr *ipaddr, uint16_t lport);


/******************************************************************************/
/* The structure of neighbour nodes */
struct nb_node {
    int                 connfd;
    struct in_addr      ip;
    uint16_t            lport;
    time_t              ts;
    struct list_head    list;
};

/* Compare if the two node are equal */
#define node_eq(n1, n2) \
    (memcmp(&(n1)->ip, &(n2)->ip, sizeof(struct in_addr)) == 0 && \
     memcmp(&(n1)->lport, &(n2)->lport, sizeof(uint16_t)) == 0)

/* Create a new neighbour node */
struct nb_node * nb_new(int connfd, struct in_addr *ipaddr, uint16_t lport);

/* Add a new neighbor to global neighbor list */
void g_nb_list_add(struct nb_node *nb);

/* Delete the neighbor from global neighbor list */
void g_nb_list_del(struct nb_node *nb);

/* Search a neighbor by its socket descriptor in global neighbour list */
struct nb_node * g_nb_list_find_by_connfd(int connfd);

/* Search a neighbour by peer's IP address and port in global neighbour list */
struct nb_node * g_nb_list_find_by_peer(struct in_addr *ipaddr, uint16_t lport);

#endif