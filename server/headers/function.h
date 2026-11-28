#ifndef MOVIEGO_HANDLERS_H
#define MOVIEGO_HANDLERS_H
#include <stdbool.h>
#include <stddef.h>
#include <mysql/mysql.h>
#include "../../lib/resultCodes.h"

typedef struct Session {
    MYSQL *db;  // Borrowed only while dispatching a request. NULL between requests.
    unsigned long long user_id;
    int role_id;
    bool is_auth;
    bool closing;
    char username[256];
    struct Session *next_online;
} Session;

// Return a result code. Optional body lines go in resp_buf, prefixed with CRLF.
typedef int Handler(Session *session, const char *payload, char *resp_buf, size_t buf_size);
Handler handle_login;
Handler handle_register;
Handler handle_logout;
Handler handle_exit;
Handler handle_change_password;
Handler handle_show_categories;
Handler handle_browse_category;
Handler handle_show_cinemas;
Handler handle_browse_cinema;
Handler handle_show_times;
Handler handle_browse_time;
Handler handle_show_films;
Handler handle_film_cinemas;
Handler handle_film_showtimes;
Handler handle_show_seats;
Handler handle_book_ticket;
Handler handle_book_tickets;
Handler handle_pay_booking;
Handler handle_cancel_booking;
Handler handle_search_films;
Handler handle_add_film;
Handler handle_view_tickets;
Handler handle_ticket_detail;
Handler handle_cinema_rooms;
Handler handle_room_showtimes;
Handler handle_add_showtime;
Handler handle_show_users;
Handler handle_delete_user;
Handler handle_change_role;

void clear_session(Session *session);
int refresh_role(Session *session);
int dispatch_request(Session *session, char *request, char *response, size_t capacity);
#endif
