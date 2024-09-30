#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                            char *cookie, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookie != NULL) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                            char *cookie, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookie != NULL) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char *cookie, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    int content_length = 0;
    if (body_data_fields_count == 0) {
        content_length += strlen(body_data[0]);
    }

    for (int i = 0; i < body_data_fields_count; i++) {
        content_length += strlen(body_data[i]);
        if (i != body_data_fields_count - 1)
            content_length++;
    }
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    sprintf(line, "Content-Length: %d", content_length);
    compute_message(message, line);
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    // Step 4 (optional): add cookies
    if (cookie != NULL) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }
    // Step 5: add new line at end of header
    compute_message(message, "");
    if (body_data_fields_count == 0) {
        strcat(message, body_data[0]);

    }
    
    // Step 6: add the actual payload data
    for (int i = 0; i < body_data_fields_count; i+= 2) {
        sprintf(line, "%s=%s", body_data[i], body_data[i + 1]);
        strcat(message, line);
        if (i != body_data_fields_count - 2)
            strcat(message, "&");
    }

    compute_message(message, "");
    free(line);
    return message;
}
