#include "socket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int send_all(int fd, const void *data, size_t length) {
    const char *cursor = data;
    while (length) {
        ssize_t n = send(fd, cursor, length, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        cursor += n;
        length -= (size_t)n;
    }
    return 0;
}

int recv_all(int fd, void *data, size_t length) {
    char *cursor = data;
    while (length) {
        ssize_t n = recv(fd, cursor, length, 0);
        if (n < 0 && errno == EINTR) continue;
        // EOF and SO_RCVTIMEO (EAGAIN/EWOULDBLOCK) both end this session.
        if (n <= 0) return -1;
        cursor += n;
        length -= (size_t)n;
    }
    return 0;
}

int sendMessage(int fd, const char *message) {
    size_t length = strlen(message);
    if (!length || length > MAX_RESP_BUF_SIZE) return -1;
    uint32_t header = htonl((uint32_t)length);
    if (send_all(fd, &header, sizeof(header)) < 0) return -1;
    return send_all(fd, message, length);
}

int recv_frame(int fd, char *buffer, size_t capacity) {
    uint32_t header;
    if (recv_all(fd, &header, sizeof(header)) < 0) return -1;
    size_t length = ntohl(header);
    if (!length || length > MAX_RESP_BUF_SIZE || length >= capacity) return -1;
    if (recv_all(fd, buffer, length) < 0 || memchr(buffer, 0, length)) return -1;
    buffer[length] = '\0';
    return 0;
}

char *next_response_line(MovieResponse *response) {
    char *line = response->next;
    if (!line) return NULL;
    char *end = strstr(line, "\r\n");
    response->next = end ? end + 2 : NULL;
    if (end) *end = '\0';
    return line;
}

int receive_response(int fd, MovieResponse *response) {
    response->next = NULL;
    if (recv_frame(fd, response->text, sizeof(response->text)) < 0) return -1;
    response->next = response->text;
    char *code = next_response_line(response);
    if (strlen(code) != 4 || strspn(code, "0123456789") != 4) return -1;
    return atoi(code);
}

int recvResult(int fd) {
    MovieResponse response;
    return receive_response(fd, &response);
}
