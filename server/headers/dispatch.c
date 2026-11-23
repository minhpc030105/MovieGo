#include "function.h"
#include "db_pool.h"
#include <stdio.h>
#include <string.h>

enum {
    ADMIN = 1 << 0, MANAGER = 1 << 1, CUSTOMER = 1 << 2, GUEST = 1 << 3,
    BROWSE = CUSTOMER | MANAGER, SIGNED_IN = ADMIN | MANAGER | CUSTOMER,
    EVERYONE = SIGNED_IN | GUEST
};

static const struct {
    const char *name;
    unsigned roles;
    Handler *handler;
} command_table[] = {
    {"LOGIN", GUEST, handle_login},
    {"REGISTER", GUEST, handle_register},
    {"LOGOUT", SIGNED_IN, handle_logout},
    {"EXIT", EVERYONE, handle_exit},
    {"CHANGE_PASSWORD", SIGNED_IN, handle_change_password},
    {"SHOW_CATEGORIES", BROWSE, handle_show_categories},
    {"BROWSE_CATEGORY", BROWSE, handle_browse_category},
    {"SHOW_CINEMAS", BROWSE, handle_show_cinemas},
    {"BROWSE_CINEMA", BROWSE, handle_browse_cinema},
    {"SHOW_PREMIERED_TIME", BROWSE, handle_show_times},
    {"BROWSE_PREMIERED_TIME", BROWSE, handle_browse_time},
    {"SHOW_FILMS", BROWSE, handle_show_films},
    {"SHOW_CINEMA_BY_FILM", CUSTOMER, handle_film_cinemas},
    {"SHOW_TIME_BY_FILM_CINEMA", CUSTOMER, handle_film_showtimes},
    {"SHOW_SEATS", CUSTOMER, handle_show_seats},
    {"BOOK_TICKET", CUSTOMER, handle_book_ticket},
    {"SEARCH_FILM_BY_TITLE", BROWSE, handle_search_films},
    {"ADD_FILM", MANAGER, handle_add_film},
    {"VIEW_TICKETS", CUSTOMER, handle_view_tickets},
    {"VIEW_TICKET_DETAIL", CUSTOMER, handle_ticket_detail},
    {"SHOW_ROOMS_BY_CINEMA", MANAGER, handle_cinema_rooms},
    {"SHOW_SHOWTIMES_BY_ROOM", MANAGER, handle_room_showtimes},
    {"ADD_SHOWTIME", MANAGER, handle_add_showtime},
    {"SHOW_ALL_USERS", ADMIN, handle_show_users},
    {"DELETE_USER", ADMIN, handle_delete_user},
    {"CHANGE_USER_ROLE", ADMIN, handle_change_role},
};

// Split only on whole CRLF pairs. A final CRLF is optional, empty fields are invalid.
static bool split_request(char *request, char **payload) {
    char *line = request;
    *payload = request + strlen(request);
    if (!*request) return false;
    while (*line) {
        char *end = line + strcspn(line, "\r\n");
        if (!*end) break;
        if (end == line || end[0] != '\r' || end[1] != '\n') return false;
        bool command_line = line == request;
        line = end + 2;
        if (command_line) {
            // Leave separators between arguments for read_fields.
            *payload = line;
            *end = '\0';
        }
        if (!*line) *end = '\0';
    }
    return true;
}

static int run_db_command(Session *session, unsigned roles, Handler *handler,
                          const char *payload, char *body, size_t size) {
    if (session->is_auth) {
        int error = refresh_role(session);
        if (error) return error;
        if (session->role_id < 0 || session->role_id > 2) return PERMISSION_DENIED;
        if (!(roles & (1u << session->role_id)))
            return handler == handle_login ? LOGIN_ALREADY : PERMISSION_DENIED;
    }
    return handler(session, payload, body, size);
}

static int route(Session *session, char *name, const char *payload, char *body, size_t size) {
    for (size_t i = 0; i < sizeof(command_table) / sizeof(command_table[0]); i++) {
        if (strcmp(name, command_table[i].name) != 0) continue;
        unsigned roles = command_table[i].roles;
        Handler *handler = command_table[i].handler;
        if (!session->is_auth && !(roles & GUEST)) return AUTH_REQUIRED;
        if (handler == handle_logout || handler == handle_exit)
            return handler(session, payload, body, size);

        session->db = db_pool_acquire(DB_ACQUIRE_TIMEOUT_MS);
        if (!session->db) return SERVER_BUSY;
        int code = run_db_command(session, roles, handler, payload, body, size);
        db_pool_release(session->db);
        session->db = NULL;
        return code;
    }
    return INVALID_REQUEST;
}

int dispatch_request(Session *session, char *request, char *response, size_t capacity) {
    if (capacity < 5) return RESPONSE_TOO_LARGE;
    response[4] = '\0';
    char *payload;
    int code = INVALID_REQUEST;
    if (split_request(request, &payload))
        code = route(session, request, payload, response + 4, capacity - 4);
    if (code >= 2000) response[4] = '\0';
    char prefix[5];
    int n = snprintf(prefix, sizeof(prefix), "%d", code);
    if (n != 4) {
        memcpy(response, "3018", 5);
        return INVALID_REQUEST;
    }
    memcpy(response, prefix, 4);
    return code;
}
