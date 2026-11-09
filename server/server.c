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
#include "headers/logger.h"
#include "headers/cache.h"
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
    
    // Khởi tạo logger
    initLogger();
    
    // Khởi tạo lists
    // Khởi tạo users
    arr = createListLoginedAccount(); 
    selectUser(init_conn, &head, (user){0});

    // Khởi tạo cache
    initCache(init_conn);

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
        
        // message nhận được
        if (strlen(message) > 0) {
            logMessage(connfd, "RECV", message);
        }
        
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
        
        /* ========== BROWSE FILM ========== */
        } else if(strcmp(cmd, "SHOW_CATEGORIES") == 0){
            handleShowCategory(conn, connfd);
    
        } else if(strcmp(cmd, "BROWSE_CATEGORY") == 0){  
            char category_id[256];
            resolveBrowseCategoryMessage(category_id);
            handleBrowseCategory(conn, connfd, category_id);

        } else if(strcmp(cmd, "SHOW_CINEMAS") == 0){  
            handleShowCinema(conn, connfd);

        } else if(strcmp(cmd, "BROWSE_CINEMA") == 0){ 
            char cinema_id[256];
            resolveBrowseCinemaMessage(cinema_id);
            handleBrowseCinema(conn, connfd, cinema_id);

        } else if(strcmp(cmd, "SHOW_PREMIERED_TIME") == 0){
            handleShowPremieredTime(conn, connfd);

        } else if(strcmp(cmd, "BROWSE_PREMIERED_TIME") == 0){  
            char time_slot[256];
            resolveBrowsePremieredTimeMessage(time_slot);
            handleBrowseShowTime(conn, connfd, time_slot);
            
        /* ========== BOOK TICKET ========== */
        } else if(strcmp(cmd, "SHOW_FILMS") == 0){  
            handleShowFilm(conn, connfd);

        } else if(strcmp(cmd, "SHOW_CINEMA_BY_FILM") == 0){  
            handleShowCinemaByFilm(conn, connfd);

        } else if(strcmp(cmd, "SHOW_TIME_BY_FILM_CINEMA") == 0){  
            handleShowTimeByFilmCinema(conn, connfd);

        } else if(strcmp(cmd, "SHOW_SEATS") == 0){  
            handleShowSeat(conn, connfd);

        } else if (strcmp(cmd, "BOOK_TICKET") == 0) {
            char showtime_id[256];
            char seat_id[256];
            resolveBookTicketMessage(showtime_id, seat_id);
            handleBookTicket(conn, connfd, username, showtime_id, seat_id);
        /* ========== SEARCH FILM ========== */
        } else if(strcmp(cmd, "SEARCH_FILM_BY_TITLE") == 0){  
            char title[256];
            resolveSearchFilmByTitleMessage(title);
            handleSearchFilmByTitle(conn, connfd, title);
        /* ========== ADD FILM ========== */
        } else if(strcmp(cmd, "ADD_FILM") == 0) {
            char title[256];
            char category_id[256];
            char show_time[256];
            resolveAddFilmMessage(title, category_id, show_time);
            printf("[DEBUG] ADD_FILM params - title: %s, category_id: %s, show_time: %s\n", title, category_id, show_time);
            handleAddFilm(conn, connfd, title, category_id, show_time);
        /* ========== VIEW TICKETS ========== */
        } else if(strcmp(cmd, "VIEW_TICKETS") == 0) {
            char username[256];
            resolveViewTicketsMessage(username);
            handleViewTickets(conn, connfd, username);
        } else if(strcmp(cmd, "VIEW_TICKET_DETAIL") == 0) {
            char ticket_id[256];
            resolveViewTicketDetailMessage(ticket_id);
            handleViewTicketDetail(conn, connfd, ticket_id);
        } else if(strcmp(cmd, "SHOW_ROOMS_BY_CINEMA") == 0) {
            char cinema_id[256];
            resolveShowRoomsByCinemaMessage(cinema_id);
            handleShowRoomsByCinema(conn, connfd, cinema_id);
        } else if(strcmp(cmd, "SHOW_SHOWTIMES_BY_ROOM") == 0) {
            char room_id[256];
            resolveShowShowtimesByRoomMessage(room_id);
            handleShowShowtimesByRoom(conn, connfd, room_id);
        } else if(strcmp(cmd, "ADD_SHOWTIME") == 0) {
            char film_id[256];
            char cinema_id[256];
            char room_id[256];
            char start_datetime[256];
            resolveAddShowTimeMessage(film_id, cinema_id, room_id, start_datetime);
            handleAddShowTime(conn, connfd, film_id, cinema_id, room_id, start_datetime);
        /* ========== ADMIN MANAGEMENT ========== */
        } else if(strcmp(cmd, "SHOW_ALL_USERS") == 0){
            handleShowAllUsers(conn, connfd);
        } else if(strcmp(cmd, "DELETE_USER") == 0){
            handleDeleteUser(conn, connfd);
            
        } else if(strcmp(cmd, "CHANGE_USER_ROLE") == 0){
            handleChangeUserRole(conn, connfd);
            
        } else {
            printf("[ERROR] Unknown command from fd %d: %s\n", connfd, cmd);
            sendMessage(connfd, "ERROR: Unknown command."); 
        }
    }

    mysql_close(conn);
    close(connfd);
    return NULL;
}
