#include <stdio.h>
#include <stdlib.h>

#include "p2pn.h"

void
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
void *
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
uint32_t
get_msgid(struct msgstore *ms)
{
    struct P2P_h *ph;
    ph = (struct P2P_h *) (ms->msg);
    return ph->msg_id;
}

/* Initialize a waiting node */
void
wtn_init(struct wtnode_meta *wtn)
{
    memset(wtn, 0, sizeof(struct wtnode_meta));
    wtn->nrequest = -1;
}

/* Search a waiting node by its socket descriptor */
struct wtnode_meta *
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

struct wtnode_meta *
wtn_find_by_joinid(uint32_t msgid)
{
    struct wtnode_meta *wtn, *wtn_tgt;

    wtn_tgt = NULL;
    list_for_each_entry(wtn, &waiting_nodes.list, list) {
        if (wtn->join_msgid == msgid) {
            wtn_tgt = wtn;
            break;
        }
    }
    return wtn_tgt;
}

/* Check if the waiting node with given IP and port number exists
 * in the waiting node list.
 */
int
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
void
nm_init(struct node_meta *nm)
{
    memset(nm, 0, sizeof(struct node_meta));
    INIT_LIST_HEAD(&nm->msgs.list);
}

/* Search a neighbor node by its socket descriptor */
struct node_meta *
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
int
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
int
sock_pton(struct sockaddr_in *addr, char *addstr)
{
    int read_fail, iplen;
    char *sp;
    char buf[SLEN];

    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    read_fail = 0;
    if (addstr != NULL) {
        if((sp = strstr(addstr, ":")) != NULL) {
            iplen = sp - addstr;
            if (iplen < 50 && iplen > 0 && iplen < (int)strlen(addstr)) {
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
void
wt_list_add(struct wtnode_meta *new_wt)
{
    list_add(&new_wt->list, &waiting_nodes.list);
    wt_size++;
}

/* Delete the waiting node from the waiting node list */
void
wt_list_del(struct wtnode_meta *wt)
{
    list_del(&wt->list);
    wt_size--;
    free(wt);
}

/* Add a new neighbor node to the neighbor node list */
void
nm_list_add(struct node_meta *new_nm)
{
    list_add(&new_nm->list, &neighbors.list);
    nm_size++;
}

/* Delete the neighbor node from the neighbor node list */
void
nm_list_del(struct node_meta *nm)
{
    list_del(&nm->list);
    nm_size--;
    free(nm);
}

/* Create a new item for the msgstore list */
struct msgstore *
msg_new(void *msg, unsigned int len, int fromfd)
{
    struct msgstore *nwmsg;

    nwmsg = (struct msgstore *) Malloc(sizeof(struct msgstore));
    memset(nwmsg, 0, sizeof(struct msgstore));

    gettimeofday(&nwmsg->ts, NULL);
    nwmsg->len = len;
    nwmsg->fromfd = fromfd;
    nwmsg->msg = (unsigned char *) Malloc(len);
    memcpy(nwmsg->msg, msg, len);

    return nwmsg;
}

/* Free the memory of a msgstore item */
void
free_msg(struct msgstore *ms)
{
    free(ms->msg);
    free(ms);
}

/* Find a stored message in a given msgstore list by its message id */
struct msgstore *
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
void
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
int
push_g_recv_msg(int connfd, void *msg, unsigned int len)
{
    struct msgstore *ms;

    ms = msg_new(msg, len, connfd);
    list_add(&ms->list, &g_recvmsgs.list);

    return 0;
}

/* Create a new peer cache for a new socket descriptor */
int
create_peer_cache(int connfd)
{
    struct peer_cache *pc;
    p2plog(DEBUG, "create new cache for socket fd %d\n", connfd);
    pc = (struct peer_cache *)Malloc(sizeof(struct peer_cache));
    memset(pc, 0, sizeof(struct peer_cache));
    pc->connfd = connfd;
    list_add(&pc->list, &pr_cache.list);
    return 0;
}

/* Remove a peer cache for a given socket descriptor */
int
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
struct peer_cache *
get_peer_cache(int connfd)
{
    struct peer_cache *pc;

    list_for_each_entry(pc, &pr_cache.list, list) {
        if (pc->connfd == connfd)
            return pc;
    }
    return NULL;
}

