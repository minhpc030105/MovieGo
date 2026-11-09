#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "../lib/socket/socket.h"
#include "../lib/messages/message.h"
#include "headers/function.h"
#include "headers/queryUser.h"
#include "../db/connect.h"

#define BACKLOG 20

#define LOGIN_SUCCESS_USER 1010
#define LOGIN_SUCCESS_MANAGER 1011
#define LOGIN_SUCCESS_ADMIN 1012
#define LOGIN_FAIL 2011
#define REGISTER_SUCCESS 1020
#define REGISTER_FAIL 2021
#define LOGOUT_SUCCESS 1030

pthread_mutex_t arr_lock = PTHREAD_MUTEX_INITIALIZER;  // Protect logged users list
pthread_mutex_t head_lock = PTHREAD_MUTEX_INITIALIZER; // Protect user linked list

listLoginedAccount arr;
node head = NULL;





void *handleCommunicate(void *);

int main(int argc, char **argv){
    if(argc != 2){
        printf("Usage: ./server_out <number port>\n");
        exit(1);
    }

     // Khởi tạo và test DB connection
    MYSQL *init_conn;
    connectDatabase(&init_conn);
    if(init_conn == NULL){
        printf("Cannot connect to database\n");
        exit(1);
    }
    printf("Connected to database successfully\n");
    
    // Khởi tạo lists
    // Khởi tạo users
    arr = createListLoginedAccount(); 
    selectUser(init_conn, &head, (user){0});

    mysql_close(init_conn); 

    int listenfd, *connfd;
    struct sockaddr_in server;
    struct sockaddr_in *client;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    pthread_t tid;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\nError: ");
        return 0;
    }

    
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("\nError: ");
        return 0;
    }

    if (listen(listenfd, BACKLOG) == -1) {
        perror("\nError: ");
        return 0;
    }

    printf("Server is listening on port %s...\n", argv[1]);
    client = malloc(sin_size);


    while (1) {
        connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, (struct sockaddr*) client, &sin_size);

        if (*connfd == -1) {
            perror("\nError: ");
            free(connfd);
            continue;
        }

        // Tạo luồng mới
        pthread_create(&tid, NULL, &handleCommunicate, connfd);
    }
    free(client);
    close(listenfd);
    return 0;
}

void *handleCommunicate(void* arg) {
    int connfd = *((int*) arg);
    free(arg);
    pthread_detach(pthread_self());

    MYSQL *conn;
    connectDatabase(&conn);
    if (conn == NULL) {
        close(connfd);
        return NULL;
    }
    
    if(connfd == -1){
        return NULL;
    }
    
    printf("Client connected (fd: %d)\n", connfd);
    send(connfd, "Connected to server successfully!", 255, 0);

    char message[255];
    char message_copy[255];
    char username[255] = {0};

    while(1){
        memset(message, 0, 255);
        
    
        recvMessage(connfd, message); 
        
        // Nếu message rỗng hoặc client đóng kết nối
        if (strlen(message) == 0) {
            printf("Client (fd: %d) disconnected unexpected.\n", connfd);
            if (strlen(username) > 0) {
                handleLogout(connfd, &arr, username);
            }
            break;
        }

        if(strcmp(message, "EXIT") == 0){
            printf("Client (fd: %d) disconnected safely.\n", connfd);
            if (strlen(username) > 0) {
                handleLogout(connfd, &arr, username);
            }
            break;
        }
        
        strcpy(message_copy, message); 

        char *cmd = strtok(message, "\r\n");
        if(cmd == NULL) {
            continue;
        }

        printf("[DEBUG] Request from fd %d: %s\n", connfd, cmd);
        
        /* ========== AUTHENTICATION ========== */
        if(strcmp(cmd, "LOGIN") == 0){
            char username_param[256];
            char password[256];
            resolveLoginMessage(username_param, password);
            strcpy(username, username_param);
            handleLogin(connfd, &arr, &head, username_param, password);
            
        } else if(strcmp(cmd, "REGISTER") == 0){
            char name[256];
            char reg_username[256];
            char reg_password[256];
            resolveRegisterMessage(name, reg_username, reg_password);
            handleRegister(conn, connfd, &head, name, reg_username, reg_password);
            
        } else if(strcmp(cmd, "LOGOUT") == 0){
            handleLogout(connfd, &arr, username);
            memset(username, 0, 255);
            
        } else if(strcmp(cmd, "CHANGE_PASSWORD") == 0){
            handleChangePassword(connfd, conn, &head);

        } else {
            printf("[ERROR] Unknown command from fd %d: %s\n", connfd, cmd);
            sendMessage(connfd, "ERROR: Unknown command."); 
        }
    }

    mysql_close(conn);
    close(connfd);
    return NULL;
}
