#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"
#include "queryUser.h"
#include "function.h"
#include "../../lib/resultCodes.h"

// External mutex declarations
extern pthread_mutex_t arr_lock;
extern pthread_mutex_t head_lock;

void handleLogin(int connfd, listLoginedAccount *arr, node *h, char *username, char *password){
    pthread_mutex_lock(&arr_lock);
    pthread_mutex_lock(&head_lock);
    
    int check = checkLogin(*h, &username, password, arr);
    printf("Login check result: %d\n", check);

    if (check == -1) {
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_FAIL);
    }
    else if (check == 0) {
        addToListLoginedAccount(arr, &username);
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_SUCCESS_ADMIN);
    }
    else if (check == 1) {
        addToListLoginedAccount(arr, &username);
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_SUCCESS_MANAGER);
    }
    else if (check == 2) {
        addToListLoginedAccount(arr, &username);
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_SUCCESS_USER);
    }
    else if (check == 3) {
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_ALREADY);
    }
}

void handleLogout(int connfd, listLoginedAccount *arr, char *username){
    printf("Logout user: %s\n", username);
    
    pthread_mutex_lock(&arr_lock);
    deleteFromListLoginedAccount(arr, &username);
    pthread_mutex_unlock(&arr_lock);
    
    sendResult(connfd, LOGOUT_SUCCESS);
}

void handleRegister(MYSQL *conn, int connfd, node *h, char *name, char *username, char *password){
    printf("[DEBUG] Register - name: '%s', username: '%s', password: '%s'\n", name, username, password);

    user newUser;
    strncpy(newUser.name, name, sizeof(newUser.name) - 1);
    newUser.name[sizeof(newUser.name) - 1] = '\0';
    
    strncpy(newUser.username, username, sizeof(newUser.username) - 1);
    newUser.username[sizeof(newUser.username) - 1] = '\0';
    
    strncpy(newUser.password, password, sizeof(newUser.password) - 1);
    newUser.password[sizeof(newUser.password) - 1] = '\0';
    
    newUser.role_id = 2;

    int result = registerUser(conn, newUser);

    if (result == 1) {
        pthread_mutex_lock(&head_lock);
        addNode(h, newUser);
        pthread_mutex_unlock(&head_lock);
        sendResult(connfd, REGISTER_SUCCESS);
    } else {
        sendResult(connfd, REGISTER_FAIL);
    }
}

void handleChangePassword( int connfd, MYSQL *conn, node *h){
    char username[255] = {0};
    char oldPassword[255] = {0};
    char newPassword[255] = {0};

    resolveChangePasswordMessage(username, oldPassword, newPassword);

    int result = changePassword(conn, username, oldPassword, newPassword);
    printf("Change password result: %d\n", result);

    if (result == 1) {
        pthread_mutex_lock(&head_lock);
        changeNodePassword(h, username, newPassword);
        pthread_mutex_unlock(&head_lock);
        sendResult(connfd, CHANGE_PASSWORD_SUCCESS);
    } else {
        
        sendResult(connfd, CHANGE_PASSWORD_FAIL);
    }
}
