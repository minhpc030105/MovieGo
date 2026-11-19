#include <stdio.h>
#include <string.h>

void makeLoginMessage(char *username, char *password, char *message){
    strcpy(message, "LOGIN\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, password);
    strcat(message, "\r\n");
}

void makeRegisterMessage(char *name,char *username, char *password, char *message){
    strcpy(message, "REGISTER\r\n");
    strcat(message, name);
    strcat(message, "\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, password);
    strcat(message, "\r\n");
}

void makeChangePasswordMessage(char *oldPassword, char *newPassword, char *message) {
    strcpy(message, "CHANGE_PASSWORD\r\n");
    strcat(message, oldPassword);
    strcat(message, "\r\n");
    strcat(message, newPassword);
    strcat(message, "\r\n");
}

void makeLogoutMessage(char *message){
    strcpy(message, "LOGOUT\r\n");
}

/*----------ADD FILM--------*/
void makeAddFilmMessage(
    char *title,
    char *category_id,
    char *show_time,
    char *message
){
    strcpy(message, "ADD_FILM\r\n");
    strcat(message, title);
    strcat(message, "\r\n");
    strcat(message, category_id);
    strcat(message, "\r\n");
    strcat(message, show_time);
    strcat(message, "\r\n");
}

/*----------END ADD FILM--------*/

/*----------FIND FILM--------*/

void makeSearchFilmByTitleMessage(char *title, char *message){
    strcpy(message, "SEARCH_FILM_BY_TITLE\r\n");
    strcat(message, title);
    strcat(message, "\r\n");
}
/*----------END FIND FILM--------*/

/*----------BROWSE FILM--------*/

void makeShowCategoryMessage(char *message) {
    strcpy(message, "SHOW_CATEGORIES\r\n");
}

void makeShowCinemaMessage(char *message) {
    strcpy(message, "SHOW_CINEMAS\r\n");
}

void makeShowPremieredTimeMessage(char *message) {
    strcpy(message, "SHOW_PREMIERED_TIME\r\n");
}

void makeBrowseFollowCategoryMessage(char *category_id, char *message){
    strcpy(message, "BROWSE_CATEGORY\r\n");
    strcat(message, category_id);
    strcat(message, "\r\n");
}

void makeBrowseFollowCinemaMessage(char *cinema_id, char *message){
    strcpy(message, "BROWSE_CINEMA\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
}

void makeBrowseFollowPremieredTimeMessage(char *premiered_time_id, char *message){
    strcpy(message, "BROWSE_PREMIERED_TIME\r\n");
    strcat(message, premiered_time_id);
    strcat(message, "\r\n");
}

/*----------END BROWSE FILM--------*/

/*----------BOOK TICKET--------*/
void makeShowFilmMessage(char *message) {
    // Client yêu cầu danh sách phim
    strcpy(message, "SHOW_FILMS\r\n");
}
void makeShowCinemaByFilmMessage(char *film_id, char *message) {
    // Client yêu cầu danh sách rạp theo film đã chọn
    strcpy(message, "SHOW_CINEMA_BY_FILM\r\n");
    strcat(message, film_id);
    strcat(message, "\r\n");
}

void makeShowTimeByFilmCinemaMessage(char *film_id, char *cinema_id, char *message) {
    // Client yêu cầu suất chiếu theo film + cinema
    
    strcpy(message, "SHOW_TIME_BY_FILM_CINEMA\r\n");
    strcat(message, film_id);
    strcat(message, "\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
}

void makeShowSeatMessage(char *showtime_id, char *message) {
    // Client yêu cầu danh sách ghế theo suất chiếu
    strcpy(message, "SHOW_SEATS\r\n");
    strcat(message, showtime_id);
    strcat(message, "\r\n");
}

void makeBookTicketMessage(char *showtime_id, char *seat_id, char *message) {
    // Client gửi yêu cầu đặt vé
    strcpy(message, "BOOK_TICKET\r\n");
    strcat(message, showtime_id);
    strcat(message, "\r\n");
    strcat(message, seat_id);
    strcat(message, "\r\n");
}

/*----------END BOOK TICKET--------*/

// Manager control

void makeAnnounceFilmMessage(char *film_id, char *cinema_id, char *premiered_time_id, char *date, char *message) {
    strcpy(message, "POST_FILM\r\n");
    strcat(message, film_id);
    strcat(message, "\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
    strcat(message, premiered_time_id);
    strcat(message, "\r\n");
    strcat(message, date);
    strcat(message, "\r\n");
} 

/*----------VIEW TICKET--------*/

void makeViewTicketsMessage(char *message) {
    strcpy(message, "VIEW_TICKETS\r\n");
}
void makeViewTicketDetailMessage(char *ticket_id, char *message) {
    strcpy(message, "VIEW_TICKET_DETAIL\r\n");
    strcat(message, ticket_id);
    strcat(message, "\r\n");
}
/*----------END VIEW TICKET--------*/

/*----------ADD SHOW TIME--------*/

void makeShowRoomsByCinemaMessage(char *cinema_id, char *message) {
    strcpy(message, "SHOW_ROOMS_BY_CINEMA\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
}

void makeShowShowtimesByRoomMessage(char *room_id, char *message) {
    strcpy(message, "SHOW_SHOWTIMES_BY_ROOM\r\n");
    strcat(message, room_id);
    strcat(message, "\r\n");
}

void makeAddShowtimeMessage(char *film_id, char *cinema_id, char *room_id, char *start_datetime, char *message) {
    strcpy(message, "ADD_SHOWTIME\r\n");
    strcat(message, film_id);
    strcat(message, "\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
    strcat(message, room_id);
    strcat(message, "\r\n");
    strcat(message, start_datetime);
    strcat(message, "\r\n");
}

/*----------END ADD SHOW TIME--------*/

/*----------ADMIN--------*/
void makeShowAllUsersMessage(char *message){
    strcpy(message, "SHOW_ALL_USERS\r\n");
}

void makeDeleteUserMessage(char *user_id, char *message){
    strcpy(message, "DELETE_USER\r\n");
    strcat(message, user_id);
    strcat(message, "\r\n");
}

void makeChangeUserRoleMessage(char *user_id, char *new_role, char *message){
    strcpy(message, "CHANGE_USER_ROLE\r\n");
    strcat(message, user_id);
    strcat(message, "\r\n");
    strcat(message, new_role);
    strcat(message, "\r\n");
}

/*----------END ADMIN--------*/