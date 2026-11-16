#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "headers/function.h"
#include "headers/menu.h"
#include "../lib/resultCodes.h"

#define BUFSIZE 1024
#define STR_LEN 255

int main(int argc, char **argv) {
    if(argc != 3){
        printf("Usage: ./client_out <ip address> <number port>\n");
        exit(1);
    }

 
    int sockfd;
    struct sockaddr_in servaddr;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\nERROR: ");
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;  // IPV4
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("\nERROR: ");
        return 0;
    }


    char conn_mess[BUFSIZE];
    memset(conn_mess, 0, BUFSIZE);

    if (recv(sockfd, conn_mess, sizeof(conn_mess), 0) > 0) {
        printf("\n\n%s\n\n", conn_mess);
    }

    int choice;
    int login_status = 0;
    
    char message[STR_LEN];
    char username[STR_LEN];
    char password[STR_LEN];

    while (1) {
        viewWelcome();
        printf("Choice: ");
        scanf("%d", &choice);
        clearKeyboardBuffer();

        switch (choice) {
            case 1: {
                memset(message, 0, STR_LEN);
                memset(username, 0, STR_LEN);
                memset(password, 0, STR_LEN);

                handleLogin(sockfd, username, password, message, &login_status);
                
                if(login_status == LOGIN_SUCCESS_USER){
                    handleRequestUser(sockfd, username, message);
                } 
                else if(login_status == LOGIN_SUCCESS_MANAGER){
                    handleRequestManager(sockfd, username, message);
                }
                else if(login_status == LOGIN_SUCCESS_ADMIN){
                    handleRequestAdmin(sockfd, username, message);
                }
                break;
            }
            case 2: {
                memset(message, 0, STR_LEN);
                handleRegister(sockfd, message);
                break;
            }
            case 3: {
                printf("\n\nThanks for coming to MovieGo!!\n\n");
                
                sendMessage(sockfd, "EXIT");
                
                close(sockfd);
                return 0; 
            }
            default: {
                printf("Unknown request type!\n\n");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}
