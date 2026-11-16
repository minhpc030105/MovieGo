#ifndef FUNCTIONUSER_H
#define FUNCTIONUSER_H

#include "user.h"

int checkEmptyList(node list);

void addNewNode(node* head, user x);

void addNode(node* head, user x);

void changeNodePassword(node *head, char username[], char newPassword[]);

#endif
