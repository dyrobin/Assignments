#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>
#include <netinet/in.h>

/* Default listening port */
//#define PORT_DEFAULT    6346
#define PORT_DEFAULT    10001

/* definition of protocol message type */
#define MSG_PING        0x00
#define MSG_PONG        0x01
#define MSG_BYE         0x02
#define MSG_JOIN        0x03
#define MSG_QUERY       0x80
#define MSG_QHIT        0x81

/* header length */
#define HLEN            (sizeof(struct P2P_h))

/* body length of JOIN message */
#define JOINLEN         (sizeof(struct P2P_join))

/* The minimum length of PONG body for network probing response */
#define PONG_MINLEN     (sizeof(struct P2P_pong_front))

/* The length of each entry in Pong body */
#define PONG_ENTRYLEN   (sizeof(struct P2P_pong_entry))

/* The minimum length of a QUERY_HIT message body */
#define QHIT_MINLEN     (sizeof(struct P2P_qhit_front))

/* The length of each entry for a QUERY_HIT message */
#define QHIT_ENTRYLEN   (sizeof(struct P2P_qhit_entry))

/* Protocol version */
#define P_VERSION       1
/* MAX TTL */
#define MAX_TTL         5

/* max number of entries for a PONG response */
#define MAX_PEER_AD     5
/* TTL value for PING (heart beat) */
#define PING_TTL_HB     1
/* Reply code of JOIN accept */
#define JOIN_ACC        0x0200

/* The Header definition of our Protocol */
struct P2P_h {
    uint8_t     version;
    uint8_t     ttl;
    uint8_t     msg_type;
    uint8_t     reserved;
    /* the listening port of the original sender */
    uint16_t    org_port;
    /* the length of message body */
    uint16_t    length;
    /* the ip address of the original sender */
    uint32_t    org_ip;
    uint32_t    msg_id;
};

/* The body of the JOIN response */
struct P2P_join {
    uint16_t    status;
};

/* The front part of the Pong message body,
   which includes the entry size */
struct P2P_pong_front {
    uint16_t    entry_size;
    uint16_t    sbz;
};

/* The structure of each entry in Pong body */
struct P2P_pong_entry {
    struct in_addr ip;
    uint16_t       port;
    uint16_t       sbz;
};

/* The first part of the QUERY_HIT message */
struct P2P_qhit_front {
    uint16_t    entry_size;
    uint16_t    sbz;
};

/* Each entry of the QUERY_HIT message */
struct P2P_qhit_entry {
    uint16_t    res_id;
    uint16_t    sbz;
    uint32_t    res_val;
};


int send_join_message(int connfd);

int handle_join_message(int connfd, void *msg, unsigned int len);

int send_ping_message(int connfd, int ttl);

int handle_ping_message(int connfd, void *msg, unsigned int len);

int handle_pong_message(int connfd, void *msg, unsigned int len);

int send_query_message(char *search_key);

int handle_query_message(int connfd, void *msg, unsigned int len);

int send_query_hit(int connfd, void *msg, uint32_t val);

int handle_query_hit(void *msg, unsigned int len);

int handle_bye_message(int connfd);

#endif
