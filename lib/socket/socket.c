#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 4096

//gửi chuỗi ký tự
void sendMessage(int fd, char *message){
    int length = htonl(strlen(message));
    send(fd, &length, sizeof(length), 0);
    send(fd, message, strlen(message), 0);
}

// đọc chuỗi ký tự 
void recvMessage(int fd, char *message){
    int length = 0;
    char buffer[BUFSIZE];
    memset(&buffer, 0, BUFSIZE);
    recv(fd, &length, sizeof(length), 0);
    int msg_len = ntohl(length);
    if (msg_len >= BUFSIZE) {
        msg_len = BUFSIZE - 1;
    }
    recv(fd, buffer, msg_len, 0);
    buffer[msg_len] = '\0';
    strcpy(message, buffer);
}

// gửi sô nguyên
void sendResult(int fd, int result){
    int message = htonl(result);
    send(fd, &message, sizeof(message), 0);
}

// đọc số nguyên
int recvResult(int fd){
    int result;
    recv(fd, &result, sizeof(result), 0);
    return ntohl(result);
}