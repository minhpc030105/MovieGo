#ifndef FUNCTION_H
#define FUNCTION_H

#include <mysql/mysql.h>
#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"
#include "queryUser.h"

void handleLogin(int connfd, listLoginedAccount *arr, node *h, char *username, char *password);

void handleRegister(MYSQL *conn, int connfd, node *h, char *name, char *username, char *password);

void handleChangePassword(int connfd, MYSQL *conn, node* h);

void handleLogout(int connfd, listLoginedAccount *arr, char *username);

#endif
