#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queryUser.h"
#include "security_utils.h"

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
    if (mysql_query(connection, "SELECT id, name, username, password, salt, role_id FROM users")) {
        fprintf(stderr, "selectUser query error: %s\n", mysql_error(connection));
        return;
    }
    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) {
        return;
    }
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        x.id = strtoul(row[0], NULL, 10);
        strcpy(x.name, row[1]);
        strcpy(x.username, row[2]);
        strcpy(x.password, row[3]);
        strcpy(x.salt, (row[4] != NULL) ? row[4] : "");
        x.role_id = strtoul(row[5], NULL, 10);

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
        if (strcmp(*username, p->data.username) == 0) {
            int matched = 0;
            if (strlen(p->data.salt) > 0) {
                matched = verify_password(password, p->data.salt, p->data.password);
            } else {
                matched = (strcmp(password, p->data.password) == 0);
            }

            if (matched) {
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
    if (mysql_query(connection, query)) {
        fprintf(stderr, "Register check query error: %s\n", mysql_error(connection));
        return 0;
    }
    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) return 0;
    if (mysql_num_rows(result) > 0) {
        mysql_free_result(result);
        printf("Registration failed: Username '%s' already exists\n", newUser.username);
        return 0;
    }
    mysql_free_result(result);

    /* Băm mật khẩu với salt ngẫu nhiên trước khi lưu DB */
    char salt[SALT_HEX_LEN] = {0};
    char hashed_pwd[HASH_HEX_LEN] = {0};
    generate_salt(salt, sizeof(salt));
    hash_password(newUser.password, salt, hashed_pwd);

    sprintf(query, "INSERT INTO users (name, username, password, salt, role_id) VALUES ('%s','%s', '%s', '%s', %ld)",
            newUser.name, newUser.username, hashed_pwd, salt, newUser.role_id);
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
    sprintf(query, "SELECT password, salt FROM users WHERE username = '%s'", username);
    if (mysql_query(connection, query)) {
        return 0;
    }
    MYSQL_RES *result = mysql_store_result(connection);
    if (!result) return 0;

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return 0;
    }

    const char *stored_hash = row[0];
    const char *stored_salt = (row[1] != NULL) ? row[1] : "";

    int verified = 0;
    if (strlen(stored_salt) > 0) {
        verified = verify_password(oldPassword, stored_salt, stored_hash);
    } else {
        verified = (strcmp(stored_hash, oldPassword) == 0);
    }

    if (!verified) {
        mysql_free_result(result);
        return 0;
    }
    mysql_free_result(result);

    /* Sinh salt mới và băm mật khẩu mới */
    char new_salt[SALT_HEX_LEN] = {0};
    char new_hash[HASH_HEX_LEN] = {0};
    generate_salt(new_salt, sizeof(new_salt));
    hash_password(newPassword, new_salt, new_hash);

    sprintf(query, "UPDATE users SET password = '%s', salt = '%s' WHERE username = '%s'",
            new_hash, new_salt, username);
    if (mysql_query(connection, query)) {
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
