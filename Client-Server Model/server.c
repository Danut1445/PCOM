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

int maxnrfd;
int currfd;
struct client *clients;
struct pollfd *fds;

/* Aceasta functie se ocupa de anuntarea tuturor clientilor tcp
    ca urmeaza sa se inchida serverul, si dupa elibereaza memoria
    ocupata pentru a stoca date despre acestia*/
void exit_server() {
    struct packet_tcp response;
    memset(&response, 0, sizeof(struct packet_tcp));
    response.type = 255;
    for (int i = 3; i < currfd; i++) {
        if (clients[i].online) {
            send_all(fds[i].fd, &response, sizeof(struct packet_tcp));
        }
        close(fds[i].fd);
        for (int j = 0; j < clients[i].topicsize; j++)
            free(clients[i].topics[j]);
        free(clients[i].topics);
    }
    free(fds);
    free(clients);
}

/*Functiaa se ocupa de adaugarea unui nou client la server, primeste
ca argumente socektul clientului, si adresa acestuia*/
void handle_client(int client_sockfd, struct sockaddr_in client_addr) {
    int ret;
    char client_id[MAX_ID];
    memset(client_id, 0, sizeof(client_id));
    //Primim mesajul cu id-ul clientului de la acesta
    ret = recv_all(client_sockfd, client_id, MAX_ID);  
    DIE(ret < 0, "recv");
    int already = 0;

    //Verificam daca avem deja un client cu ID-ul corespunzator
    //in vectorul nostru cu clienti
    for (int i = 3; i < currfd; i++) {
        if (!strcmp(clients[i].ID, client_id)) {
            if (clients[i].online == 1) {
                struct packet_tcp response;
                memset(&response, 0, sizeof(struct packet_tcp));
                response.type = 255;
                send_all(client_sockfd, &response, sizeof(struct packet_tcp));
                printf("Client %s already connected.\n", clients[i].ID);
            } else {
                fds[i].fd = client_sockfd;
                clients[i].online = 1;
                printf("New client %s connected from %s:%hu.\n", client_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
            already = 1;
            break;
        }
    }
    if (already == 1)
        return;
    
    //Daca vectorul e plin, dublam dimensiunea acestuia
    if (currfd == maxnrfd) {
        struct pollfd *aux = malloc(2 * sizeof(struct pollfd) * maxnrfd);
        DIE(aux < 0, "aux");
        struct client *aux2 = malloc(2 * sizeof(struct client) * maxnrfd);
        DIE(aux2 < 0, "aux2");
        for (int i = 0; i < maxnrfd; i++) {
            aux[i] = fds[i];
            aux2[i] = clients[i];
        }
        free(fds);
        free(clients);
        fds = aux;
        clients = aux2;
        maxnrfd *= 2;
    }

    //Creeam un nou client si il adaugam in vectorul cu clienti
    fds[currfd].fd = client_sockfd;
    fds[currfd].events = POLLIN;
    memcpy(&clients[currfd].ID, client_id, MAX_ID);
    clients[currfd].maxtopics = 16;
    clients[currfd].topicsize = 0;
    clients[currfd].online = 1;
    clients[currfd].topics = malloc(16 * sizeof(char *));
    printf("New client %s connected from %s:%hu.\n", client_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    currfd++;  
}

/*Functia se ocupa cu primirea unui mesaje de la un client udp
    si trimiterea acestuia catre toti clientii tcp corespunzatori*/
void handle_udp_client(int udp_sockfd) {
    int ret;
    struct packet_udp udp_packet;
    struct packet_tcp response_packet;
    struct sockaddr_in udp_addr;
    socklen_t udp_len = sizeof(udp_addr);

    //Primim mesajul de la clientul udp
    ret = recvfrom(udp_sockfd, &udp_packet, sizeof(struct packet_udp), 0, (struct sockaddr *) &udp_addr, &udp_len);
    DIE(ret < 0, "[SERV] Error recv");

    //Pregatim un pachet tcp pe care o sa il trimitem la toti clientii care se potrivesc
    memset(&response_packet, 0, sizeof(struct packet_tcp));
    memcpy(response_packet.payload, &udp_packet, sizeof(struct packet_udp));
    strcpy(response_packet.ip, inet_ntoa(udp_addr.sin_addr));
    response_packet.port = ntohs(udp_addr.sin_port);
    response_packet.type = 0;

    //Gasim clientii care se potrivesc si le trimitem pachetul tcp
    for (int i = 3; i < currfd; i++) {
        if (!clients[i].online)
            continue;

        for (int j = 0; j < clients[i].topicsize; j++) {
            if (compare_func(clients[i].topics[j], udp_packet.topic)) {
                send_all(fds[i].fd, &response_packet, sizeof(struct packet_tcp));
                break;
            }
        }
    }    
}

/*Functia se ocupa cu primirea unei comenzii de la un client tcp
    une i este pozitia clientului in vectorul nostru de clienti*/
void handle_tcp_client(int i) {
    struct packet_tcp packet;
    recv_all(fds[i].fd, &packet, sizeof(struct packet_tcp));
    //Daca type e zero, inseamna ca vrea sa se deconecteze
    if (packet.type == 0) {
        clients[i].online = 0;
        close(fds[i].fd);
        printf("Client %s disconnected.\n", clients[i].ID);
        return;
    }

    //Daca type e unu, inseamna ca vrea sa dea subscribe
    if (packet.type == 1) {
        int found = 0;
        for (int j = 0; j < clients[i].topicsize; j++) {
            if (!strcmp(clients[i].topics[j], packet.payload)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            //Daca nu mai avem loc de topicuri, il redimensionam
            if (clients[i].topicsize == clients[i].maxtopics) {
                char **aux = malloc(sizeof(char *) * clients[i].maxtopics * 2);
                DIE(aux < 0, "aux topic");
                for (int i = 0; i < clients[i].maxtopics; i++)
                    aux[i] = clients[i].topics[i];
                free(clients[i].topics);
                clients[i].topics = aux;
                clients[i].maxtopics *= 2;
            }
            clients[i].topics[clients[i].topicsize] = malloc(TOPIC);
            memcpy(clients[i].topics[clients[i].topicsize], packet.payload, TOPIC);
            clients[i].topicsize++;;
        }
        return;
    }

    //Daca type e doi, inseamna ca vrea sa dea unsubscribe
    if (packet.type == 2) {
        for (int j = 0; j < clients[i].topicsize; j++) {
            if (!strcmp(clients[i].topics[j], packet.payload)) {
                free(clients[i].topics[j]);
                for (int k = j; k < clients[i].topicsize - 1; k++)
                    clients[i].topics[k] = clients[i].topics[k + 1];
                clients[i].topicsize--;
                break;
            }
        }
        return;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./server <PORT_SERVER>\n");
        return 0;
    }
    //Initializam serverul de clienti si de file descriptori
    maxnrfd = 64;
    currfd = 3;
    clients = malloc(maxnrfd * sizeof(struct client));
    DIE(clients < 0, "error clients malloc");
    fds = malloc(maxnrfd * sizeof(struct pollfd));
    DIE(fds < 0, "error fds malloc");

    uint16_t port;
    int udp_sockfd, tcp_sockfd;
    struct sockaddr_in server_addr;
    int ret, flag = 1;

    port = atoi(argv[1]);

    //Facem disable la bufferul pentru stdout
    ret = setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(ret < 0, "error buffering");

    //Facem rost de socket pt clientii udp
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sockfd < 0, "udp socket");

    //Dacem rost de socekt pt clientii tcp
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sockfd < 0, "tcp socket");

    ret = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    DIE (ret < 0, "Naglae algorithm");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    //Facem bind la ambele socketuri, acceptand conexiunii de pe orice adresa
    ret = bind(udp_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    DIE(ret < 0, "bind udp");

    ret = bind(tcp_sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    DIE(ret < 0, "bind tcp");

    //Pe socketul tcp o sa apelam listen si in momentul in care o sa primim o notificare
    //o sa dam accept si o sa creeam un nou socket pentru clientul respectiv pentru a 
    //comunica 1 la 1 cu serverul
    ret = listen(tcp_sockfd, maxnrfd);
	DIE(ret < 0, "listen");

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = tcp_sockfd;
    fds[1].events = POLLIN;
    fds[2].fd = udp_sockfd;
    fds[2].events = POLLIN;

    while (1) {
        //Asteptam sa primim o notificare
        ret = poll(fds, currfd, -1);
        DIE(ret < 0, "poll");

        if (fds[0].revents && POLLIN) {
            char comm[200];
            fgets(comm, sizeof(comm), stdin);
            char *word = strtok(comm, " \n");
            if (!strcmp("exit", word)) {
                exit_server();
                break;
            }
        }

        if (fds[1].revents && POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_sockfd = accept(tcp_sockfd, (struct sockaddr *)&client_addr, &client_len);
            DIE(client_sockfd < 0, "accept");

            ret = setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
            DIE (ret < 0, "naglae client");

            handle_client(client_sockfd, client_addr);
            continue;
        }

        if (fds[2].revents && POLLIN) {
            handle_udp_client(udp_sockfd);
            continue;
        }

        for (int i = 3; i < currfd; i++) {
            if (!clients[i].online)
                continue;
            if (fds[i].revents && POLLIN)
                handle_tcp_client(i);
        }
    }

    //Inchidem socketurile
    ret = close(udp_sockfd);
    DIE (ret < 0, "[SERV] Error while closing UDP socket");
    ret = close(tcp_sockfd);
    DIE (ret < 0, "[SERV] Error while closing TCP socket");
    
    return 0;
}