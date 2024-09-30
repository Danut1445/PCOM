#ifndef __UTILS_H__
#define __UTILS_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);
int compare_func(char *s1, char *s2);

/* Dimensiunea maxima a mesajului */
#define MAX_MSG 1500
#define TOPIC 50
#define MAX_ID 11

struct packet_udp {
    char topic[TOPIC];
    uint8_t type;
    char payload[MAX_MSG];
};

struct packet_tcp {
    uint8_t type;
    char ip[16];
    uint16_t port;
    char payload[sizeof(struct packet_udp)];
};

struct client {
    char ID[MAX_ID];
    uint32_t maxtopics;
    uint32_t topicsize;
    uint8_t online;
    char **topics;
};

#endif