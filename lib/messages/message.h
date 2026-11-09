#ifndef MESSAGE_H
#define MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

char* getTypeMessage(char *message);

void makeLoginMessage(char *username, char *password, char *message);

void getLoginMessage(char *username, char *password);

void resolveLoginMessage(char *username, char *password);

void makeRegisterMessage(char *name,char *username, char *password, char *message);

void getRegisterMessage(char *name, char *username, char *password);

void resolveRegisterMessage(char *name, char *username, char *password);

void makeChangePasswordMessage(char *username, char *oldPassword, char *newPassword, char *message);

void getChangePasswordMessage(char *username, char *oldPassword, char *newPassword);

void resolveChangePasswordMessage(char *username, char *oldPassword, char *newPassword);

void makeLogoutMessage(char *message);

void getLogoutMessage(char *username);

#ifdef __cplusplus
}
#endif

#endif
