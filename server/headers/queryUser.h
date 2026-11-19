#ifndef QUERYUSER_H
#define QUERYUSER_H

#if defined(__has_include)
  #if __has_include(<mysql/mysql.h>)
    #include <mysql/mysql.h>
  #elif __has_include(<mysql.h>)
    #include <mysql.h>
  #else
    #include <mysql/mysql.h>
  #endif
#else
  #include <mysql/mysql.h>
#endif

#include "../../lib/loginedUser.h"
#include "../../lib/users/functionUser.h"

void selectUser(MYSQL *connection, node* h, user x);

listLoginedAccount createListLoginedAccount();
void addToListLoginedAccount(listLoginedAccount *arr, char **value);
void freeListLoginedAccount(listLoginedAccount *arr);
int searchListLoginedAccount(const listLoginedAccount *arr, char **value);
void deleteFromListLoginedAccount(listLoginedAccount *arr, char **value);
listLoginedAccount createListLoginedUser(listLoginedAccount arr);

int checkLogin(node head, char **username, char *password, listLoginedAccount *arr);
int registerUser(MYSQL *connection, user newUser);
int changePassword(MYSQL *connection, char *username, char *oldPassword, char *newPassword);

int getUserIdByUsername(MYSQL *conn, const char *username);

#endif
