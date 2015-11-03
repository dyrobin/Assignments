#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "list.h"
#include "proto.h"
#include "util.h"

extern enum LOGLEVEL        g_loglv;        /* Logging level */

extern struct key_value     g_kv_list;      /* List of key/value pairs */
extern struct peer_cache    g_pc_list;      /* List of peer caches */
extern struct message       g_msg_list;     /* List of messages */

extern struct nb_node       g_nb_list;      /* List of neighbor nodes */
extern int                  g_nb_list_size; /* Size of neighbor node list */

extern struct wt_node       g_wt_list;      /* List of waiting nodes */
extern int                  g_wt_list_size; /* Size of waiting node list */


/* wrapper of the malloc() */
static void *
Malloc(size_t size)
{
    void *res;

    if ((res = malloc(size)) == NULL) {
        perror("malloc error");
        exit(1);
    }

    return res;
}

/******************************************************************************/
/* Logging */
void
p2plog_all(enum LOGLEVEL lv, const char *file, const int line,
	       const char *function, char *fmt, ...)
{
    va_list ap;
    FILE *target = (lv == ERROR) ? stderr : stdout;

    if (lv >= g_loglv) {
        va_start(ap, fmt);
        fprintf(target, "%s:%d::%s() ", file, line, function);
        vfprintf(target, fmt, ap);
        va_end(ap);
    }
    return;
}

void
p2plog_env()
{
    struct nb_node *nb;
    struct wt_node *wt;

    char buf[XS_LEN];

    p2plog(DEBUG, "::::Waiting List::::[%d]\n", g_wt_list_size);
    list_for_each_entry(wt, &g_wt_list.list, list) {
        p2plog(DEBUG, "%10d | %16s:%5d | nq = %5d | urgent = %5d\n",
           wt->connfd,
           inet_ntop(AF_INET, &wt->ip, buf, sizeof(buf)),
           ntohs(wt->lport),
           wt->status,
           wt->urgent);
    }

    p2plog(DEBUG, "::::Neighbor List::::[%d]\n", g_nb_list_size);
    list_for_each_entry(nb, &g_nb_list.list, list) {
        p2plog(DEBUG, "%10d | %16s:%5d\n",
           nb->connfd,
           inet_ntop(AF_INET, &nb->ip, buf, sizeof(buf)),
           ntohs(nb->lport));
    }
    p2plog(DEBUG, "\n\n");
}


/******************************************************************************/
/* key/value pairs */

int
g_kv_list_load_from_file(char *filename)
{
    FILE *fp;

    if((fp = fopen(filename, "r")) == NULL) {
        p2plog(ERROR, "Failed to open file: %s\n", filename);
        return -1;
    }

    char buf[MSG_MAX];
    char *key, *value;
    int keylen;
    while (fgets(buf, MSG_MAX, fp)) {
        if (buf[strlen(buf) - 1] != '\n') {
            /* Too long line*/
            p2plog(ERROR, "Line too long in file: %s\n", filename);
            fclose(fp);
            return -1;
        }

        key = strtok(buf, " ");
        value = strtok(NULL, " ");

        keylen = strlen(key);
        if (keylen > KEY_MAX - 1) {
            p2plog(ERROR, "Key too long in file: %s\n", filename);
            continue;
        }

        struct key_value *kv;
        kv = (struct key_value *) Malloc(sizeof(struct key_value));
        memcpy(kv->key, key, keylen);
        kv->key[keylen] = '\0';
        kv->value = (uint32_t)strtoul(value, NULL, 16);

        list_add(&kv->list, &g_kv_list.list);
        p2plog(INFO, "Add key/value %s = 0x%08X\n", kv->key, kv->value);
    }

    fclose(fp);
    return 0;
}

/* search value by key obtained from QUERY message */
uint32_t
g_kv_list_search(void *msg, unsigned int len)
{
    unsigned int keylen;
    keylen = len - HLEN;
    if (keylen > KEY_MAX) {
        p2plog(ERROR, "key is too long, length %d\n", keylen);
        return 0;
    }

    char buf[M_LEN];
    memcpy(buf, ((char*)msg) + HLEN, keylen);
    buf[keylen] = '\0';

    struct key_value *kv;
    list_for_each_entry(kv, &g_kv_list.list, list) {
        p2plog(DEBUG, "buf = %s, key = %s\n", buf, kv->key);
        if (strcmp(kv->key, buf) == 0) {
            p2plog(INFO, "QUERY \"%s\" matches\n", buf);
            return kv->value;
        }
    }

    return 0;
}


/******************************************************************************/
/* Peer cache */

/* Create a new peer cache for a new socket descriptor */
struct peer_cache *
pc_new(int connfd)
{
    struct peer_cache *pc;

    pc = (struct peer_cache *)Malloc(sizeof(struct peer_cache));
    memset(pc, 0, sizeof(struct peer_cache));

    pc->connfd = connfd;
    
    return pc;
}

/* Add a new peer cache to global peer cache list */
void
g_pc_list_add(struct peer_cache *pc)
{
    if (pc) list_add(&pc->list, &g_pc_list.list);
}

/* Delete a peer cache from global peer cache list */
void
g_pc_list_del(struct peer_cache *pc)
{
    if (pc) {
        list_del(&pc->list);
        free(pc);        
    }
}

/* Search a peer cache by its socket descriptor in global peer cache list */
struct peer_cache *
g_pc_list_find_by_connfd(int connfd)
{
    struct peer_cache *pc;

    list_for_each_entry(pc, &g_pc_list.list, list) {
        if (pc->connfd == connfd) return pc;
    }

    return NULL;
}

/* Delete a peer cache found by socket descriptor from global peer cache list */
void
g_pc_list_remove_by_connfd(int connfd)
{
    struct peer_cache *pc;
    pc = g_pc_list_find_by_connfd(connfd);
    g_pc_list_del(pc);
}

/******************************************************************************/
/* Messages */

/* Get the message id from a P2P message */
static uint32_t
get_msgid(struct message *msg)
{
    struct P2P_h *ph;
    ph = (struct P2P_h *) (msg->content);
    return ph->msg_id;
}

/* Create a new message */
struct message *
msg_new(void *content, unsigned int len, int fromfd)
{
    struct message *msg;

    msg = (struct message *) Malloc(sizeof(struct message));
    memset(msg, 0, sizeof(struct message));

    msg->content = (unsigned char *) Malloc(len);
    memcpy(msg->content, content, len);
    msg->len = len;
    msg->fromfd = fromfd;
    gettimeofday(&msg->tv, NULL);
    
    return msg;
}

/* Free the memory of a message */
void
msg_free(struct message *msg)
{
    if (msg) {
        free(msg->content);
        free(msg);        
    }
}

/* Add a message to global message list */
void
g_msg_list_add(struct message *msg)
{
    if (msg) list_add(&msg->list, &g_msg_list.list);
}

/* Garbage Collect (gc) global message list.
 * Messages received for more than 10 seconds will be freed.
 */
void
g_msg_list_gc()
{
    struct message *msg, *msgtmp;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    list_for_each_entry_safe(msg, msgtmp, &g_msg_list.list, list) {
        if (tv.tv_sec - msg->tv.tv_sec > 10) {
            p2plog(DEBUG, "free msg %08X in the msg list\n", get_msgid(msg));
            list_del(&msg->list);
            msg_free(msg);
        }
    }
}

/* Find a message by its message id in global message list */
struct message *
g_msg_list_find_by_id(uint32_t msgid)
{
    struct message *msg;

    list_for_each_entry(msg, &g_msg_list.list, list) {
        if (get_msgid(msg) == msgid)
            return msg;
    }
    return NULL;
}


/******************************************************************************/
/* Waiting nodes */

/* Create a new waiting node */
struct wt_node *
wt_new(int connfd, struct in_addr *ipaddr, uint16_t lport)
{
    struct wt_node * wt;

    wt = (struct wt_node *)Malloc(sizeof(struct wt_node));
    memset(wt, 0, sizeof(struct wt_node));

    wt->connfd = connfd;
    wt->ip = *ipaddr;
    wt->lport = lport;
    wt->urgent = 0;
    wt->status = -1;
    wt->ts = time(NULL);

    return wt;
}

/* Add a new waiting node to global waiting list */
void
g_wt_list_add(struct wt_node *wt)
{
    if (wt) {
        list_add(&wt->list, &g_wt_list.list);
        g_wt_list_size++;
    }
}

/* Delete the waiting node from global waiting list */
void
g_wt_list_del(struct wt_node *wt)
{
    if (wt) {
        list_del(&wt->list);
        g_wt_list_size--;
        free(wt);
    }
}

/* Search a waiting node by its socket descriptor in global waiting list*/
struct wt_node *
g_wt_list_find_by_connfd(int connfd)
{
    struct wt_node *wt;

    list_for_each_entry(wt, &g_wt_list.list, list) {
        if (wt->connfd == connfd) return wt;
    }

    return NULL;
}

/* Search a waiting node by peer's IP address and port in global waiting list */
struct wt_node *
g_wt_list_find_by_peer(struct in_addr *ipaddr, uint16_t lport)
{
    struct wt_node *wt;
    struct wt_node tgt;

    tgt.ip = *ipaddr;
    tgt.lport = lport;

    list_for_each_entry(wt, &g_wt_list.list, list) {
        if (node_eq(wt, &tgt)) return wt;
    }

    return NULL;
}


/******************************************************************************/
/* Neighbour nodes */

/* Create a new neighbour node */
struct nb_node *
nb_new(int connfd, struct in_addr *ipaddr, uint16_t lport)
{
    struct nb_node * nb;

    nb = (struct nb_node *)Malloc(sizeof(struct nb_node));
    memset(nb, 0, sizeof(struct nb_node));

    nb->connfd = connfd;
    nb->ip = *ipaddr;
    nb->lport = lport;
    nb->ts = time(NULL);

    return nb;
}

/* Add a new neighbor to global neighbor list */
void
g_nb_list_add(struct nb_node *nb)
{
    if (nb) {
        list_add(&nb->list, &g_nb_list.list);
        g_nb_list_size++;
    }
}

/* Delete the neighbor from global neighbor list */
void
g_nb_list_del(struct nb_node *nb)
{
    if (nb) {
        list_del(&nb->list);
        g_nb_list_size--;
        free(nb);
    }
}

/* Search a neighbor by its socket descriptor in global neighbour list */
struct nb_node *
g_nb_list_find_by_connfd(int connfd)
{
    struct nb_node *nb;

    list_for_each_entry(nb, &g_nb_list.list, list) {
        if (nb->connfd == connfd) return nb;
    }

    return NULL;
}

/* Search a neighbour by peer's IP address and port in global neighbour list */
struct nb_node *
g_nb_list_find_by_peer(struct in_addr *ipaddr, uint16_t lport)
{
    struct nb_node *nb;
    struct nb_node tgt;

    tgt.ip = *ipaddr;
    tgt.lport = lport;

    list_for_each_entry(nb, &g_nb_list.list, list) {
        if (node_eq(nb, &tgt)) return nb;
    }

    return NULL;
}
