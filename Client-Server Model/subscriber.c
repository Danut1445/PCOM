#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#include <math.h>
#include "utils.h"
#include "helpers.h"

//Definim un tip de date pentru un pointer de functie care
//primeste ca parametrii packet_tcp si packet_udp si returneaza void
typedef void (*func)(struct packet_tcp, struct packet_udp);

/*Mai jos avem functii ajutatoare pentru clientul nostru*/
void send_exit(int sock_fd, char *word) {
    struct packet_tcp packet;
    packet.type = 0;
    send_all(sock_fd, &packet, sizeof(struct packet_tcp));
}

void send_subscribe(int sock_fd, char *word) {
    word = strtok(NULL, " \n");
    struct packet_tcp packet;
    packet.type = 1;
    memset(packet.payload, 0, sizeof(struct packet_udp));
    memcpy(packet.payload, word, strlen(word));
    send_all(sock_fd, &packet, sizeof(struct packet_tcp));
    printf("Subscribed to topic.\n");
}

void send_unsubscribe(int sock_fd, char *word) {
    word = strtok(NULL, " \n");
    struct packet_tcp packet;
    packet.type = 2;
    memset(packet.payload, 0, sizeof(struct packet_udp));
    memcpy(packet.payload, word, strlen(word));
    send_all(sock_fd, &packet, sizeof(struct packet_tcp));
    printf("Unsubscribed to topic.\n");
}

void parse_int(struct packet_tcp packet, struct packet_udp information) {
    uint32_t number = ntohl(*(uint32_t *)(information.payload + 1));
    if (*(uint8_t *)information.payload == 1)
        number = number * -1;
    printf("%s:%d - %s - INT - %d\n", packet.ip, packet.port, information.topic, number);    
}

void parse_short_real(struct packet_tcp packet, struct packet_udp information) {
    uint16_t number = ntohs(*(uint16_t *)(information.payload));
    double nr = number;
    nr /= 100;
    printf("%s:%d - %s - SHORT_REAL - %.2f\n", packet.ip, packet.port, information.topic, nr);   
}

void parse_float(struct packet_tcp packet, struct packet_udp information) {
    double number = ntohl(*(uint32_t *)(information.payload + 1));
    uint8_t power = *(uint8_t *)(information.payload + 5);
    long div = 1;
    while (power) {
        div *= 10;
        power--;
    }
    if (*(uint8_t *)information.payload == 1)
    number = number * -1;
    printf("%s:%d - %s - FLOAT - %f\n", packet.ip, packet.port, information.topic, number / div);  
}

void parse_string(struct packet_tcp packet, struct packet_udp information) {
    printf("%s:%d - %s - STRING - %s\n", packet.ip, packet.port, information.topic, information.payload);  
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n");
        return 0;
    }

    struct pollfd poll_fds[2];
    int sock_fd;
    struct sockaddr_in server_addr;
    char ID_client[11];
    char IP_server[16];
    uint16_t port;
    int rc;
    int flag = 1;
    //Setam vectorul de functii pe care o sa il folosim mai tarziu
    func vec_func[4];
    vec_func[0] = parse_int;
    vec_func[1] = parse_short_real;
    vec_func[2] = parse_float;
    vec_func[3] = parse_string;

    //Facem rost de adresa ip si portul serverului
    memcpy(ID_client, argv[1],  sizeof(strlen(argv[1])));
    memcpy(IP_server, argv[2], strlen(argv[2]));
    port = atoi(argv[3]);

    rc = setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(rc < 0, "Error buffer stdout");
    
    //Creeam un socket
    sock_fd  = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock_fd < 0, "socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, IP_server, &server_addr.sin_addr);
    DIE(rc < 0, "inet_pton");

    rc = setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    DIE (rc < 0, "Error Nagle algorithm");

    //Conectam socketul la server
    rc = connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(rc < 0, "connect");

    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
 
    poll_fds[1].fd = sock_fd;
    poll_fds[1].events = POLLIN;

    //Trimitem ID-ul serverului
    send_all(sock_fd, &ID_client, MAX_ID);

    while (1) {
        //Asteptam o notificare
        rc = poll(poll_fds, 2, -1);
        DIE(rc < 0, "poll");

        //A venit de la STDIN
        if (poll_fds[0].revents & POLLIN) {
            char comm[200];
            fgets(comm, sizeof(comm), stdin);
            char *word = strtok(comm, " \n\0");
            if (!strcmp(word, "exit")) {
                send_exit(sock_fd, word);
                break;
            }
            if (!strcmp(word, "subscribe"))
                send_subscribe(sock_fd, word);
            if (!strcmp(word, "unsubscribe"))
                send_unsubscribe(sock_fd, word);
        //A venit de la server
        } else if (poll_fds[1].revents & POLLIN) {
            struct packet_tcp packet;
            struct packet_udp information;
            int rc = recv_all(sock_fd, &packet, sizeof(struct packet_tcp));
            memcpy(&information, packet.payload, sizeof(struct packet_udp));

            if (rc == 0) {
                break;
            }

            if (packet.type == 255)
                break;
            vec_func[information.type](packet, information);
        }
    }

    rc = close(sock_fd);
    DIE(rc < 0, "Error closing socket");

    return 0;
}