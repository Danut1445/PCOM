#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson/parson.h"

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    char comm[100];
    char token[400];
    char cookie[400];
    memset(token, 0, 400);
    memset(cookie, 0, 400);

    while (1) {
        scanf("%s", comm);
        if (!strcmp(comm, "register")) {
            char username[100], password[100];
            fgets(username, 200, stdin);
            printf("username=");
            fgets(username, 200, stdin);
            username[strlen(username) - 1] = '\0';
            printf("password=");
            fgets(password, 200, stdin);
            password[strlen(password) - 1] = '\0';
            if (strchr(username, ' ')) {
                printf("Eroare: user invalid");
                continue;
            }
            if (strchr(password, ' ')) {
                printf("Eroare: parola invalid");
                continue;
            }

            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            json_object_set_string(root_object, "username", username);
            json_object_set_string(root_object, "password", password);
            char *payload = json_serialize_to_string_pretty(root_value);

            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request("34.246.184.49:8080", "/api/v1/tema/auth/register", "application/json", &payload, 0, NULL, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (!strcmp(parse_response, "ok"))
                printf("Succes: Utilizator înregistrat cu succes!\n");
            else
                printf("Eroare: Utilizatorul exista deja\n");
            free(message);
            free(response);
            json_free_serialized_string(payload);
            json_value_free(root_value);
        }
        if (!strcmp(comm, "login")) {
            char username[100], password[100];
            fgets(username, 200, stdin);
            printf("username=");
            fgets(username, 200, stdin);
            username[strlen(username) - 1] = '\0';
            printf("password=");
            fgets(password, 200, stdin);
            password[strlen(password) - 1] = '\0';
            if (strchr(username, ' ')) {
                printf("Eroare: user invalid");
                continue;
            }
            if (strchr(password, ' ')) {
                printf("Eroare: parola invalid");
                continue;
            }

            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            json_object_set_string(root_object, "username", username);
            json_object_set_string(root_object, "password", password);
            char *payload = json_serialize_to_string_pretty(root_value);

            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request("34.246.184.49:8080", "/api/v1/tema/auth/login", "application/json", &payload, 0, NULL, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (strcmp(parse_response, "ok")) {
                printf("Eroare, id sau parola incorecta\n");
            } else {
                parse_response = strstr(response, "Set-Cookie");
                parse_response = strchr(parse_response, ' ') + 1;
                char *ch;
                ch = strchr(parse_response, ';');
                *ch = '\0';
                printf("Succes: Utilizatorul a fost logat cu succes\n");
                strcpy(cookie, parse_response);
            }
            free(message);
            free(response);
            json_free_serialized_string(payload);
            json_value_free(root_value);
        }
        if (!strcmp(comm, "enter_library")) {
            message = compute_get_request("34.246.184.49:8080", "/api/v1/tema/library/access", NULL, cookie, NULL);
            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (!strncmp(parse_response + 2, "error", 5)) {
                printf("Eroare, userul nu este logat\n");
            } else {
                parse_response += 10;
                char *ch;
                ch = strrchr(parse_response, '"');
                *ch = '\0';
                printf("Succes: Utilizatorul a primit acces\n");
                strcpy(token, parse_response);
            }
            free(message);
            free(response);
        }
        if (!strcmp(comm, "get_books")) {
            message = compute_get_request("34.246.184.49:8080", "/api/v1/tema/library/books", NULL, cookie, token);
            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (!strncmp(parse_response + 2, "error", 5)) {
                printf("Eroare, userul nu are acces la librarie\n");
            } else {
                JSON_Value *root_value = json_parse_string(parse_response);
                parse_response = json_serialize_to_string_pretty(root_value);
                printf("Succes: This are the books:\n%s\n", parse_response);
                json_value_free(root_value);
            }
            free(message);
            free(response);
        }
        if (!strcmp(comm, "add_book")) {
            char s[200];
            fgets(s, 200, stdin);
            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            printf("title=");
            fgets(s, 200, stdin);
            s[strlen(s) - 1] = '\0';
            json_object_set_string(root_object, "title", s);
            printf("author=");
            fgets(s, 200, stdin);
            s[strlen(s) - 1] = '\0';
            json_object_set_string(root_object, "author", s);
            printf("genre=");
            fgets(s, 200, stdin);
            s[strlen(s) - 1] = '\0';
            json_object_set_string(root_object, "genre", s);
            printf("publisher=");
            fgets(s, 200, stdin);
            s[strlen(s) - 1] = '\0';
            char nr[200];
            printf("page_count=");
            fgets(nr, 200, stdin);
            int n = atoi(nr);
            if (n == 0) {
                printf("Eroare: Tip de date incorect pentru numarul de pagini\n");
                json_value_free(root_value);
                continue;
            }
            json_object_set_number(root_object, "page_count", n);
            json_object_set_string(root_object, "publisher", s);
            char *payload = json_serialize_to_string_pretty(root_value);
            message = compute_post_request("34.246.184.49:8080", "/api/v1/tema/library/books", "application/json", &payload, 0, cookie, token);
            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (!strcmp(parse_response, "ok"))
                printf("Succes: cartea a fost adaugata!\n");
            else
                printf("Eroare: nu s-a putut adauga cartea!\n");
            free(message);
            free(response);
            json_free_serialized_string(payload);
            json_value_free(root_value);
        }
        if (!strcmp(comm, "get_book")) {
            int id;
            printf("id=");
            scanf("%d", &id);
            char path[60];
            sprintf(path, "/api/v1/tema/library/books/%d", id);
            message = compute_get_request("34.246.184.49:8080", path, NULL, cookie, token);
            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (!strncmp(parse_response + 2, "error", 5)) {
                printf("Eroare, comanda nu s-a putut efectua\n");
            } else {
                JSON_Value *root_value = json_parse_string(parse_response);
                parse_response = json_serialize_to_string_pretty(root_value);
                printf("Succes: aceasta este cartea:\n%s\n", parse_response);
                json_value_free(root_value);
            }
            free(message);
            free(response);
        }
        if (!strcmp(comm, "delete_book")) {
            int id;
            printf("id=");
            scanf("%d", &id);
            char path[60];
            sprintf(path, "/api/v1/tema/library/books/%d", id);
            message = compute_delete_request("34.246.184.49:8080", path, NULL, cookie, token);
            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (!strncmp(parse_response + 2, "error", 5)) {
                printf("Eroare, comanda nu s-a putut efectua\n");
            } else {
                printf("Succes: cartea a fost stersa\n");
            }
            free(message);
            free(response);
        }
        if (!strcmp(comm, "logout")) {
            message = compute_get_request("34.246.184.49:8080", "/api/v1/tema/auth/logout", NULL, cookie, token);
            int sockfd = open_connection("34.246.184.49", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close_connection(sockfd);
            char *parse_response = strrchr(response, '\n') + 1;
            if (strcmp(parse_response, "ok"))
                printf("Eroare, userul nu este logat\n");
            else
                printf("Succes, userul a fost delogat\n");
            free(message);
            free(response);
            memset(cookie, 0, 100);
            memset(token, 0, 100);
        }
        if (!strcmp(comm, "exit")) {
            break;
        }
    }

    return 0;
}
