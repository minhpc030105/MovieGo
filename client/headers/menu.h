#ifndef MENU_H
#define MENU_H

#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"

void viewWelcome();
void viewLogin(char username[], char password[]);
void viewRegister(char *name, char *username, char *password);
void viewChangePassword(char *oldPassword, char *newPassword);
void viewUser();

// void viewChangePassword(char *oldPassword, char *newPassword);

/*----------ADD FILM--------*/
void addNewFilm(int sockfd, char title[], char category_id[], char show_time[], char description[]);

/*----------END ADD FILM--------*/

/*----------FIND FILM--------*/
void getTitleFilm(char title[]);
/*----------END FIND FILM--------*/

/*----------BROWSE FILM--------*/
void browseFilm();

// void getCategoryID(char **category_id);
// void getCinemaID(char **cinema_id);

void getCategoryID(char *category_id);
void getPremieredTimeID(char *time_id);

/*----------END BROWSE FILM--------*/

void getCinemaID(char *cinema_id);

/*----------BOOK TICKET--------*/
void bookTicket();

void getInput(const char *prompt, char *out, int max_len);

/*----------VIEW TICKET--------*/
void viewTicket();
/*----------END VIEW TICKET--------*/

void getFilmID(char *film_id);



void getShowTimeID(char *showtime_id);

void getSeatID(char *seat_id);

/*----------END BOOK TICKET--------*/


void viewChangePassword(char *oldPassword, char *newPassword);
void viewManager();
void viewAdmin();
#endif