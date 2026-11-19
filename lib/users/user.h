#ifndef USER_H
#define USER_H

struct User {
    unsigned long id;
    char name[256];
    char username[256];
    char password[256];
    char salt[65];
    unsigned long role_id;
};
typedef struct User user;

struct Node{
    user data;
    struct Node *next;
};
typedef struct Node* node;

#endif // USER_H
