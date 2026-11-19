#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "../db/connect.h"
#include "../lib/socket/socket.h"
#include "headers/cache.h"
#include "headers/function.h"
#include "headers/logger.h"

static void *serve_client(void *arg) {
    int fd = *(int *)arg;
    free(arg);
    pthread_detach(pthread_self());
    struct timeval idle_timeout = {.tv_sec = 30 * 60};
    Session session = {.role_id = -1};
    char request[MAX_RESP_BUF_SIZE + 1];
    char response[MAX_RESP_BUF_SIZE + 1];

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &idle_timeout, sizeof(idle_timeout)) == 0) {
        connectDatabase(&session.db);
        while (session.db && !session.closing && recv_frame(fd, request, sizeof(request)) == 0) {
            int code = dispatch_request(&session, request, response, sizeof(response));
            logResult(fd, "SEND", code);
            if (sendMessage(fd, response) < 0) break;
        }
    }
    clear_session(&session);
    if (session.db) mysql_close(session.db);
    mysql_thread_end();
    close(fd);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./server_out <port>\n");
        return 1;
    }
    if (mysql_library_init(0, NULL, NULL) != 0) return 1;
    MYSQL *db = NULL;
    connectDatabase(&db);
    if (!db) return 1;
    initCache(db);
    mysql_close(db);
    initLogger();

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    struct sockaddr_in addr = {
        .sin_family = AF_INET, .sin_port = htons(atoi(argv[1])), .sin_addr.s_addr = htonl(INADDR_ANY)
    };
    if (listener < 0 || setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0 ||
        bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(listener, 20) < 0) {
        perror("listen");
        return 1;
    }
    for (;;) {
        int fd = accept(listener, NULL, NULL);
        if (fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        int *arg = malloc(sizeof(*arg));
        if (arg) {
            *arg = fd;
            pthread_t tid;
            if (pthread_create(&tid, NULL, serve_client, arg) == 0) continue;
            free(arg);
        }
        close(fd);
    }
    close(listener);
    return 1;
}
