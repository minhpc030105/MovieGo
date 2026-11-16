#ifndef FUNCTIONCLIENT_H
#define FUNCTIONCLIENT_H

#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"
#include "menu.h"

void clearKeyboardBuffer();
void handleLogin(int sockfd, char *username, char *password, char *message, int *login_status);
void handleRegister(int sockfd, char *message);
void handleRequestUser(int sockfd, char *username, char *message);
void handleRequestAdmin(int sockfd, char *username, char *message);
void handleLogout(int sockfd, char *message);
void handleChangePassword(int sockfd, char *username, char *message);

/*----------BOOK TICKET--------*/
void handleBookTicket(int sockfd);
/*----------END BOOK TICKET--------*/

/*----------VIEW TICKET--------*/
void handleViewTickets(int sockfd, char *username);
void handleViewTicketDetail(int sockfd);
/*----------END VIEW TICKET--------*/

/*----------FIND FILM--------*/
void handleSearchByTitle(int sockfd);
/*----------END FIND FILM--------*/

/*----------BROWSE FILM--------*/
void handleBrowseFollowCategory(int sockfd);
void handleBrowseFollowCinema(int sockfd);
void handleBrowseFollowShowTime(int sockfd);
void handleBrowseFilm(int sockfd);
/*----------END BROWSE FILM--------*/

/*----------ADMIN MANAGEMENT--------*/
void handleShowAllUsers(int sockfd);
void handleDeleteUser(int sockfd);
void handleChangeUserRole(int sockfd);
/*----------END ADMIN MANAGEMENT--------*/

/*----------MANAGER--------*/
void handleRequestManager(int sockfd, char *username, char *message);
/*-----ADD FILM-----*/
void handleAddFilm(int sockfd);
/*-----END ADD FILM-----*/

/*-----ADD SHOW TIME-----*/
void handleAddShowTime(int sockfd);
/*-----END ADD SHOW TIME-----*/
/*----------END MANAGER----------*/

#endif