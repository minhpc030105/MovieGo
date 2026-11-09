#include <stdio.h>
#include <string.h>

char* getTypeMessage(char *message){
    return strtok(message, "\r\n");
}

void makeLoginMessage(char *username, char *password, char *message){
    strcpy(message, "LOGIN\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, password);
    strcat(message, "\r\n");
}

void resolveLoginMessage(char *username, char *password){
    char *tmp_username = strtok(NULL, "\r\n");
    char *tmp_password = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
    if(tmp_password) strcpy(password, tmp_password);
}

void makeRegisterMessage(char *name,char *username, char *password, char *message){
    strcpy(message, "REGISTER\r\n");
    strcat(message, name);
    strcat(message, "\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, password);
    strcat(message, "\r\n");
}

void resolveRegisterMessage(char *name, char *username, char *password){
    char *tmp_name = strtok(NULL, "\r\n");
    char *tmp_username = strtok(NULL, "\r\n");
    char *tmp_password = strtok(NULL, "\r\n");
    if(tmp_name) strcpy(name, tmp_name);
    if(tmp_username) strcpy(username, tmp_username);
    if(tmp_password) strcpy(password, tmp_password);
}

void makeChangePasswordMessage(char *username, char *oldPassword, char *newPassword, char *message) {
    strcpy(message, "CHANGE_PASSWORD\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, oldPassword);
    strcat(message, "\r\n");
    strcat(message, newPassword);
    strcat(message, "\r\n");
}

void resolveChangePasswordMessage(char *username, char *oldPassword, char *newPassword) {
    char *tmp_username = strtok(NULL, "\r\n");
    char *tmp_oldPassword = strtok(NULL, "\r\n");
    char *tmp_newPassword = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
    if(tmp_oldPassword) strcpy(oldPassword, tmp_oldPassword);
    if(tmp_newPassword) strcpy(newPassword, tmp_newPassword);
}

void makeLogoutMessage(char *message){
    strcpy(message, "LOGOUT\r\n");
}

void resolveLogoutMessage(char *username) {
    char *tmp_username = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
}
