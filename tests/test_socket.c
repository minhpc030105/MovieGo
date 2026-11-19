#include "../lib/socket/socket.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static char payload[MAX_RESP_BUF_SIZE + 1];

static void *send_large_frame(void *arg) {
    assert(sendMessage(*(int *)arg, payload) == 0);
    return NULL;
}

int main(void) {
    int peers[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, peers) == 0);
    int small_buffer = 1024;
    assert(setsockopt(peers[0], SOL_SOCKET, SO_SNDBUF, &small_buffer, sizeof(small_buffer)) == 0);
    memset(payload, 'x', MAX_RESP_BUF_SIZE);
    pthread_t writer;
    assert(pthread_create(&writer, NULL, send_large_frame, peers) == 0);
    char received[MAX_RESP_BUF_SIZE + 1];
    assert(recv_frame(peers[1], received, sizeof(received)) == 0);
    assert(strcmp(received, payload) == 0);
    pthread_join(writer, NULL);

    struct timeval timeout = {.tv_usec = 50000};
    assert(setsockopt(peers[1], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0);
    assert(send_all(peers[0], "\0\0", 2) == 0);
    assert(recv_frame(peers[1], received, sizeof(received)) < 0);
    assert(errno == EAGAIN || errno == EWOULDBLOCK);
    close(peers[1]);
    assert(sendMessage(peers[0], "EXIT") < 0);
    close(peers[0]);
    return 0;
}
