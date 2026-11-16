#include <stdio.h>
#include <string.h>

char* getTypeMessage(char *message){
    return strtok(message, "\r\n");
}

void makeLoginMessage(char *username, char *password, char *message){
    strcpy(message, "LOGIN\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, password);
    strcat(message, "\r\n");
}

void resolveLoginMessage(char *username, char *password){
    char *tmp_username = strtok(NULL, "\r\n");
    char *tmp_password = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
    if(tmp_password) strcpy(password, tmp_password);
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

void resolveRegisterMessage(char *name, char *username, char *password){
    char *tmp_name = strtok(NULL, "\r\n");
    char *tmp_username = strtok(NULL, "\r\n");
    char *tmp_password = strtok(NULL, "\r\n");
    if(tmp_name) strcpy(name, tmp_name);
    if(tmp_username) strcpy(username, tmp_username);
    if(tmp_password) strcpy(password, tmp_password);
}

void makeChangePasswordMessage(char *username, char *oldPassword, char *newPassword, char *message) {
    strcpy(message, "CHANGE_PASSWORD\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
    strcat(message, oldPassword);
    strcat(message, "\r\n");
    strcat(message, newPassword);
    strcat(message, "\r\n");
}

void resolveChangePasswordMessage(char *username, char *oldPassword, char *newPassword) {
    char *tmp_username = strtok(NULL, "\r\n");
    char *tmp_oldPassword = strtok(NULL, "\r\n");
    char *tmp_newPassword = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
    if(tmp_oldPassword) strcpy(oldPassword, tmp_oldPassword);
    if(tmp_newPassword) strcpy(newPassword, tmp_newPassword);
}

void makeLogoutMessage(char *message){
    strcpy(message, "LOGOUT\r\n");
}

void resolveLogoutMessage(char *username) {
    char *tmp_username = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
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

void resolveAddFilmMessage(
    char *title,
    char *category_id,
    char *show_time
){
    char *tmp_title = strtok(NULL, "\r\n");
    char *tmp_category_id = strtok(NULL, "\r\n");
    char *tmp_show_time = strtok(NULL, "\r\n");
    if(tmp_title) strcpy(title, tmp_title);
    if(tmp_category_id) strcpy(category_id, tmp_category_id);
    if(tmp_show_time) strcpy(show_time, tmp_show_time);
}
/*----------END ADD FILM--------*/

/*----------FIND FILM--------*/

void makeSearchFilmByTitleMessage(char *title, char *message){
    strcpy(message, "SEARCH_FILM_BY_TITLE\r\n");
    strcat(message, title);
    strcat(message, "\r\n");
}
void resolveSearchFilmByTitleMessage(char *title) {
    char *tmp_title = strtok(NULL, "\r\n");
    if(tmp_title) {
        strcpy(title, tmp_title);
    } else {
        title[0] = '\0';
    }
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

void resolveBrowseCategoryMessage(char *category_id) {
    char *tmp_category_id = strtok(NULL, "\r\n");
    if(tmp_category_id) strncpy(category_id, tmp_category_id, 255);
    else category_id[0] = '\0';
}

void resolveBrowseCinemaMessage(char *cinema_id) {
    char *tmp_cinema_id = strtok(NULL, "\r\n"); 
    if(tmp_cinema_id) strncpy(cinema_id, tmp_cinema_id, 255);
    else cinema_id[0] = '\0';
}

void resolveBrowsePremieredTimeMessage(char *time_slot) {
    char *tmp_time_slot = strtok(NULL, "\r\n"); 
    if(tmp_time_slot) strncpy(time_slot, tmp_time_slot, 255);
    else time_slot[0] = '\0';
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

void resolveShowCinemaByFilmMessage(char *film_id) {
    char *tmp_film_id = strtok(NULL, "\r\n");
    if (tmp_film_id) strcpy(film_id, tmp_film_id);
}
void makeShowTimeByFilmCinemaMessage(char *film_id, char *cinema_id, char *message) {
    // Client yêu cầu suất chiếu theo film + cinema
    
    strcpy(message, "SHOW_TIME_BY_FILM_CINEMA\r\n");
    strcat(message, film_id);
    strcat(message, "\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
}

void resolveShowTimeByFilmCinemaMessage(char *film_id, char *cinema_id) {
    char *tmp1 = strtok(NULL, "\r\n");
    char *tmp2 = strtok(NULL, "\r\n");
    if (tmp1) strcpy(film_id, tmp1);
    if (tmp2) strcpy(cinema_id, tmp2);
}
void makeShowSeatMessage(char *showtime_id, char *message) {
    // Client yêu cầu danh sách ghế theo suất chiếu
    strcpy(message, "SHOW_SEATS\r\n");
    strcat(message, showtime_id);
    strcat(message, "\r\n");
}

void resolveShowSeatMessage(char *showtime_id) {
    char *tmp_showtime_id = strtok(NULL, "\r\n");
    if (tmp_showtime_id) strcpy(showtime_id, tmp_showtime_id);
}
void makeBookTicketMessage(char *showtime_id, char *seat_id, char *message) {
    // Client gửi yêu cầu đặt vé
    strcpy(message, "BOOK_TICKET\r\n");
    strcat(message, showtime_id);
    strcat(message, "\r\n");
    strcat(message, seat_id);
    strcat(message, "\r\n");
}

void resolveBookTicketMessage(char *showtime_id, char *seat_id) {
    char *tmp_showtime = strtok(NULL, "\r\n");
    char *tmp_seat = strtok(NULL, "\r\n");
    if (tmp_showtime) strcpy(showtime_id, tmp_showtime);
    if (tmp_seat) strcpy(seat_id, tmp_seat);
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

void resolveAnnounceFilmMessage(char *film_id, char *cinema_id, char *premiered_time_id, char *date) {
    char *tmp_film_id = strtok(NULL, "\r\n");
    char *tmp_cinema_id = strtok(NULL, "\r\n");
    char *tmp_premiered_time_id = strtok(NULL, "\r\n");
    char *tmp_date = strtok(NULL, "\r\n");

    if(tmp_film_id) strcpy(film_id, tmp_film_id);
    if(tmp_cinema_id) strcpy(cinema_id, tmp_cinema_id);
    if(tmp_premiered_time_id) strcpy(premiered_time_id, tmp_premiered_time_id);
    if(tmp_date) strcpy(date, tmp_date);
}

/*----------VIEW TICKET--------*/

void makeViewTicketsMessage(char *username, char *message) {
    strcpy(message, "VIEW_TICKETS\r\n");
    strcat(message, username);
    strcat(message, "\r\n");
}
void resolveViewTicketsMessage(char *username) {
    char *tmp_username = strtok(NULL, "\r\n");
    if(tmp_username) strcpy(username, tmp_username);
}

void makeViewTicketDetailMessage(char *ticket_id, char *message) {
    strcpy(message, "VIEW_TICKET_DETAIL\r\n");
    strcat(message, ticket_id);
    strcat(message, "\r\n");
}
void resolveViewTicketDetailMessage(char *ticket_id) {
    char *tmp_ticket_id = strtok(NULL, "\r\n");
    if(tmp_ticket_id) strcpy(ticket_id, tmp_ticket_id);
}
/*----------END VIEW TICKET--------*/

/*----------ADD SHOW TIME--------*/

void makeShowRoomsByCinemaMessage(char *cinema_id, char *message) {
    strcpy(message, "SHOW_ROOMS_BY_CINEMA\r\n");
    strcat(message, cinema_id);
    strcat(message, "\r\n");
}

void resolveShowRoomsByCinemaMessage(char *cinema_id) {
    char *tmp_cinema_id = strtok(NULL, "\r\n");
    if (tmp_cinema_id) strcpy(cinema_id, tmp_cinema_id);
}

void makeShowShowtimesByRoomMessage(char *room_id, char *message) {
    strcpy(message, "SHOW_SHOWTIMES_BY_ROOM\r\n");
    strcat(message, room_id);
    strcat(message, "\r\n");
}

void resolveShowShowtimesByRoomMessage(char *room_id) {
    char *tmp_room_id = strtok(NULL, "\r\n");
    if (tmp_room_id) strcpy(room_id, tmp_room_id);
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

void resolveAddShowTimeMessage(char *film_id, char *cinema_id, char *room_id, char *start_datetime) {
    char *tmp_film_id = strtok(NULL, "\r\n");
    char *tmp_cinema_id = strtok(NULL, "\r\n");
    char *tmp_room_id = strtok(NULL, "\r\n");
    char *tmp_start_datetime = strtok(NULL, "\r\n");

    if (tmp_film_id) strcpy(film_id, tmp_film_id);
    if (tmp_cinema_id) strcpy(cinema_id, tmp_cinema_id);
    if (tmp_room_id) strcpy(room_id, tmp_room_id);
    if (tmp_start_datetime) strcpy(start_datetime, tmp_start_datetime);
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

void resolveDeleteUserMessage(char *user_id){
    char *tmp_user_id = strtok(NULL, "\r\n");
    if(tmp_user_id) strcpy(user_id, tmp_user_id);
}

void makeChangeUserRoleMessage(char *user_id, char *new_role, char *message){
    strcpy(message, "CHANGE_USER_ROLE\r\n");
    strcat(message, user_id);
    strcat(message, "\r\n");
    strcat(message, new_role);
    strcat(message, "\r\n");
}

void resolveChangeUserRoleMessage(char *user_id, char *new_role){
    char *tmp_user_id = strtok(NULL, "\r\n");
    char *tmp_new_role = strtok(NULL, "\r\n");
    if(tmp_user_id) strcpy(user_id, tmp_user_id);
    if(tmp_new_role) strcpy(new_role, tmp_new_role);
}
/*----------END ADMIN--------*/