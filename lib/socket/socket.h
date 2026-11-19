#ifndef SOCKET_H
#define SOCKET_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_RESP_BUF_SIZE 65536

int send_all(int fd, const void *data, size_t length);
int recv_all(int fd, void *data, size_t length);
int sendMessage(int fd, const char *message);
int recv_frame(int fd, char *buffer, size_t capacity);

typedef struct {
    char text[MAX_RESP_BUF_SIZE + 1];
    char *next;
} MovieResponse;

int receive_response(int fd, MovieResponse *response);
char *next_response_line(MovieResponse *response);
int recvResult(int fd);

#ifdef __cplusplus
}
#endif
#endif
