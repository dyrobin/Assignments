#define _POSIX_C_SOURCE     2       /* getopt */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ifaddrs.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

#include "list.h"
#include "sock_util.h"
#include "util.h"
#include "proto.h"

/* Data structure */
struct key_value        g_kv_list;      /* List of key/value pairs */
struct peer_cache       g_pc_list;      /* List of peer caches */
struct message          g_msg_list;     /* List of messages */

struct nb_node          g_nb_list;      /* List of neighbor nodes */
int                     g_nb_list_size; /* Size of neighbor node list */

struct wt_node          g_wt_list;      /* List of waiting nodes */
int                     g_wt_list_size; /* Size of waiting node list */

struct ifaddrs         *g_ifaddrs;      /* List of all interfaces */

/* Node info */
enum LOGLEVEL           g_loglv = INFO; /* Logging level */
struct sockaddr_in      g_lstn_addr;    /* Listening address */
int                     g_ad_num;       /* Peers number in advertisement */
int                     g_auto_join;    /* Flag of auto join nodes */

static int              lstn_fd;        /* Listen socket */
static char            *search_key;     /* Search key */

/* Other static variables */
static int              peer_error;
static struct sigaction act;

/* Time for maintenance */
#define SELECT_SECONDS       3
#define  HBEAT_SECONDS       5
#define  PROBE_SECONDS       8
#define  QUERY_SECONDS      10
#define ZOMBIE_SECONDS      30

#define LISTEN_QUEUE         5
#define NEIGHBOUR_MAX        8


static void sig_pipe(int s)
{
    peer_error = 4;
    sigaction(s, &act, NULL);
}

/* Usage of the p2pn program
 */
static void
usage()
{
    printf("Usage: p2pn -l [ip:port] -f [kvfile] \n"
           "           [-s [search_key] -b [ip:port] -p [max_peers_in_pong]]\n"
           "           [-j]\n");
    printf("    -l: Listening address and port \n");
    printf("    -f: key/value data file \n");
    printf("    -s: Search key \n");
    printf("    -b: Bootstrap server address and port \n");
    printf("    -p: Max Number of neighbor entries in PONG \n");
    printf("    -j: Suppress auto join behaviour\n");
}

/**
 * handle messages in the peer_cache
 *
 * @param pc the peer cache
 * @return
 */
static int
recv_msg(struct peer_cache *pc)
{
    if (pc->bp < HLEN) {
        /* More data pending to parse header */
        return 0;
    }

    int from_neigh, connfd;

#define from_neigh() \
    (from_neigh == 1)
#define set_from_neigh() \
    (from_neigh = 1)
#define unset_from_neigh() \
    (from_neigh = 0)

    connfd = pc->connfd;
    unset_from_neigh();

    struct wt_node *wt;
    struct nb_node *nb;

    nb = g_nb_list_find_by_connfd(connfd);
    if (nb != NULL) {
        set_from_neigh();
    } else {
        wt = g_wt_list_find_by_connfd(connfd);
        if (wt == NULL) {
            p2plog(ERROR, "Receive a message from an unknown sender.\n");
            return 0;
        }
    }
    
    struct P2P_h *ph;
    ph = (struct P2P_h *) (pc->recvbuf);

    /* Validate message */
    if (ph->version != P_VERSION) {
        p2plog(ERROR, "Invalid version number: %d\n", ph->version);
        goto CLEAR_CACHE;
    }

    if (ph->ttl == 0 || ph->ttl > MAX_TTL) {
        p2plog(ERROR, "Invalid TTL: %d\n", ph->ttl);
        goto CLEAR_CACHE;
    }

    unsigned int msglen = HLEN + ntohs(ph->length);

    if (pc->bp < msglen) {
        /* More data pending to parse message */
        return 0;
    }

    if (from_neigh()) {
        p2plog(DEBUG, "From neighbour node: %s\n", 
               sock_ntop(&nb->ip, nb->lport));
        nb->ts = time(NULL);
    } else {
        p2plog(DEBUG, "From waiting node: %s\n", 
               sock_ntop(&wt->ip, wt->lport));
    }
    p2plog(DEBUG, "In MSG: [%08X], msg_type = %02X, len = %d, ttl = %d\n",
            ph->msg_id, ph->msg_type, ntohs(ph->length), ph->ttl);

    if (!from_neigh() &&
        ((ph->msg_type & MSG_JOIN) != MSG_JOIN)){
        /* msg is not from a established neighbor, and it is not a JOIN 
         * message, we should not allow this message. */
           p2plog(ERROR, "Receive Non-JOIN from a waiting node\n");
           goto CLEAR_MSG;
    }

    switch(ph->msg_type) {
        case MSG_PING:
            handle_ping_message(connfd, ph, msglen);
            break;

        case MSG_PONG:
            handle_pong_message(ph, msglen);
            break;

        case MSG_BYE:
            handle_bye_message(connfd);
            break;

        case MSG_JOIN:
            handle_join_message(connfd, ph, msglen);
            break;

        case MSG_QUERY:
            p2plog(DEBUG, "Receive QUERY MSG: [%08X], len = %d, from %s\n",
                   ph->msg_id, ntohs(ph->length), 
                   sock_ntop(&nb->ip, nb->lport));
            handle_query_message(connfd, ph, msglen);
        break;

        case MSG_QHIT:
            p2plog(DEBUG, "Receive QHIT MSG: [%08X], len = %d, from %s\n",
                   ph->msg_id, ntohs(ph->length), 
                   sock_ntop(&nb->ip, nb->lport));
            handle_query_hit(ph, msglen);
        break;

        default:
            p2plog(ERROR, "Receive a message with an invalid message type\n");
            goto CLEAR_MSG;
    }

    unsigned int i;
CLEAR_MSG:
    for (i = msglen; i < pc->bp; i++)
        pc->recvbuf[i - msglen] = pc->recvbuf[i];
    pc->bp -= msglen;

    return msglen;

CLEAR_CACHE:
        pc->bp = 0;
        return 0;
}


/**
 * Receive bytes from the remote side and save them in the peer cache.
 *
 * @param connfd  the socket fd for the remote side
 * @param buf     the buff which holds the received bytes
 * @param bufsize the number of the received bytes
 */
static void
recv_byte_stream(int connfd, char *buf, int bufsize)
{
    struct peer_cache *pc;
    int n;

    if((pc = g_pc_list_find_by_connfd(connfd)) == NULL) {
        p2plog(ERROR, "Peer cache not found, connfd = %d\n", connfd);
        peer_error = 2;
        return;
    }

    if ((MSG_MAX - pc->bp) < (unsigned)bufsize) {
        p2plog(ERROR, "Peer cache buffer full for connfd = %d\n", connfd);
        peer_error = 3;
        return;
    }

    memcpy(pc->recvbuf + pc->bp, buf, bufsize);
    pc->bp += bufsize;

    while ((n = recv_msg(pc)) != 0) {
        /* blank */
    }
}

/**
 * Handle pending peers in the waiting list.
 */
static void
handle_waiting_list(time_t now)
{
    struct wt_node *wt, *wt_tmp;
    struct sockaddr_in addr;
    int connfd;

    list_for_each_entry_safe(wt, wt_tmp, &g_wt_list.list, list) {
        /* Establish connections to newly discovered peers when
         * it becomes 'urgent'. */
        if (!wt_connected(wt) && wt_urgent(wt)) {
            memset(&addr, 0, sizeof(addr));
            memcpy(&addr.sin_addr, &wt->ip, sizeof(addr.sin_addr));
            addr.sin_port = wt->lport;
            addr.sin_family = AF_INET;

            if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
                wt->connfd = connfd;
                if (ConnectWithin(connfd, (SA *)&addr, sizeof(addr), 2) >= 0) {
                    send_join_message(connfd);
                    wt->status = 1;    /* Set to 1: Join Request sent */
                    wt->ts = now;
                    wt_urgent_reset(wt);
                    g_pc_list_add(pc_new(connfd));
                } else {
                    p2plog(ERROR, 
                           "Connection failed, drop waiting node %s, fd = %d\n", 
                           sock_ntop(&wt->ip, wt->lport), connfd);
                    Close(connfd);
                    g_wt_list_del(wt);
                }
            } else {
                p2plog(ERROR, "socket error\n");
                g_wt_list_del(wt);
            }
        }
    }

    /* kick those who neither send Join Request nor accept our Join */
    list_for_each_entry_safe(wt, wt_tmp, &g_wt_list.list, list) {
        if (now - wt->ts > (ZOMBIE_SECONDS >> 1)) {
            p2plog(INFO, "Zombie, drop waiting node %s, fd = %d\n", 
                   sock_ntop(&wt->ip, wt->lport), wt->connfd);
            if (wt_connected(wt)) {
                Close(wt->connfd);
                g_pc_list_remove_by_connfd(wt->connfd);                 
            }
            g_wt_list_del(wt);
        }
    }

    /* Currently, the only chance that a newly discovered peer can become
     * 'urgent' is when we are in need of more neighbours. */
    if (g_nb_list_size < NEIGHBOUR_MAX) {
        list_for_each_entry(wt, &g_wt_list.list, list) {
            /* Here we pick one such peer at a time. */
            if (!wt_connected(wt) && !wt_urgent(wt) && 
                !wt_requested(wt)) {
                wt_urgent_set(wt);
                break;
            }
        }
    }
}

static void
handle_neighbour_list(time_t now)
{
    /* Kick thouse zombie neighbours */
    struct nb_node *nb, *nb_tmp;
    list_for_each_entry_safe(nb, nb_tmp, &g_nb_list.list, list) {
        if (now - nb->ts > ZOMBIE_SECONDS) {
            p2plog(INFO, "Zombie, drop neighbour node %s, fd = %d\n", 
                   sock_ntop(&nb->ip, nb->lport), nb->connfd);
            Close(nb->connfd);
            g_pc_list_remove_by_connfd(nb->connfd);
            g_nb_list_del(nb);
        }
    }
}

/**
 * Maintain the p2p network.
 *
 * Maintain the stable and availability of the p2p network by
 * periodically send PING messages and renew its neighbor database.
 */
static void
network_maintain()
{   
    static time_t    hbeat_next;
    static time_t    probe_next;
    static time_t    query_next;

    struct nb_node *nb;
    time_t now = time(NULL);

    handle_neighbour_list(now); 
    handle_waiting_list(now);

    if (now > hbeat_next) {
        /* send heart beat to all neighbors */
        list_for_each_entry(nb, &g_nb_list.list, list) {
            send_ping_message(nb->connfd, PING_TTL_HB);
        }
        hbeat_next = now + HBEAT_SECONDS;
    }

    if (now > probe_next) {
        /* send neighbor query to all neighbors */
        list_for_each_entry(nb, &g_nb_list.list, list) {
            send_ping_message(nb->connfd, MAX_TTL);
        }
        /* send probe randomly to avoid receiving JOIN simultaneously */
        probe_next = now + PROBE_SECONDS + rand() % PROBE_SECONDS;
    }
    
    if (search_key != NULL && now > query_next) {
        /* search the network */
        send_query_message(search_key);
        query_next = now + QUERY_SECONDS;
    }
}

/**
 * The main loop for message receving and handling
 */
static int
node_loop()
{
    struct sockaddr_in cliaddr;
    socklen_t clisize;

    fd_set aset;
    int maxfd, connfd;

    struct timeval timeout;
    char buf[MSG_MAX];
    int n;

    struct nb_node *nb, *nb_tmp;
    struct wt_node *wt, *wt_tmp;
    
    int opt_recv_low = HLEN;


    for ( ; ; ) {
        /* Find max FD for select call */
        FD_ZERO(&aset);

        maxfd = lstn_fd;
        FD_SET(lstn_fd, &aset);
        list_for_each_entry(nb, &g_nb_list.list, list) {
            if (nb->connfd > maxfd) maxfd = nb->connfd;
            FD_SET(nb->connfd, &aset);
        }
        list_for_each_entry(wt, &g_wt_list.list, list) {
            if (wt_connected(wt)) {
                if (wt->connfd > maxfd) maxfd = wt->connfd;
                FD_SET(wt->connfd, &aset);
            }
        }

        /* Set select timeout to TICK seconds */
        timeout.tv_sec = SELECT_SECONDS;
        timeout.tv_usec = 0;

        if (select(maxfd + 1, &aset, NULL, NULL, &timeout) == -1) {
          perror("select()");
          p2plog(ERROR, "Failed to select\n");
          continue;
        }

        if (FD_ISSET(lstn_fd, &aset)) {
            /* New connection arrives */
            clisize = sizeof(cliaddr);
            connfd = Accept(lstn_fd, (SA *)&cliaddr, &clisize);

            if (connfd < 0) {
                p2plog(ERROR, "Accept() failed\n");
            } else {
                if (setsockopt(connfd, SOL_SOCKET, SO_RCVLOWAT, &opt_recv_low,
                               sizeof(int)) != 0) {
                    perror("setsockopt()");
                    p2plog(ERROR, "Failed to set socket OPT: SO_RCVLOWAT");
                } else {
            /**
             * Should not always create a waiting node when receiving a new
             * connection (Normally the JOIN Request is coming). The reason
             * for this is that the node sending JOIN Request by creating a 
             * new connection might have been already in the waiting list 
             * or even in the neighbor list. This happens when a node is added 
             * to the waiting list by handling PONG and later on that node 
             * starts to send JOIN. Therefore, a new waiting node can only be
             * created when it is not in either waiting list or neighbor list.
             * 
             * Solution: The new incoming connection should be stored in 
             * the third list different from neither waiting list nor neighbor 
             * list. When the following JOIN comes, we should check if waiting
             * list or neighbor list has already contained the node by 
             * identifying both IP address and listening port. If nothing in 
             * the lists, then a new entry of neighbor list can be allocated. 
             * Note that listening port is different from the port returned by
             * accept(). Therefore, wtn->lport will be updated when JOIN
             * message is handled. See handle_join_message() in detail.
             *
             * Bug is fixed here by merging the third list with waiting list, 
             * then separating them from each other when handling JOIN request 
             * message in handle_join_message().
             */
                    wt = wt_new(connfd, &cliaddr.sin_addr, cliaddr.sin_port);
                    wt->status = 0;  /* set to 0: new peer that connected to us,
                                      * but no Join Request yet */
                    p2plog(INFO, "Connection from %s, fd = %d\n",
                            sock_ntop(&cliaddr.sin_addr, cliaddr.sin_port),
                            wt->connfd);
                    /* save to waiting list */
                    g_wt_list_add(wt);
                    g_pc_list_add(pc_new(connfd));
                }
            }
        }

        /**
         * NOTE HERE!!! 
         * When list_for_each_entry_safe() is used, make sure only current 
         * entry can be deleted and other entries must be retained. Otherwise,
         * the link will be broken. This is really hard to check as there might
         * be many deep functions in the body of it. To make this true, you 
         * have to check all the inner functions.
         */ 

        /* Check all neighbor nodes if they are readable */
        list_for_each_entry_safe(nb, nb_tmp, &g_nb_list.list, list) {
            if (FD_ISSET(nb->connfd, &aset)) {
                peer_error = 0;
                if ((n = Read(nb->connfd, buf, MSG_MAX)) <= 0) {
                    if (n == 0) {
                        p2plog(INFO, 
                            "Disconnect from neighbour node: %s, fd = %d\n",
                            sock_ntop(&nb->ip, nb->lport), nb->connfd);
                    } else {
                        p2plog(ERROR,
                            "Read error, drop neighbour node: %s, fd = %d\n",
                            sock_ntop(&nb->ip, nb->lport), nb->connfd);
                    }
                    peer_error = 1;
                } else {
                    recv_byte_stream(nb->connfd, buf, n);
                }

                if (peer_error != 0) {
                    Close(nb->connfd);
                    g_pc_list_remove_by_connfd(nb->connfd);
                    g_nb_list_del(nb);
                }
            }
        }

        /* check nodes in waiting list */
        list_for_each_entry_safe(wt, wt_tmp, &g_wt_list.list, list) {
            if (wt_connected(wt) && FD_ISSET(wt->connfd, &aset)) {
                peer_error = 0;
                if ((n = Read(wt->connfd, buf, MSG_MAX)) <= 0) {
                    if (n == 0) {
                        p2plog(INFO,
                            "Disconnect from waiting node: %s, fd = %d\n",
                            sock_ntop(&wt->ip, wt->lport), wt->connfd);
                    } else {
                        p2plog(ERROR,
                            "Read error, drop waiting node: %s, fd = %d\n",
                            sock_ntop(&wt->ip, wt->lport), wt->connfd);
                    }
                    peer_error = 1;
                } else {
                    recv_byte_stream(wt->connfd, buf, n);
                }

                if (peer_error != 0) {
                    Close(wt->connfd);
                    g_pc_list_remove_by_connfd(wt->connfd);
                    g_wt_list_del(wt);
                }
            }
        }

        network_maintain();

        p2plog(INFO, "Waiting: %d  Neighbours: %d\n", 
               g_wt_list_size, g_nb_list_size);

        if (peer_error == 4) {
            p2plog(WARN, "SIGPIPE captured.\n");
        }
    }

    return 0;
}

/**
 * Start the p2p node.
 */
static int
start_node()
{
    int enabled = 1;
    lstn_fd = Socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(lstn_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &enabled, sizeof(enabled)) != 0) {
        p2plog(WARN, "Failed to set socket OPT: SO_REUSEADDR");
    }

    Bind(lstn_fd, (SA *)&g_lstn_addr, sizeof(g_lstn_addr));

    Listen(lstn_fd, LISTEN_QUEUE);
    p2plog(INFO, "P2P node starts on %s\n", 
           sock_ntop(&g_lstn_addr.sin_addr, g_lstn_addr.sin_port));


    node_loop();

    return 0;
}

int
main(int argc, char **argv)
{
    /**************** Get options from command line **************************/
    int  opt;
    char *lstn, *btstrp, *search, *kvfile, *peerad;

    lstn   = NULL;
    btstrp = NULL;
    search = NULL;
    kvfile = NULL;
    peerad = NULL;

    while ((opt = getopt(argc, argv, "l:b:s:f:p:j")) != -1) {
        switch (opt) {
            case 'l':
                lstn = optarg;
                break;
            case 'b':
                btstrp = optarg;
                break;
            case 's':
                search = optarg;
                break;
            case 'f':
                kvfile = optarg;
                break;
            case 'p':
                peerad = optarg;
                break;
            case 'j':
                g_auto_join = 1;
                break;
            default:
                usage();
                exit(1);
        }
    }

    p2plog(DEBUG, "\nl:%s\nb:%s\ns:%s\nf:%s\np:%s\n",
       lstn, btstrp, search, kvfile, peerad);

    /*********************** Set node configuration **************************/
    /* set "g_lstn_addr" */
    memset(&g_lstn_addr, 0, sizeof(g_lstn_addr));
    g_lstn_addr.sin_family = AF_INET;
    if (lstn != NULL) {
        if (sock_pton(lstn, &g_lstn_addr.sin_addr, 
                      &g_lstn_addr.sin_port) < 0) {
            p2plog(ERROR, "Invalid listen format (should be ipaddr:port)\n");
            exit(1);
        }

        if (g_lstn_addr.sin_port == 0) {
            p2plog(ERROR, "Invalid listen port (should be nonzero)\n");
            exit(1);
        }
    } else {
        g_lstn_addr.sin_addr.s_addr = INADDR_ANY;
        g_lstn_addr.sin_port = htons(PORT_DEFAULT);
    }

    /* set "g_ad_num" */
    if (peerad != NULL) {
        int n = atoi(peerad);
        if (n > 0 && n <= MAX_PEER_AD)
            g_ad_num = n;
        else {
            p2plog(WARN, "Invalid peer_ad number %d, set to DEFAULT:%d\n",
                   n, MAX_PEER_AD);
            g_ad_num = MAX_PEER_AD;
        }
    } else {
        g_ad_num = MAX_PEER_AD;
    }

    search_key = search;
    
    /********************  Init data structures ******************************/
    memset(&g_kv_list, 0, sizeof(g_kv_list));
    INIT_LIST_HEAD(&g_kv_list.list);

    memset(&g_pc_list, 0, sizeof(g_pc_list));
    INIT_LIST_HEAD(&g_pc_list.list);

    memset(&g_msg_list, 0, sizeof(g_msg_list));
    INIT_LIST_HEAD(&g_msg_list.list);

    memset(&g_nb_list, 0, sizeof(g_nb_list));
    INIT_LIST_HEAD(&g_nb_list.list);
    g_nb_list_size = 0;

    memset(&g_wt_list, 0, sizeof(g_wt_list));
    INIT_LIST_HEAD(&g_wt_list.list);
    g_wt_list_size = 0;

    /* load key/value from kvfile */
    if (kvfile != NULL) {
        if(g_kv_list_load_from_file(kvfile) != 0) {
            p2plog(ERROR, "Fail to read kvfile\n");
            exit(1);
        }
    }

    /* put bootstrap node into waiting list */
    struct sockaddr_in socktmp;
    memset(&socktmp, 0, sizeof(socktmp));
    if (btstrp) {
        if ((sock_pton(btstrp, &socktmp.sin_addr, &socktmp.sin_port)) < 0) {
            p2plog(ERROR, 
                   "Invalid bootstrap format (should be ipaddr:port)\n");
            exit(1);
        }
        struct wt_node *wt;
        wt = wt_new(0, &socktmp.sin_addr, socktmp.sin_port);
        wt_urgent_set(wt);
        g_wt_list_add(wt);
    }

    /* save all interfaces, used for self-loop determination */
    if (getifaddrs(&g_ifaddrs) == -1) {
        perror("getifaddrs()");
        p2plog(ERROR, "Failed to get interfaces\n");
        exit(1);
    }

    /******************* other initializations *******************************/
    /* Force output immediately */
    setbuf(stdout, NULL);

    /* set seed for rand() */
    srand(time(NULL));
    
    /* set signal handler for SIGPIPE */
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = sig_pipe;
    if (sigaction(SIGPIPE, &act, NULL) != 0) {
        perror("sigaction()");
        p2plog(ERROR, "Failed to set signal action\n");
        exit(1);
    }

    /* Start the p2p node */
    start_node();

    return 0;
}
