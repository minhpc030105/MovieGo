#ifndef MESSAGE_H
#define MESSAGE_H
#ifdef __cplusplus
extern "C" {
#endif

void makeLoginMessage(char *username, char *password, char *message);
void makeRegisterMessage(char *name,char *username, char *password, char *message);
void makeChangePasswordMessage(char *oldPassword, char *newPassword, char *message);
void makeLogoutMessage(char *message);
void makeAddFilmMessage(char *title, char *category_id, char *show_time, char *message);
void makeSearchFilmByTitleMessage(char *title, char *message);
void makeShowCategoryMessage(char *message);
void makeShowCinemaMessage(char *message);
void makeShowPremieredTimeMessage(char *message);
void makeBrowseFollowCategoryMessage(char *category_id, char *message);
void makeBrowseFollowCinemaMessage(char *cinema_id, char *message);
void makeBrowseFollowPremieredTimeMessage(char *premiered_time_id, char *message);
void makeShowFilmMessage(char *message);
void makeShowCinemaByFilmMessage(char *film_id, char *message);
void makeShowTimeByFilmCinemaMessage(char *film_id, char *cinema_id, char *message);
void makeShowSeatMessage(char *showtime_id, char *message);
void makeBookTicketMessage(char *showtime_id, char *seat_id, char *message);
void makeAnnounceFilmMessage(char *film_id, char *cinema_id, char *premiered_time_id, char *date, char *message);
void makeViewTicketsMessage(char *message);
void makeViewTicketDetailMessage(char *ticket_id, char *message);
void makeShowRoomsByCinemaMessage(char *cinema_id, char *message);
void makeShowShowtimesByRoomMessage(char *room_id, char *message);
void makeAddShowtimeMessage(char *film_id, char *cinema_id, char *room_id, char *start_datetime, char *message);
void makeShowAllUsersMessage(char *message);
void makeDeleteUserMessage(char *user_id, char *message);
void makeChangeUserRoleMessage(char *user_id, char *new_role, char *message);

#ifdef __cplusplus
}
#endif
#endif
