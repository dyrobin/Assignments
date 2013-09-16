#include "proto.h"
#include "p2pn.h"

#define TICK_SELECT    3
#define TICK_HEARTBEAT 5
#define TICK_NGHQUERY  8

/* the listening fd of this p2p node */
int listen_fd;

/* the sock addr of the listening fd */
struct sockaddr_in ltn_addr;

/* neighbor database */
struct node_meta neighbors;
/* size of the neighbor list */
int nm_size = 0;

/* Database for nodes which are not neighbors,
 * but would be helpful in the future.
 * Bootstrap node is also saved here.
 */
struct wtnode_meta waiting_nodes;
/* size of the waiting list */
int wt_size = 0;

struct peer_cache pr_cache;

struct msgstore sentmsgs;
struct msgstore g_recvmsgs;

/* local key/value database information */
struct keyval localdata;

/* input param: number of neighbor entries in PONG */
int n_peer_ad = MAX_PEER_AD;

/* Bootstrap information */
static char *bsp_code = NULL;

static char *search_key = NULL;

static struct timeval search_timer;

/* previous time for network_maintain */
struct timeval prev_hbeat;
struct timeval prev_nghquery;


/* debug level */
enum DLEVEL debuglv = DEBUG;

/* Usage of the p2pn program
 */
static void
usage()
{
    printf("Usage: p2pn -l [ip:port] -f [kvfile] \n"
                "\t    [-s [search_key] -b [ip:port] -p [max_peers_in_pong]]\n");
    printf("    -l: Listening address and port \n");
    printf("    -f: key/value data file \n");
    printf("    -s: Search key \n");
    printf("    -b: Bootstrap server address and port \n");
    printf("    -p: Max Number of neighbor entries in PONG \n");
}

static void
debug_env()
{
    struct node_meta *nm;
    struct wtnode_meta *wtn;

    char buf[128];

    p2plog(DEBUG, "\n::::Waiting List::::[%d]\n", wt_size);
    list_for_each_entry(wtn, &waiting_nodes.list, list) {
        p2plog(DEBUG, "%10d | %16s:%5d | nq = %5d | urgent = %5d\n",
	       wtn->connfd,
	       inet_ntop(AF_INET, &wtn->ip, buf, sizeof(buf)),
	       ntohs(wtn->lport),
	       wtn->nrequest,
	       wtn->urgent);
    }

    p2plog(DEBUG, "::::Neighbor List::::[%d]\n", nm_size);
    list_for_each_entry(nm, &neighbors.list, list) {
        p2plog(DEBUG, "%10d | %16s:%5d\n",
	       nm->connfd,
	       inet_ntop(AF_INET, &nm->ip, buf, sizeof(buf)),
	       ntohs(nm->lport));
    }
    p2plog(DEBUG, "\n\n");
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
    struct P2P_h *ph;
    struct wtnode_meta *wtn;
    struct node_meta *nm;

    uint32_t kval;
    int msglen, remain_len, from_neigh, connfd, i;
    char mmbuf[SLEN];

#define from_neigh() \
    from_neigh == 1
#define set_from_neigh() \
    from_neigh = 1
#define unset_from_neigh() \
    from_neigh = 0

    remain_len = pc->bp + 1;
    connfd = pc->connfd;
    unset_from_neigh();
    nm = nm_find_by_connfd(connfd);
    if (nm != NULL) {
        set_from_neigh();
        p2plog(DEBUG, "Receive from neighbor: %s:%d\n",
	       inet_ntop(AF_INET, &nm->ip, mmbuf, SLEN),
	       ntohs(nm->lport));
    } else {
        wtn = wtn_find_by_connfd(connfd);
        if (wtn != NULL)
            p2plog(DEBUG, "Receive from waiting list: %s:%d\n",
		   inet_ntop(AF_INET, &wtn->ip, mmbuf, 128),
		   ntohs(wtn->lport));
        else {
            p2plog(ERROR, "Received a msg from an unknown sender.\n");
            return 0;
            }
    }

    if (pc->bp < HLEN) {
        return 0;
    }

    ph = (struct P2P_h *) (pc->recvbuf);
    msglen = HLEN + ntohs(ph->length);

    if (ph->version != P_VERSION) {
        p2plog(ERROR, "Invalid version number: %d\n",
	       ph->version);
        goto CLEAR_CACHE;
    }

    if (ph->ttl > MAX_TTL) {
        p2plog(ERROR, "TTL too large: %d\n",
	       ph->ttl);
        goto CLEAR_CACHE;
    }

    if (msglen > remain_len) {
    /* there is more data pending, stop recv */
        return 0;
    }

    p2plog(DEBUG, "MSG: [%08X], msg_type = %02X, len = %d\n",
	   ph->msg_id, ph->msg_type, ntohs(ph->length));

    if (!from_neigh() &&
            ((ph->msg_type & MSG_JOIN) != MSG_JOIN)){
        /* msg is not from a established neighbor,
           and it is not a JOIN type message, we should
           not allow this message */
           p2plog(ERROR, "Unexpected msg type from a non-neigh\n");
           goto CLEAR_CACHE;
    }
    switch(ph->msg_type) {
        case MSG_PING:
        handle_ping_message(connfd, (char *)ph, msglen);
        break;

        case MSG_PONG:
        handle_pong_message(connfd, (char *)ph, msglen);
        break;

        case MSG_BYE:
        handle_bye_message(connfd, (void *)ph, msglen);
        break;

        case MSG_JOIN:
        handle_join_message(connfd, (char *)ph, msglen);
        break;

        case MSG_QUERY:

        p2plog(INFO, "Receive QUERY MSG: [%08X], len = %d, from: %s:%d\n",
	       ph->msg_id,
	       ntohs(ph->length),
	       inet_ntop(AF_INET, &nm->ip, mmbuf, SLEN),
	       ntohs(nm->lport));

        if (find_stored_msg(&g_recvmsgs, ph->msg_id) != NULL) {
                p2plog(ERROR, "dupplicated msg found in our message storage\n");
                break;
        }
        gc_msgstore(&g_recvmsgs);
        push_g_recv_msg(connfd, (void *)ph, msglen);
        /* match local keys */
        if ((kval = search_localdata(ph, msglen)) != 0) {
            send_query_hit(connfd, ph, msglen, kval);
            p2plog(INFO, "QUERY matches local key, QUERY HIT sent.\n");
        }
        /* still forward msg to find more result */
        flood_msg(connfd, ph, msglen);
        p2plog(INFO, "flood query message\n");
        break;

        case MSG_QHIT:

        p2plog(INFO, "Receive QHIT MSG: [%08X], len = %d, from: %s:%d\n",
	       ph->msg_id,
	       ntohs(ph->length),
	       inet_ntop(AF_INET, &nm->ip, mmbuf, SLEN),
	       ntohs(nm->lport));

        handle_query_hit(connfd, (void *)ph, msglen);
        break;

        default:
        p2plog(ERROR, "receive a message with a invalid message type\n");
        goto CLEAR_CACHE;
    }

    for (i = msglen; i < pc->bp; i++)
        pc->recvbuf[i-msglen] = pc->recvbuf[i];
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

    if((pc = get_peer_cache(connfd)) == NULL) {
        p2plog(ERROR, "cannot find peer cache, connfd = %d\n",
		connfd);
        return;
    }

    if (pc->bp + bufsize > MAXLINE) {
        p2plog(ERROR, "buffer is full in the peer cache for connfd = %d\n",
	       connfd);
        return;
    }

    bcopy(buf, pc->recvbuf + pc->bp, bufsize);
    pc->bp += bufsize;
    while ((n = recv_msg(pc)) != 0) {
        //blank;
    }
}

/**
 * Handle pending peers in the waiting list.
 */
static void
handle_waiting_list()
{
    struct wtnode_meta *wtn, *wtnx;
    struct sockaddr_in addr;
    int connfd, n;

    if (nm_size < RECM_NEIB) {
    /* We try to connect more peers if our own neighbor database
       is small.
    */
        list_for_each_entry(wtn, &waiting_nodes.list, list) {
            if (!wtn_conn_established(wtn)
                    && !wtn_urgent(wtn) && !wtn_requested(wtn))
                wtn_urgent_set(wtn);
                break;
        }
    }

    list_for_each_entry_safe(wtn, wtnx, &waiting_nodes.list, list) {
    /* if there is urgent waiting node, try to establish connection */
        if (!wtn_conn_established(wtn) && wtn_urgent(wtn)) {
            bzero(&addr, sizeof(addr));
            bcopy(&wtn->ip, &addr.sin_addr, sizeof(struct in_addr));
            addr.sin_port = wtn->lport;
            addr.sin_family = AF_INET;

            if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) >= 0) {
                wtn->connfd = connfd;
                if ((n = connect_pto(connfd, (SA *)&addr,
				     sizeof(addr), 2)) >= 0) {
                    send_join_message(connfd);
                    wtn->nrequest++;
                    wtn_urgent_reset(wtn);
                    create_peer_cache(connfd);
                } else {
                    perror("connect_pto error");
                    wt_list_del(wtn);
                }
            } else {
                perror("socket error");
                wt_list_del(wtn);
            }
        } else {
            /*TODO we should select nodes to connect
            if number of neighbors become less. */

            //send_join_message(connfd);
            //wtn->nrequest++;
        }
    }
}

/**
 * Maintain the p2p network.
 *
 * Maintain the stable and avaliability of the p2p network by
 * periodically send PING messages and renew its neighbor database.
 */
void
network_maintain()
{
    struct timeval now, hbeat, nquery;
    struct node_meta *nm;

    /* check waiting list */
    handle_waiting_list();

    /* heart beat and neighbor query */
    hbeat = prev_hbeat;
    hbeat.tv_sec += TICK_HEARTBEAT;
    nquery = prev_nghquery;
    nquery.tv_sec += TICK_NGHQUERY;

    gettimeofday(&now, NULL);
    if (timercmp(&hbeat, &now, <=)) {
        /* send heart beat to all neighbors */
        list_for_each_entry(nm, &neighbors.list, list) {
            send_ping_message(nm->connfd, PING_TTL_HB);
        }
        prev_hbeat = now;
    }
    if (timercmp(&nquery, &now, <=)) {
        /* send neighbor query to all neighbors */
        list_for_each_entry(nm, &neighbors.list, list) {
            send_ping_message(nm->connfd, MAX_TTL);
        }
        prev_nghquery = now;
    }
    /* search the network */
    if (search_key != NULL &&
        timercmp(&search_timer, &now, <=)) {
        send_query_message(search_key);
        search_key = NULL;
    }
}

/**
 * The main loop for message receving and handling
 */
static int
node_loop()
{
    struct sockaddr_in cliaddr;
    struct node_meta *tempn, *nxtn;
    struct wtnode_meta *wtn, *wtnn;
    fd_set aset;
    int nready, connfd, maxfd, n, opt_recv_low;
    socklen_t clisize;
    struct timeval timeout, pre_slct, post_slct;
    char sbuf[SLEN];
    char buf[MAXLINE];


    opt_recv_low = HLEN;
    FD_ZERO(&aset);
    FD_SET(listen_fd, &aset);


    for ( ; ; ) {
        /* Find max FD for select call */
        FD_ZERO(&aset);
        FD_SET(listen_fd, &aset);
        maxfd = listen_fd;
        list_for_each_entry(tempn, &neighbors.list, list) {
            if (tempn->connfd > maxfd)
                maxfd = tempn->connfd;
            FD_SET(tempn->connfd, &aset);
        }
        list_for_each_entry(wtn, &waiting_nodes.list, list) {
            if (wtn_conn_established(wtn)) {
                if (wtn->connfd > maxfd)
                    maxfd = wtn->connfd;
                FD_SET(wtn->connfd, &aset);
            }
        }

	p2plog(DEBUG, "\n");
        /* Set select timeout to TICK seconds */
        timeout.tv_sec = TICK_SELECT;
        timeout.tv_usec = 0;

        gettimeofday(&pre_slct, NULL);
        nready = select(maxfd + 1, &aset, NULL, NULL, &timeout);
        gettimeofday(&post_slct, NULL);

        if (nready == 0) {
            network_maintain();
            continue;
        }

        if (FD_ISSET(listen_fd, &aset)) { /* New connection arrives */
            clisize = sizeof(cliaddr);
            connfd = Accept(listen_fd, (SA *) &cliaddr, &clisize);
            if (setsockopt(connfd, SOL_SOCKET, SO_RCVLOWAT, &opt_recv_low, sizeof(int)) != 0) {
                p2plog(ERROR, "Failed to set socket OPT: SO_RCVLOWAT");
            }
            wtn = (struct wtnode_meta *)Malloc(sizeof(struct wtnode_meta));
            wtn_init(wtn);
            wtn->connfd = connfd;
            wtn->ip = cliaddr.sin_addr;
            p2plog(DEBUG, "Connection from %s:%d, fd = %d\n",
		   inet_ntop(AF_INET, &cliaddr.sin_addr, sbuf, sizeof(sbuf)),
		   ntohs(cliaddr.sin_port),
		   wtn->connfd);
            /* save to waiting list */
            wt_list_add(wtn);
            create_peer_cache(connfd);

            /* add to fdset */
            FD_SET(connfd, &aset);

            if (--nready <= 0) { /* no more discriptor to read */
                network_maintain();
                continue;
            }
        }

        /* Check all neighbor nodes if they are readable */
        list_for_each_entry_safe(tempn, nxtn, &neighbors.list, list) {
            if (FD_ISSET(tempn->connfd, &aset)) {
                if ((n = Read(tempn->connfd, buf, MAXLINE)) <= 0) {
                    if (n == 0) {
                        p2plog(WARN, "Disconnect from neighbor: %s:%d, fd = %d\n",
			       inet_ntop(AF_INET, &tempn->ip, sbuf, sizeof(sbuf)),
			       ntohs(tempn->lport),
			       tempn->connfd);
                    } else {
                        p2plog(ERROR, "Read error, drop neighbor: %s:%d, fd = %d\n",
			       inet_ntop(AF_INET, &tempn->ip, sbuf, sizeof(sbuf)),
			       ntohs(tempn->lport),
			       tempn->connfd);
                    }
                    remove_peer_cache(tempn->connfd);
                    Close(tempn->connfd);
                    FD_CLR(tempn->connfd, &aset);
                    nm_list_del(tempn);
                } else {
                    recv_byte_stream(tempn->connfd, buf, n);
                }

                if (--nready <= 0) { /* no more discriptor to read */
                    network_maintain();
                    break;
                }
            }
        }

        /* check nodes in waiting list */
        list_for_each_entry_safe(wtn, wtnn, &waiting_nodes.list, list) {
            if (wtn_conn_established(wtn) && FD_ISSET(wtn->connfd, &aset)) {
                if ((n = Read(wtn->connfd, buf, MAXLINE)) <= 0) {
                    if (n == 0) {
                    p2plog(WARN, "Disconnect from waiting node: %s:%d, fd = %d\n",
                        inet_ntop(AF_INET, &wtn->ip, sbuf, sizeof(sbuf)),
                        ntohs(wtn->lport),
                        wtn->connfd);
                    } else {
                    p2plog(ERROR, "Read error, drop waiting node: %s:%d, fd = %d\n",
                        inet_ntop(AF_INET, &wtn->ip, sbuf, sizeof(sbuf)),
                        ntohs(wtn->lport),
                        wtn->connfd);
                    }
                    remove_peer_cache(wtn->connfd);
                    Close(wtn->connfd);
                    FD_CLR(wtn->connfd, &aset);
                    wt_list_del(wtn);
                } else {
                    recv_byte_stream(wtn->connfd, buf, n);
                }
            }

            if (--nready <=0) {
                network_maintain();
                break;
            }
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
    struct sockaddr_in socktmp;
    struct wtnode_meta *wtn;
    int on;

    listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

    on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        p2plog(ERROR, "Failed to set socket OPT: SO_REUSEADDR");
    }
    Bind(listen_fd, (SA *)&ltn_addr, sizeof(ltn_addr));

    Listen(listen_fd, LISTENQ);
    p2plog(INFO, "P2P node starts on  %s\n", sock_ntop(&ltn_addr));

    /* init local node database */
    bzero(&neighbors, sizeof(neighbors));
    INIT_LIST_HEAD(&neighbors.list);
    bzero(&waiting_nodes, sizeof(waiting_nodes));
    INIT_LIST_HEAD(&waiting_nodes.list);

    /*init local sent_msg store */
    bzero(&sentmsgs, sizeof(sentmsgs));
    INIT_LIST_HEAD(&sentmsgs.list);
    /*init local recv mes_store */
    bzero(&g_recvmsgs, sizeof(g_recvmsgs));
    INIT_LIST_HEAD(&g_recvmsgs.list);

    /*init peer cache */
    bzero(&pr_cache, sizeof(pr_cache));
    INIT_LIST_HEAD(&pr_cache.list);

    /* If there is bootstrap code provided, read it and put
       the information in the waiting list */
    if (bsp_code != NULL) {
        if ((sock_pton(&socktmp, bsp_code)) == 1) {
            p2plog(ERROR, "invalid bootstrap information: %s\n",
                   bsp_code);
            exit(1);
        }
        wtn = Malloc(sizeof(struct wtnode_meta));
        wtn_init(wtn);
        wtn->ip = socktmp.sin_addr;
        wtn->lport = socktmp.sin_port;
        wtn->urgent = 1;
        wt_list_add(wtn);
    }

    /* Init other global data */
    bzero(&prev_hbeat, sizeof(prev_hbeat));
    bzero(&prev_nghquery, sizeof(prev_nghquery));

    node_loop();

    return 0;
}

/**
 * Read key/value pairs from a file.
 * @param file the file to read key/value pairs.
 * @return     0 on success, 1 otherwise.
 */
int
read_kvfile(char *file)
{
    FILE *fp;
    char buf[MLEN];
    uint32_t value;
    int n, klen;
    struct keyval *kv;

    if((fp = fopen(file, "r")) == NULL) {
        p2plog(ERROR, "Failed to open file: %s\n", file);
        return 1;
    }
    while ((n = fscanf(fp, "%s %x\n", buf, &value)) != EOF) {
        if (n == 2) {
            kv = (struct keyval *) Malloc(sizeof(struct keyval));
            klen = (strlen(buf) > KEYLEN) ? KEYLEN : strlen(buf);
            bcopy(buf, kv->key, klen);
            kv->key[klen] = '\0';
            kv->value = value;
            list_add(&kv->list, &localdata.list);
            p2plog(DEBUG, "Add key/value %s = 0x%08X\n", kv->key, kv->value);
        }
    }

    fclose(fp);
    return 0;
}

int
main(int argc, char **argv)
{
    int  opt, n;
    char *search, *kvfile, *laddr, *maxp;

    search = NULL;
    kvfile = NULL;
    laddr = NULL;
    bsp_code = NULL;
    maxp = NULL;
    bzero(&ltn_addr, sizeof(ltn_addr));
    ltn_addr.sin_family = AF_INET;

    setbuf(stdout, NULL);
    while ((opt = getopt(argc, argv, "l:b:s:f:p:")) != -1) {
        switch (opt) {
            case 'l':
            laddr = optarg;
            break;

            case 'b':
            bsp_code = optarg;
            break;

            case 's':
            search = optarg;
            break;

            case 'f':
            kvfile = optarg;
            break;

            case 'p':
            maxp = optarg;
            break;

            default:
            usage();
            exit(1);
        }
    }

    if (laddr == NULL) {
        usage();
        exit(1);
    }

    p2plog(DEBUG, "l:%s\nb:%s\ns:%s\nf:%s\np:%s\n",
	   laddr, bsp_code, search, kvfile, maxp);

    /* set max neighbor entiries in PONG */
    if (maxp != NULL) {
        n = atoi(maxp);
        if (n > 0 && n <= MAX_PEER_AD)
            n_peer_ad = n;
        else
            p2plog(WARN, "Invalid peer_ad number:%d, set to DEFAULT:%d\n",
		   n, MAX_PEER_AD);
    }
    /* Read port number from argv */
    if (sock_pton(&ltn_addr, laddr) != 0) {
        p2plog(ERROR, "Incorrect listening addr:port\n");
        usage();
        exit(1);
    }

    search_key = search;

    /* init local key value data store */
    bzero(&localdata, sizeof(localdata));
    INIT_LIST_HEAD(&localdata.list);

    if (kvfile != NULL) {
        if(read_kvfile(kvfile) != 0) {
            p2plog(ERROR, "Fail to read kvfile!\n");
            exit(1);
        }
    }
    gettimeofday(&search_timer, NULL);
    search_timer.tv_sec += 10;

    /* Start the p2p node */
    start_node();
    return 0;
}
