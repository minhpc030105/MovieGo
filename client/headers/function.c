#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"
#include "menu.h"
#include "function.h"
#include "../../lib/resultCodes.h"

void clearKeyboardBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {

    }
}

void handleLogin(int sockfd, char *username, char *password, char *message, int *login_status){
    viewLogin(username, password);
    makeLoginMessage(username, password, message);
    sendMessage(sockfd, message);
    *login_status = recvResult(sockfd);
    if(*login_status == LOGIN_FAIL){
        printf("Your username or password is incorrect!!\n");
    }else if(*login_status == LOGIN_ALREADY){
        printf("Your account is being used in another address!!\n");
    }else{
        printf("You have logged in successfully!!\n");
    }
}

void handleRegister(int sockfd, char *message){
    char newName[255], newUsername[255], newPassword[255];

    viewRegister(newName, newUsername, newPassword);
    makeRegisterMessage(newName, newUsername, newPassword, message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    if (result == REGISTER_SUCCESS) {
        printf("You have registered successfully!!\n");
    } else {
        printf("Username has been existed!!\n");
    }
}

void handleRequestUser(int sockfd, char *username, char *message, const char *role) {
    int user_choice;
    do {
        viewUser(role);
        printf("Choice: ");
        scanf("%d", &user_choice);
        clearKeyboardBuffer();
        switch(user_choice) {
            case 1:{
                handleChangePassword(sockfd, username, message);
                break;
            }
            case 2:{
                handleLogout(sockfd, message);
                user_choice = 0;
                break;
            }
        }
    } while (user_choice != 0);
}

void handleLogout(int sockfd, char *message){
    makeLogoutMessage(message);
    sendMessage(sockfd, message);
    recvResult(sockfd);
    printf("You have been successfully logged out!!\n");
}

void handleChangePassword(int sockfd, char *username, char *message){
    char oldPassword[255], newPassword[255];
    viewChangePassword(oldPassword, newPassword);
    makeChangePasswordMessage(username, oldPassword, newPassword, message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    // printf("%d\n", result);
    if (result == CHANGE_PASSWORD_SUCCESS) {
        printf("Changed password successfully\n");
    } else {
        printf("Changed password fail!!\n");
    }
}
