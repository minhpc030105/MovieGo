#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"
#include "queryUser.h"
#include "function.h"
#include "cache.h"
#include "../../lib/resultCodes.h"

#define MSG_END "END"


// External mutex declarations
extern pthread_mutex_t arr_lock;
extern pthread_mutex_t head_lock;

void handleLogin(int connfd, listLoginedAccount *arr, node *h, char *username, char *password){
    pthread_mutex_lock(&arr_lock);
    pthread_mutex_lock(&head_lock);
    
    int check = checkLogin(*h, &username, password, arr);
    printf("Login check result: %d\n", check);

    if (check == -1) {
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_FAIL);
    }
    else if (check == 0) {
        addToListLoginedAccount(arr, &username);
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_SUCCESS_ADMIN);
    }
    else if (check == 1) {
        addToListLoginedAccount(arr, &username);
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_SUCCESS_MANAGER);
    }
    else if (check == 2) {
        addToListLoginedAccount(arr, &username);
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_SUCCESS_USER);
    }
    else if (check == 3) {
        pthread_mutex_unlock(&head_lock);
        pthread_mutex_unlock(&arr_lock);
        sendResult(connfd, LOGIN_ALREADY);
    }
}

void handleLogout(int connfd, listLoginedAccount *arr, char *username){
    printf("Logout user: %s\n", username);
    
    pthread_mutex_lock(&arr_lock);
    deleteFromListLoginedAccount(arr, &username);
    pthread_mutex_unlock(&arr_lock);
    
    sendResult(connfd, LOGOUT_SUCCESS);
}

void handleRegister(MYSQL *conn, int connfd, node *h, char *name, char *username, char *password){
    printf("[DEBUG] Register - name: '%s', username: '%s', password: '%s'\n", name, username, password);

    user newUser;
    strncpy(newUser.name, name, sizeof(newUser.name) - 1);
    newUser.name[sizeof(newUser.name) - 1] = '\0';
    
    strncpy(newUser.username, username, sizeof(newUser.username) - 1);
    newUser.username[sizeof(newUser.username) - 1] = '\0';
    
    strncpy(newUser.password, password, sizeof(newUser.password) - 1);
    newUser.password[sizeof(newUser.password) - 1] = '\0';
    
    newUser.role_id = 2;

    int result = registerUser(conn, newUser);

    if (result == 1) {
        pthread_mutex_lock(&head_lock);
        addNode(h, newUser);
        pthread_mutex_unlock(&head_lock);
        sendResult(connfd, REGISTER_SUCCESS);
    } else {
        sendResult(connfd, REGISTER_FAIL);
    }
}

void handleChangePassword( int connfd, MYSQL *conn, node *h){
    char username[255] = {0};
    char oldPassword[255] = {0};
    char newPassword[255] = {0};

    resolveChangePasswordMessage(username, oldPassword, newPassword);

    int result = changePassword(conn, username, oldPassword, newPassword);
    printf("Change password result: %d\n", result);

    if (result == 1) {
        pthread_mutex_lock(&head_lock);
        changeNodePassword(h, username, newPassword);
        pthread_mutex_unlock(&head_lock);
        sendResult(connfd, CHANGE_PASSWORD_SUCCESS);
    } else {
        
        sendResult(connfd, CHANGE_PASSWORD_FAIL);
    }
}

/*----------FIND FILM--------*/
void handleSearchFilmByTitle(MYSQL *conn, int connfd, char *title) {
    char message[1024] = {0};
    char query[512];
    
    snprintf(query, sizeof(query),
        "SELECT f.id, f.title, c.name AS category, f.show_time "
        "FROM films f JOIN categories c ON f.category_id = c.id "
        "WHERE f.title LIKE '%%%s%%'", title);

    if (mysql_query(conn, query) != 0) {
        sendResult(connfd, FIND_FILM_FAIL);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res || mysql_num_rows(res) == 0) {
        sendResult(connfd, FIND_FILM_FAIL);
        if (res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, FIND_FILM_SUCCESS);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        snprintf(message, sizeof(message),
            "ID: %s | Title: %s | Category: %s | Show_time: %s minutes",
            row[0], row[1], row[2], row[3]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}

/*----------END FIND FILM--------*/



/*----------BROWSE FILM----------*/


void handleShowCategory(MYSQL *conn, int connfd){
    char query[512];
    char message[1024];

    sprintf(query, "SELECT id, name FROM categories");

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, CANT_LOAD_CATEGORIES);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, NO_CATEGORIES);
        if(res) mysql_free_result(res);
        return;
    }
    
    sendResult(connfd, SHOW_CATEGORY_SUCCESS);
    
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "%s. %s", row[0], row[1]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END); 
    mysql_free_result(res);
}

void handleShowCinema(MYSQL *conn, int connfd){
    char query[512];
    char message[1024];

    sprintf(query, "SELECT id, name FROM cinemas");

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, CANT_LOAD_CINEMAS);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, NO_CINEMAS);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_THEATER_SUCCESS);

    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "%s. %s", row[0], row[1]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END); 
    mysql_free_result(res);
}

void handleShowPremieredTime(MYSQL *conn, int connfd){
    char query[512];
    char message[1024];

    // Lấy các khung giờ chiếu
    sprintf(query, 
        "SELECT DISTINCT "
        "TIME_FORMAT(start_time, '%%H:%%i') as start, "
        "TIME_FORMAT(end_time, '%%H:%%i') as end "
        "FROM showtimes "
        "ORDER BY start_time");

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, CANT_LOAD_SHOWTIMES);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, NO_SHOWTIMES);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_TIME_SUCCESS);
   
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "%s - %s", row[0], row[1]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);  
    mysql_free_result(res);
}

void handleBrowseCategory(MYSQL *conn, int connfd, char *category_id){
    char query[512];
    char message[1024];

    sprintf(query, "SELECT f.id, f.title, c.name AS category, f.show_time "
                   "FROM films f JOIN categories c ON f.category_id = c.id "
                   "WHERE f.category_id = %s", category_id);

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, BROWSE_FAIL);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, BROWSE_FAIL);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_CATEGORY_SUCCESS);

    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "ID: %s | Title: %s | Category: %s | Show_time: %s minutes",
                 row[0], row[1], row[2], row[3]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}


void handleBrowseCinema(MYSQL *conn, int connfd, char *cinema_id){
    char query[512];
    char message[1024];

    sprintf(query, 
        "SELECT DISTINCT f.id, f.title, c.name AS category, f.show_time "
        "FROM films f "
        "JOIN categories c ON f.category_id = c.id "
        "JOIN showtimes st ON st.film_id = f.id "
        "JOIN rooms r ON st.room_id = r.id "
        "WHERE r.cinema_id = %s", 
        cinema_id);

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, BROWSE_FAIL);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, BROWSE_FAIL);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_THEATER_SUCCESS);

    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "ID: %s | Title: %s | Category: %s | Show_time: %s minutes",
                 row[0], row[1], row[2], row[3]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}

void handleBrowseShowTime(MYSQL *conn, int connfd, char *time_slot){
    char query[512];
    char message[1024];

    sprintf(query, 
        "SELECT DISTINCT f.id, f.title, c.name AS category, f.show_time "
        "FROM films f "
        "JOIN categories c ON f.category_id = c.id "
        "JOIN showtimes st ON st.film_id = f.id "
        "WHERE TIME_FORMAT(st.start_time, '%%H:%%i') = '%s'", 
        time_slot);

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, BROWSE_FAIL);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, BROWSE_FAIL);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_TIME_SUCCESS);

    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "ID: %s | Title: %s | Category: %s | Show_time: %s minutes",
                 row[0], row[1], row[2], row[3]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}

/*----------END BROWSE FILM----------*/


/*----------BOOK TICKET--------*/
void handleShowFilm(MYSQL *conn, int connfd) {
    char query[512];
    char message[1024];

    sprintf(query, "SELECT id, title FROM films ORDER BY id");

    if(mysql_query(conn, query) != 0){
        sendResult(connfd, CANT_LOAD_FILMS);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, NO_FILMS);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, FIND_FILM_SUCCESS);

    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        snprintf(message, sizeof(message), "%s. %s", row[0], row[1]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}

void handleShowCinemaByFilm(MYSQL *conn, int connfd) {
    char film_id[32] = {0};
    char query[512];
    char message[1024];

    resolveShowCinemaByFilmMessage(film_id);

    sprintf(query,
        "SELECT DISTINCT c.id, c.name "
        "FROM cinemas c "
        "JOIN rooms r ON r.cinema_id = c.id "
        "JOIN showtimes st ON st.room_id = r.id "
        "WHERE st.film_id = %s "
        "ORDER BY c.id",
        film_id
    );

    if (mysql_query(conn, query) != 0) {
        sendResult(connfd, CANT_LOAD_CINEMAS);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res || mysql_num_rows(res) == 0) {
        sendResult(connfd, NO_CINEMAS);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_THEATER_SUCCESS);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        snprintf(message, sizeof(message), "%s. %s", row[0], row[1]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}

void handleShowTimeByFilmCinema(MYSQL *conn, int connfd) {
    char film_id[32] = {0};
    char cinema_id[32] = {0};
    char query[512];
    char message[1024];

    resolveShowTimeByFilmCinemaMessage(film_id, cinema_id);

    sprintf(query,
        "SELECT st.id, st.start_time, st.end_time "
        "FROM showtimes st "
        "JOIN rooms r ON st.room_id = r.id "
        "WHERE st.film_id = %s AND r.cinema_id = %s "
        "ORDER BY st.start_time",
        film_id, cinema_id
    );

    if (mysql_query(conn, query) != 0) {
        sendResult(connfd, CANT_LOAD_SHOWTIMES);
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res || mysql_num_rows(res) == 0) {
        sendResult(connfd, NO_SHOWTIMES);
        if(res) mysql_free_result(res);
        return;
    }

    sendResult(connfd, BROWSE_TIME_SUCCESS);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        snprintf(message, sizeof(message), "%s. %s - %s",
                 row[0], row[1], row[2]);
        sendMessage(connfd, message);
    }

    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}


void handleShowSeat(MYSQL *conn, int connfd) {
    char showtime_id_str[255] = {0};
    char message[1024];

    resolveShowSeatMessage(showtime_id_str);
    int showtime_id = atoi(showtime_id_str);


    ShowtimeCache *st = findShowtime(showtime_id);
    if (!st) {
        sendResult(connfd, INVALID_SHOWTIME);
        return;
    }

    pthread_mutex_lock(&st->lock);

    int available_count = 0;
    for (int i = 0; i < st->seat_count; i++) {
        Seat *s = &st->seats[i];
        if (!s->is_booked) {
            available_count++;
        }
    }
    
    if (available_count == 0) {
        pthread_mutex_unlock(&st->lock);
        sendResult(connfd, NO_SEATS_AVAILABLE);
        return;
    }

    sendResult(connfd, VIEW_CHAIR_SUCCESS);

    for (int i = 0; i < st->seat_count; i++) {
        Seat *s = &st->seats[i];
        if (!s->is_booked) {
            snprintf(message, sizeof(message), "SEAT|%d|%s-%s",
                    s->seat_id, s->row_name, s->seat_number);
            sendMessage(connfd, message);
        }
    }
    pthread_mutex_unlock(&st->lock);
    
    sendMessage(connfd, MSG_END);

}

void handleBookTicket(
    MYSQL *conn,
    int connfd,
    char *username,
    char *showtime_id_str,
    char *seat_id_str
) {
    int showtime_id = atoi(showtime_id_str);
    int seat_id = atoi(seat_id_str);
    int user_id = getUserIdByUsername(conn, username);

    ShowtimeCache *st = findShowtime(showtime_id);
    if (!st) {
        sendResult(connfd, BOOK_TICKET_FAIL);
        return;
    }

    pthread_mutex_lock(&st->lock);

    Seat *seat = findSeat(st, seat_id);
    if (!seat || seat->is_booked) {
        pthread_mutex_unlock(&st->lock);
        sendResult(connfd, BOOK_TICKET_FAIL);
        return;
    }

    seat->is_booked = 1;
    seat->user_id   = user_id;

    pthread_mutex_unlock(&st->lock);

    // Lưu vào db 
    char query[256];
    sprintf(query,
        "INSERT INTO tickets(user_id, showtime_id, seat_id) "
        "VALUES(%d, %d, %d)",
        user_id, showtime_id, seat_id);

    if (mysql_query(conn, query) == 0) {
        sendResult(connfd, BOOK_TICKET_SUCCESS);
    } else {
        pthread_mutex_lock(&st->lock);
        seat->is_booked = 0;
        seat->user_id = 0;
        pthread_mutex_unlock(&st->lock);
        
        sendResult(connfd, BOOK_TICKET_FAIL);
    }
}
/*----------END BOOK TICKET--------*/

void handleViewTickets(MYSQL *conn, int connfd, char *username) {
    char query[1024];
    char message[2048];

    sprintf(query, 
        "SELECT t.id, f.title, c.name, r.name, "
        "CONCAT(s.row_name, '-', s.seat_number) as seat, "
        "st.start_time, st.end_time, t.booked_at "
        "FROM tickets t "
        "JOIN users u ON t.user_id = u.id "
        "JOIN showtimes st ON t.showtime_id = st.id "
        "JOIN films f ON st.film_id = f.id "
        "JOIN rooms r ON st.room_id = r.id "
        "JOIN cinemas c ON r.cinema_id = c.id "
        "JOIN seats s ON t.seat_id = s.id "
        "WHERE u.username = '%s' "
        "ORDER BY st.start_time DESC", username);
    
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row;
        
        if (mysql_num_rows(result) == 0) {
            sendResult(connfd, NO_TICKETS);
            sendMessage(connfd, MSG_END);
            mysql_free_result(result);
            return;
        } else {
            sendResult(connfd, VIEW_CHAIR_SUCCESS);
            sprintf(message, "%-5s | %-20s | %-20s | %-10s | %-5s | %-20s | %-20s", 
                    "ID", "Film", "Cinema", "Room", "Seat", "Start Time", "Booked At");
            sendMessage(connfd, message);
            while ((row = mysql_fetch_row(result))) {
                sprintf(message, "%-5s | %-20s | %-20s | %-10s | %-5s | %-20s | %-20s", 
                        row[0], row[1], row[2], row[3], row[4], row[5], row[7]);
                sendMessage(connfd, message);
            }
            
            sprintf(message, "\nYou has total: %lu ticket(s)", mysql_num_rows(result));
            sendMessage(connfd, message);
            sendMessage(connfd, MSG_END);
        }
        mysql_free_result(result);
    } else {
        sendResult(connfd, FAILED_RETRIEVE);
        return;
    }
}
void handleViewTicketDetail(MYSQL *conn, int connfd, char *ticket_id) {

     char query[1024];
    char message[2048];
    
    sprintf(query,
            "SELECT u.name, u.username, f.title, c.name, c.address, "
            "r.name, r.total_seats, CONCAT(s.row_name, s.seat_number) as seat, "
            "st.start_time, st.end_time, t.booked_at, "
            "TIMESTAMPDIFF(HOUR, NOW(), st.start_time) as hours_until "
            "FROM tickets t "
            "JOIN users u ON t.user_id = u.id "
            "JOIN showtimes st ON t.showtime_id = st.id "
            "JOIN films f ON st.film_id = f.id "
            "JOIN rooms r ON st.room_id = r.id "
            "JOIN cinemas c ON r.cinema_id = c.id "
            "JOIN seats s ON t.seat_id = s.id "
            "WHERE t.id = %s",
            ticket_id);
    
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        
        if (mysql_num_rows(result) == 0) {
            sendResult(connfd, TICKET_NOT_FOUND);
            mysql_free_result(result);
            return;
        } else {
            sendResult(connfd, VIEW_CHAIR_SUCCESS);
            MYSQL_ROW row = mysql_fetch_row(result);
            
            sprintf(message, "\n=== TICKET DETAIL ===\n");
            sendMessage(connfd, message);
            
            sprintf(message, "Customer: %s (%s)", row[0], row[1]);
            sendMessage(connfd, message);
            
            sprintf(message, "\nFilm: %s", row[2]);
            sendMessage(connfd, message);
            
            sprintf(message, "\nCinema: %s | Address: %s", row[3], row[4]);
            sendMessage(connfd, message);
            
            sprintf(message, "\nRoom: %s | Capacity: %s seats |  Seat: %s", row[5], row[6], row[7]);
            sendMessage(connfd, message);
            
            sprintf(message, "\nShowtime: %s - %s", row[8], row[9]);
            sendMessage(connfd, message);
            
            sprintf(message, "\nBooked at: %s", row[10]);
            sendMessage(connfd, message);
            
            sendMessage(connfd, MSG_END);
        }
        mysql_free_result(result);
    } else {
        sendResult(connfd, VIEW_CHAIR_FAIL);
    }
}



/*----------MANAGER--------*/

/*-----ADD FILM-----*/
void handleAddFilm(MYSQL *conn, int connfd, char *title, char *category_id, char *show_time) {
    char query[2048];


    sprintf(query, 
        "SELECT COUNT(*) FROM films WHERE LOWER(title) = LOWER('%s')", title);
    
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && atoi(row[0]) > 0) {
            mysql_free_result(result);
            printf("[DEBUG] Film already exists\n");
            sendResult(connfd, ADD_FILM_FAIL);
            return;
        }
        mysql_free_result(result);
    }
    
    sprintf(query, 
        "SELECT COUNT(*) FROM categories WHERE id = %s", category_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && atoi(row[0]) == 0) {
            mysql_free_result(result);
            printf("[DEBUG] Category does not exist\n");
            sendResult(connfd, ADD_FILM_FAIL);
            return;
        }
        mysql_free_result(result);
    }

    // 3. Insert new film
    sprintf(query, 
        "INSERT INTO films (title, category_id, show_time) VALUES ('%s', %s, '%s')", 
        title, category_id, show_time);
    
    printf("[DEBUG] Executing query: %s\n", query);
    
    if (mysql_query(conn, query) == 0) {
        int film_id = mysql_insert_id(conn);
        printf("[LOG] Added film: %s (ID: %d)\n", title, film_id);
        sendResult(connfd, ADD_FILM_SUCCESS);
    } else {
        printf("[ERROR] Failed to add film: %s\n", mysql_error(conn));
        sendResult(connfd, ADD_FILM_FAIL);
    }
}

/*-----END ADD FILM-----*/

/*-----ADD SHOW TIME-----*/

void handleShowRoomsByCinema(MYSQL *conn, int connfd, char *cinema_id) {
    char query[512];
    char message[1024];

    // Check if cinema exists
    sprintf(query, "SELECT COUNT(*) FROM cinemas WHERE id = %s", cinema_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && atoi(row[0]) == 0) {
            mysql_free_result(result);
            sendResult(connfd, INVALID_CINEMA);
            return;
        }
        mysql_free_result(result);
    }

    // Show rooms in the cinema
    sprintf(query, 
        "SELECT r.id, r.name, r.total_seats "
        "FROM rooms r " 
        "WHERE r.cinema_id = %s AND r.status = 'ACTIVE' "
        "ORDER BY r.id",
        cinema_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
    
        if (mysql_num_rows(result) == 0) {
            sendResult(connfd, NO_ROOMS);
            mysql_free_result(result);
            return;
        } else {
            sendResult(connfd, VIEW_CHAIR_SUCCESS);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                sprintf(message, "ID: %s | %s | %s seats", 
                        row[0], row[1], row[2]);
                sendMessage(connfd, message);                
            }
            sendMessage(connfd, MSG_END);
        }
        mysql_free_result(result);
    }
    else {
        sendResult(connfd, FAILED_ROOMS);
        return;
    }
}

void handleShowShowtimesByRoom(MYSQL *conn, int connfd, char *room_id) {
    char query[512];
    char message[1024];

    sprintf(query,
        "SELECT s.id, f.title, s.start_time, s.end_time, "
        "TIMESTAMPDIFF(MINUTE, s.start_time, s.end_time) AS duration "
        "FROM showtimes s "
        "JOIN films f ON s.film_id = f.id "
        "WHERE s.room_id = %s "
        "AND DATE(s.start_time) >= CURDATE() "
        "ORDER BY s.start_time", room_id);

    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);

        if (mysql_num_rows(result) == 0) {
            sendResult(connfd, SHOW_TIME_FAIL);
        }
        else {
            sendResult(connfd, SHOW_SHOWTIME_SUCCESS);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                memset(message, 0, sizeof(message));
                sprintf(message, "ID: %s | Title: %s | %s - %s (%s minutes)", row[0], row[1], row[2], row[3], row[4]);
                sendMessage(connfd, message);
            }
            sendMessage(connfd, MSG_END);
        }
        mysql_free_result(result);
    }
}
void handleAddShowTime(MYSQL *conn, int connfd, char *film_id, char *cinema_id, char *room_id, char *start_datetime) {
    char query[1024];
    char response[1024];
    char end_datetime[50];
    int duration;

    printf("[DEBUG] ADD_SHOWTIME: film_id=%s, cinema_id=%s, room_id=%s, start_datetime=%s\n", 
           film_id, cinema_id, room_id, start_datetime);

    // 1. Check if film exists and get duration
    sprintf(query, "SELECT show_time FROM films WHERE id = %s", film_id);
    memset(response, 0, sizeof(response));
    printf("[DEBUG] Query: %s\n", query);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        if (mysql_num_rows(result) == 0) {
            mysql_free_result(result);
            printf("[ERROR] Invalid film ID\n");
            sendResult(connfd, INVALID_FILM_ID);
            return;
        }
        MYSQL_ROW row = mysql_fetch_row(result);
        duration = atoi(row[0]);
        printf("[DEBUG] Film duration: %d minutes\n", duration);
        mysql_free_result(result);
    }
    else {
        printf("[ERROR] Database query error: %s\n", mysql_error(conn));
        sendResult(connfd, DB_QUERY_ERROR);
        return;
    }

    // 2. Check if cinema exist
    sprintf(query, "SELECT COUNT(*) FROM cinemas WHERE id = %s", cinema_id);
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && atoi(row[0]) == 0) {
            mysql_free_result(result);
            printf("[ERROR] Invalid cinema ID\n");
            sendResult(connfd, INVALID_CINEMA_ID);
            return;
        }
        mysql_free_result(result);
    }
    
    // 3. Check if room exits, activate anđ belongs to selected cinema
    sprintf(query, "SELECT cinema_id FROM rooms WHERE id = %s AND status = 'ACTIVE'", room_id);
    int room_cinema_id = 0;
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        if (mysql_num_rows(result) == 0) {
            mysql_free_result(result);
            printf("[ERROR] Invalid room ID or room is not active\n");
            sendResult(connfd, INVALID_ROOM_ID);
            return;
        }
        MYSQL_ROW row = mysql_fetch_row(result);
        room_cinema_id = atoi(row[0]);
        mysql_free_result(result);

        if (room_cinema_id != atoi(cinema_id)) {
            printf("[ERROR] Room does not belong to the selected cinema\n");
            sendResult(connfd, ROOM_NOT_BELONG_CINEMA);
            return;
        }
    }

    // 4. Calculate end_datetime
    sprintf(query, "SELECT DATE_ADD('%s', INTERVAL %d MINUTE)", start_datetime, duration);

    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        strcpy(end_datetime, row[0]);
        mysql_free_result(result);
    } 
    else {
        printf("[ERROR] Invalid start datetime format\n");
        sendResult(connfd, INVALID_DATETIME_FORMAT);
        return;
    }

    sprintf(query, 
        "SELECT COUNT(*) FROM showtimes "
        "WHERE room_id = %s "
        "AND NOT (end_time <= DATE_SUB('%s', INTERVAL 15 MINUTE) OR start_time >= DATE_ADD('%s', INTERVAL 15 MINUTE))", 
        room_id, start_datetime, end_datetime);
    
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);    
        if (row && atoi(row[0]) > 0) {
            mysql_free_result(result);
            printf("[ERROR] Has scheduling conflict in the selected room\n");
            sendResult(connfd, SCHEDULING_CONFLICT);
            return;
        }
        mysql_free_result(result);
    }

    // 6. insert new showtime
    sprintf(query, "INSERT INTO showtimes (film_id, room_id, start_time, end_time) VALUES (%s, %s, '%s', '%s')",
            film_id, room_id, start_datetime, end_datetime);
    if (mysql_query(conn, query) == 0) {
        int showtime_id = mysql_insert_id(conn);
        sprintf(query, 
            "SELECT c.name, r.name FROM cinemas c "
            "JOIN rooms r ON r.cinema_id = c.id "
            "WHERE c.id = %s AND r.id = %s",
            cinema_id, room_id);
        char cinema_name[255] = "empty";
        char room_name[255] = "empty";
        if (mysql_query(conn, query) == 0) {
            MYSQL_RES *result = mysql_store_result(conn);
            if (mysql_num_rows(result) > 0) {
                MYSQL_ROW row = mysql_fetch_row(result);
                strcpy(cinema_name, row[0]);
                strcpy(room_name, row[1]);
            }
            mysql_free_result(result);
        }
        sendResult(connfd, ADD_SHOWTIME_SUCCESS);
        printf("[LOG] Added showtime (ID: %d) for film %s at %s - %s - %s\n", 
               showtime_id, film_id, cinema_name, room_name, start_datetime);
    }
    else {
        printf("[ERROR] Failed to add showtime: %s\n", mysql_error(conn));
        sendResult(connfd, ADD_SHOWTIME_FAIL);
    }
}

/*-----END ADD SHOW TIME-----*/


/*----------END MANAGER--------*/

/*----------ADMIN MANAGEMENT--------*/

void handleShowAllUsers(MYSQL *conn, int connfd) {
    char query[512];
    char message[1024];
    
    sprintf(query, "SELECT id, name, username, role_id FROM users ORDER BY id");
    
    if(mysql_query(conn, query) != 0){
        printf("[DEBUG] Query failed: %s\n", mysql_error(conn));
        sendResult(connfd, SHOW_ALL_USERS_FAIL);
        return;
    }
    
    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        printf("[DEBUG] No results or empty result set\n");
        sendResult(connfd, SHOW_ALL_USERS_FAIL);
        if(res) mysql_free_result(res);
        return;
    }
    
    int row_count = mysql_num_rows(res);
    printf("[DEBUG] Found %d users, sending result code %d\n", row_count, SHOW_ALL_USERS_SUCCESS);
    sendResult(connfd, SHOW_ALL_USERS_SUCCESS);
    
    MYSQL_ROW row;
    int i = 0;
    while((row = mysql_fetch_row(res))){
        const char* role_name;
        int role_id = atoi(row[3]);
        
        switch(role_id) {
            case 0: role_name = "Admin"; break;
            case 1: role_name = "Manager"; break;
            case 2: role_name = "User"; break;
            default: role_name = "Unknown";
        }
        
        snprintf(message, sizeof(message), "%s|%s|%s|%s", 
                 row[0], row[1], row[2], role_name);
        printf("[DEBUG] Sending user %d: %s\n", ++i, message);
        sendMessage(connfd, message);
    }
    
    printf("[DEBUG] Sending END marker\n");
    sendMessage(connfd, MSG_END);
    mysql_free_result(res);
}

void handleDeleteUser(MYSQL *conn, int connfd) {
    char user_id_str[256];
    resolveDeleteUserMessage(user_id_str);
    printf("[DEBUG] Delete user - user_id_str: %s\n", user_id_str);
    
    if(strlen(user_id_str) == 0) {
        printf("[DEBUG] Missing user_id parameter\n");
        sendResult(connfd, DELETE_USER_FAIL);
        return;
    }
    
    int user_id = atoi(user_id_str);
    char query[512];
    
    // Check if user exists
    sprintf(query, "SELECT id FROM users WHERE id = %d", user_id);
    if(mysql_query(conn, query) != 0){
        sendResult(connfd, DELETE_USER_FAIL);
        return;
    }
    
    MYSQL_RES *res = mysql_store_result(conn);
    if(!res || mysql_num_rows(res) == 0){
        sendResult(connfd, DELETE_USER_FAIL);
        if(res) mysql_free_result(res);
        return;
    }
    mysql_free_result(res);
    
    // Delete user
    sprintf(query, "DELETE FROM users WHERE id = %d", user_id);
    if(mysql_query(conn, query) == 0){
        sendResult(connfd, DELETE_USER_SUCCESS);
    } else {
        sendResult(connfd, DELETE_USER_FAIL);
    }
}

void handleChangeUserRole(MYSQL *conn, int connfd) {
    char user_id_str[256];
    char new_role_str[256];
    resolveChangeUserRoleMessage(user_id_str, new_role_str);
    
    printf("[DEBUG] Change role - user_id_str: %s, new_role_str: %s\n", 
           user_id_str, new_role_str);
    
    if(strlen(user_id_str) == 0 || strlen(new_role_str) == 0) {
        printf("[DEBUG] Missing parameters\n");
        sendResult(connfd, UPDATE_USER_ROLE_FAIL);
        return;
    }
    
    int user_id = atoi(user_id_str);
    int new_role = atoi(new_role_str);
    
    printf("[DEBUG] Parsed - user_id: %d, new_role: %d\n", user_id, new_role);
    
    // Validate role
    if(new_role < 0 || new_role > 2) {
        printf("[DEBUG] Invalid role: %d\n", new_role);
        sendResult(connfd, UPDATE_USER_ROLE_FAIL);
        return;
    }
    
    char query[512];
    sprintf(query, "UPDATE users SET role_id = %d WHERE id = %d", new_role, user_id);
    
    printf("[DEBUG] Executing query: %s\n", query);
    if(mysql_query(conn, query) == 0 && mysql_affected_rows(conn) > 0){
        printf("[DEBUG] Update successful, affected rows: %lu\n", mysql_affected_rows(conn));
        sendResult(connfd, UPDATE_USER_ROLE_SUCCESS);
    } else {
        printf("[DEBUG] Update failed: %s\n", mysql_error(conn));
        sendResult(connfd, UPDATE_USER_ROLE_FAIL);
    }
}
