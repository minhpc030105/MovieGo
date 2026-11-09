#include <stdio.h>
#include <mysql/mysql.h>
#include <string.h>
#include "../../lib/loginedUser.h"
#include "../../lib/users/functionUser.h"

#define LOGIN_SUCCESS_USER "1010"
#define LOGIN_SUCCESS_MANAGER "1011"
#define LOGIN_SUCCESS_ADMIN "1012"
#define LOGIN_FAIL "2011"
#define LOGIN_ALREADY "2101"
#define LOGOUT_SUCCESS "1102"
#define LOGOUT_FAIL "2103"
#define REGISTER_SUCCESS "1101"
#define REGISTER_FAIL "2102"

void selectUser(MYSQL *connection, node* h, user x){
    mysql_query(connection, "SELECT * FROM users");
    MYSQL_RES *result = mysql_store_result(connection);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        x.id = strtoul(row[0], NULL, 10);
        strcpy(x.name, row[1]);
        strcpy(x.username, row[2]);
        strcpy(x.password, row[3]);
        x.role_id = strtoul(row[4], NULL, 10);

        addNode(h, x);

    }

    mysql_free_result(result);
}

listLoginedAccount createListLoginedAccount() {
    listLoginedAccount arr;
    arr.data = malloc(INITIAL_CAPACITY * sizeof(char *));
    arr.size = 0;
    arr.capacity = INITIAL_CAPACITY;
    return arr;
}

void addToListLoginedAccount(listLoginedAccount *arr, char **value) {
    if (arr->size >= arr->capacity) {
        arr->capacity += INCREMENT;
        arr->data = realloc(arr->data, arr->capacity * sizeof(char *));
    }
    arr->data[arr->size] = malloc((strlen(*value) + 1) * sizeof(char));
    strcpy(arr->data[arr->size], *value);
    arr->size++;
}

void freeListLoginedAccount(listLoginedAccount *arr) {
    for (size_t i = 0; i < arr->size; i++) {
        free(arr->data[i]);
    }
    free(arr->data);
    arr->size = 0;
    arr->capacity = 0;
}

int searchListLoginedAccount(const listLoginedAccount *arr, char **value) {
    for (size_t i = 0; i < arr->size; i++) {
        if (strcmp(arr->data[i], *value) == 0) {
            return i; 
        }
    }
    return -1; 
}

void deleteFromListLoginedAccount(listLoginedAccount *arr, char **value) {
    int index = searchListLoginedAccount(arr, value);

    if (index != -1) {
        free(arr->data[index]);  
        arr->data[index] = arr->data[arr->size - 1];  
        arr->size--;
    }

}

listLoginedAccount createListLoginedUser(listLoginedAccount arr){
    freeListLoginedAccount(&arr);
    arr = createListLoginedAccount();
    return arr;
}

int checkLogin(node head, char **username, char *password, listLoginedAccount *arr){
    int check_logined = searchListLoginedAccount(arr, username);
    printf("%d\n", check_logined);
    if(check_logined >= 0){
        return 3;
    }

    struct Node* p = head;
    while(p != NULL){
        if((strcmp(*username, p->data.username) == 0) && (strcmp(password, p->data.password) == 0)){
            if(p->data.role_id == 0){
                printf("%s\n", LOGIN_SUCCESS_ADMIN);
                return 0; 
            }else if(p->data.role_id == 1){
                printf("%s\n", LOGIN_SUCCESS_MANAGER);
                return 1;
            }else if(p->data.role_id == 2){
                printf("%s\n", LOGIN_SUCCESS_USER);
                return 2;
            }            
        }
        p = p->next;
    }

    printf("%s\n", LOGIN_FAIL);
    return -1;
}

int registerUser(MYSQL *connection, user newUser) {
    char query[1024];

    if (strlen(newUser.username) == 0 || strlen(newUser.name) == 0 || strlen(newUser.password) == 0) {
        printf("Registration failed: Empty fields\n");
        return 0;
    }

    sprintf(query, "SELECT username FROM users WHERE username = '%s'", newUser.username);
    mysql_query(connection, query);
    MYSQL_RES *result = mysql_store_result(connection);
    if (mysql_num_rows(result) > 0) {
        mysql_free_result(result);
        printf("Registration failed: Username '%s' already exists\n", newUser.username);
        return 0;
    }
    mysql_free_result(result);

    sprintf(query, "INSERT INTO users (name, username, password, role_id) VALUES ('%s','%s', '%s', %ld)", newUser.name, newUser.username, newUser.password, newUser.role_id);
    if (mysql_query(connection, query)) {
        fprintf(stderr, "Register Error: %s\n", mysql_error(connection));
        return 0;
    } else {
        printf("Registration successful: User '%s' created\n", newUser.username);
        return 1;
    }
}

int changePassword(MYSQL *connection, char *username, char *oldPassword, char *newPassword) {
    char query[1024];
    sprintf(query, "SELECT password FROM users WHERE username = '%s'", username);
    mysql_query(connection, query);
    MYSQL_RES *result = mysql_store_result(connection);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row || strcmp(row[0], oldPassword) != 0) {
        mysql_free_result(result);
        printf("fghjk");
        return 0;
    }
    printf("%s",row[0]);
    mysql_free_result(result);

    sprintf(query, "UPDATE users SET password = '%s' WHERE username = '%s'", newPassword, username);
    if (mysql_query(connection, query)) {
        printf("kkk");
        return 0;
    }

    return 1;
}

int getUserIdByUsername(MYSQL *conn, const char *username) {
    char query[256];
    sprintf(query, "SELECT id FROM users WHERE username='%s'", username);

    if (mysql_query(conn, query)) return -1;
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) return -1;

    MYSQL_ROW row = mysql_fetch_row(res);
    int id = -1;
    if (row) id = atoi(row[0]);

    mysql_free_result(res);
    return id;
}
