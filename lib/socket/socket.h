#ifndef SOCKET_H
#define SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

void sendMessage(int fd, char *message);
void recvMessage(int fd, char *message);
void sendResult(int fd, int result);
int recvResult(int fd);

#ifdef __cplusplus
}
#endif

#endif