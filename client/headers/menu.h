#ifndef MENU_H
#define MENU_H

#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"

void viewWelcome();
void viewLogin(char username[], char password[]);
void viewRegister(char *name, char *username, char *password);
void viewChangePassword(char *oldPassword, char *newPassword);
void viewUser(const char *role);

#endif
