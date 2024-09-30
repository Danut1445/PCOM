#include "utils.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

int recv_all(int sockfd, void *buffer, size_t len)
{
	size_t bytes_received = 0;
	size_t bytes_remaining = len;

 	while (bytes_remaining) {
		int n = recv(sockfd, buffer + bytes_received, len - bytes_received, 0);
		if (n < 0) {
			return -1;
		}
		bytes_received += n;
		bytes_remaining -= n;
	}
	return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len)
{
	size_t bytes_sent = 0;
	size_t bytes_remaining = len;

  	while (bytes_remaining) {
		int n = send(sockfd, buffer + bytes_sent, len - bytes_sent, 0);
		if (n < 0) {
			return -1;
		}
		bytes_sent += n;
		bytes_remaining -= n;
	}
	return bytes_sent;
}

int compare_func(char *s1, char *s2) {
    if (s1[0] == '\0' && s2[0] == '\0')
        return 1;

    if (s1[0] == '*' && s1[1] != '\0' && s2[0] == '\0')
        return 0;

    if (s1[0] == s2[0])
        return compare_func(s1 + 1, s2 + 1);

    if (s1[0] == '+') {
        char *s = strchr(s2, '/');
        if (s == 0 && s1[1] != '\0')
            return 0;
        if (s == 0 && s1[1] == '\0')
            return 1;
        return compare_func(s1 + 1, s);
    }

    if (s1[0] == '*')
        return compare_func(s1 + 1, s2) || compare_func(s1, s2 + 1);

    return 0;
}