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

void handleSearchFilmByTitle(MYSQL *conn, int connfd, char *title);

void handleShowFilm(MYSQL *conn, int connfd);
void handleShowCinemaByFilm(MYSQL *conn, int connfd);
void handleShowTimeByFilmCinema(MYSQL *conn, int connfd);
void handleShowSeat(MYSQL *conn, int connfd);
void handleBookTicket(MYSQL *conn, int connfd, char *username, char *showtime_id, char *seat_id);

void handleShowCategory(MYSQL *conn, int connfd);
void handleShowCinema(MYSQL *conn, int connfd);
void handleShowPremieredTime(MYSQL *conn, int connfd);

void handleBrowseCategory(MYSQL *conn, int connfd, char *category_id);
void handleBrowseCinema(MYSQL *conn, int connfd, char *cinema_id);
void handleBrowseShowTime(MYSQL *conn, int connfd, char *premiered_time_id);


void handleAddFilm(MYSQL *conn, int connfd, char *title, char *category_id, char *show_time);

void handleShowRoomsByCinema(MYSQL *conn, int connfd, char *cinema_id);
void handleShowShowtimesByRoom(MYSQL *conn, int connfd, char *room_id);
void handleAddShowTime(MYSQL *conn, int connfd, char *film_id, char *cinema_id, char *room_id, char *start_datetime);

void handleViewTickets(MYSQL *conn, int connfd, char *username);
void handleViewTicketDetail(MYSQL *conn, int connfd, char *ticket_id);

void handleShowAllUsers(MYSQL *conn, int connfd);
void handleDeleteUser(MYSQL *conn, int connfd);
void handleChangeUserRole(MYSQL *conn, int connfd);

#endif