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

/*----------ADD FILM--------*/
void makeAddNewFilmMessage(
    char *title,
    char *category_id,
    char *show_time,
    char *description,
    char *message
);
void makeAddFilmMessage(char *title, char *category_id, char *show_time, char *message);
void resolveAddFilmMessage(char *title, char *category_id, char *show_time);
/*----------END ADD FILM--------*/

/*----------FIND FILM--------*/
void makeSearchFilmByTitleMessage(char *title, char *message);
void getTitleFromMessage(char *title);
void resolveSearchFilmByTitleMessage(char *title);
/*----------END FIND FILM--------*/

/*----------BROWSE FILM--------*/

void makeShowCategoryMessage(char *message);
void makeShowCinemaMessage(char *message);
void makeShowPremieredTimeMessage(char *message);

void makeBrowseFollowCategoryMessage(char *category_id, char *message);
void makeBrowseFollowCinemaMessage(char *cinema_id, char *message);
void makeBrowseFollowPremieredTimeMessage(char *premiered_time_id, char *message);

void getCategoryIDFromMessage(char *message, char *category_id);
void getCinemaIDFromMessage(char *message, char *cinema_id);
void getPremieredTimeIDFromMessage(char *message, char *premiered_time_id);

void resolveBrowseCategoryMessage(char *category_id);
void resolveBrowseCinemaMessage(char *cinema_id);
void resolveBrowsePremieredTimeMessage(char *premiered_time_id);
/*----------END BROWSE FILM--------*/

/*----------BOOK TICKET--------*/
void makeShowFilmMessage(char *message);

void makeShowCinemaByFilmMessage(char *film_id, char *message);

void makeShowTimeByFilmCinemaMessage(char *film_id, char *cinema_id, char *message);

void makeShowSeatMessage(char *showtime_id, char *message);

void makeBookTicketMessage(char *showtime_id, char *seat_id, char *message);

void getBookTicketMessage(char *showtime_id, char *seat_id);

void resolveShowCinemaByFilmMessage(char *film_id);
void resolveShowTimeByFilmCinemaMessage(char *film_id, char *cinema_id);
void resolveShowSeatMessage(char *showtime_id);
void resolveBookTicketMessage(char *showtime_id, char *seat_id);
/*----------END BOOK TICKET--------*/

void makeAnnounceFilmMessage(char *film_id, char *cinema_id, char *premiered_time_id, char *date, char *message);

void getAnnounceFilmMessage(char *film_id, char *cinema_id, char *premiered_time_id, char *date);

/*----------VIEW TICKET--------*/
void makeViewTicketsMessage(char *username, char *message);
void makeViewTicketDetailMessage(char *ticket_id, char *message);
void resolveViewTicketsMessage(char *username);
void resolveViewTicketDetailMessage(char *ticket_id);
/*----------END VIEW TICKET--------*/

/*----------SHOW TIME--------*/
void makeShowRoomsByCinemaMessage(char *cinema_id, char *message);
void makeShowShowtimesByRoomMessage(char *room_id, char *message);
void makeAddShowtimeMessage(char *film_id, char *cinema_id, char *room_id, char *start_datetime, char *message);
void resolveShowRoomsByCinemaMessage(char *cinema_id);
void resolveShowShowtimesByRoomMessage(char *room_id);
void resolveAddShowTimeMessage(char *film_id, char *cinema_id, char *room_id, char *start_datetime);
/*----------END SHOW TIME--------*/

/*----------ADMIN--------*/
void makeShowAllUsersMessage(char *message);
void makeDeleteUserMessage(char *user_id, char *message);
void makeChangeUserRoleMessage(char *user_id, char *new_role, char *message);
void resolveDeleteUserMessage(char *user_id);
void resolveChangeUserRoleMessage(char *user_id, char *new_role);
/*----------END ADMIN--------*/

#ifdef __cplusplus
}
#endif

#endif