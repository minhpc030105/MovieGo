#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"

void viewWelcome(){
    printf("--------------Welcome------------------\n\n");
    printf("1. Login\n");
    printf("2. Register\n");
    printf("3. Exit\n\n");
    printf("---------------------------------------------------\n");
}

void viewLogin(char username[], char password[]){
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
}

void viewRegister(char *name, char *username, char *password) {
    printf("Registration\n");
    printf("Full Name: ");
    memset(name, 0, 255);
    fgets(name, 255, stdin);
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
}

void viewUser(const char *role){
    printf("--------------Welcome to MovieGo------------------\n\n");
    printf("Role: %s\n\n", role);
    printf("1. Change Password\n");
    printf("2. Logout\n");
    printf("---------------------------------------------------\n");
}

void viewChangePassword(char *oldPassword, char *newPassword) {
    printf("Change Password\n");
    printf("Old Password: ");
    scanf("%s", oldPassword);
    printf("New Password: ");
    scanf("%s", newPassword);
}
