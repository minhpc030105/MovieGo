#include "data.h"
#include <stdlib.h>
#include <string.h>

int handle_show_users(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    StmtRows *rows = stmt_select(session->db, "SELECT id,name,username,role_id FROM users ORDER BY id", NULL);
    if (!rows) return SHOW_ALL_USERS_FAIL;

    ResponseBody body = {resp_buf, buf_size, 0};
    int code = SHOW_ALL_USERS_SUCCESS;
    const char *roles[] = {"Admin", "Manager", "User"};

    for (int i = 0; i < rows->count; i++) {
        char **row = rows->data[i];
        int role = atoi(row[3]);
        if (!append_line(&body, "%s|%s|%s|%s", row[0], row[1], row[2],
                         role >= 0 && role <= 2 ? roles[role] : "Unknown")) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
    }
    free_stmt_rows(rows);
    return code;
}

int handle_delete_user(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    unsigned long long target_id = strtoull(fields.value[0], NULL, 10);
    if (target_id == session->user_id) return DELETE_USER_FAIL;

    int has_records = 0;
    if (stmt_get_int(session->db, &has_records,
        "SELECT (EXISTS(SELECT 1 FROM tickets WHERE user_id=?) "
        "OR EXISTS(SELECT 1 FROM bookings WHERE user_id=?))",
        "uu", target_id, target_id) != 0 || has_records) {
        return DELETE_USER_FAIL;
    }

    int affected = stmt_execute(session->db, "DELETE FROM users WHERE id=?", "u", target_id);
    return affected == 1 ? DELETE_USER_SUCCESS : DELETE_USER_FAIL;
}

int handle_change_role(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "ir", &fields)) return INVALID_REQUEST;
    int affected = stmt_execute(session->db, "UPDATE users SET role_id=? WHERE id=?", "ss",
                                fields.value[1], fields.value[0]);
    return affected == 1 ? UPDATE_USER_ROLE_SUCCESS : UPDATE_USER_ROLE_FAIL;
}

int handle_add_film(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "sii", &fields)) return INVALID_REQUEST;
    const char *title = fields.value[0];
    const char *category_id = fields.value[1];
    const char *show_time = fields.value[2];

    int affected = stmt_execute(session->db,
        "INSERT INTO films (title,category_id,show_time) SELECT ?,id,? FROM categories "
        "WHERE id=? AND NOT EXISTS (SELECT 1 FROM films WHERE LOWER(title)=LOWER(?))",
        "ssss", title, show_time, category_id, title);
    return affected == 1 ? ADD_FILM_SUCCESS : ADD_FILM_FAIL;
}

int handle_cinema_rooms(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    const char *cinema = fields.value[0];

    int cinema_count = 0;
    if (stmt_get_int(session->db, &cinema_count, "SELECT COUNT(*) FROM cinemas WHERE id=?", "s", cinema) != 0 ||
        cinema_count == 0) {
        return INVALID_CINEMA;
    }

    StmtRows *rows = stmt_select(session->db,
        "SELECT id,name,total_seats FROM rooms WHERE cinema_id=? AND status='ACTIVE' ORDER BY id", "s", cinema);
    if (!rows) return FAILED_ROOMS;

    ResponseBody body = {resp_buf, buf_size, 0};
    int code = NO_ROOMS;
    for (int i = 0; i < rows->count; i++) {
        char **row = rows->data[i];
        if (!append_line(&body, "ID: %s | %s | %s seats", row[0], row[1], row[2])) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = VIEW_CHAIR_SUCCESS;
    }
    free_stmt_rows(rows);
    return code;
}

int handle_room_showtimes(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;

    StmtRows *rows = stmt_select(session->db,
        "SELECT s.id,f.title,s.start_time,s.end_time,TIMESTAMPDIFF(MINUTE,s.start_time,s.end_time) "
        "FROM showtimes s JOIN films f ON s.film_id=f.id WHERE s.room_id=? AND DATE(s.start_time)>=CURDATE() "
        "ORDER BY s.start_time", "s", fields.value[0]);
    if (!rows) return SHOW_TIME_FAIL;

    ResponseBody body = {resp_buf, buf_size, 0};
    int code = SHOW_TIME_FAIL;
    for (int i = 0; i < rows->count; i++) {
        char **row = rows->data[i];
        if (!append_line(&body, "ID: %s | Title: %s | %s - %s (%s minutes)",
                         row[0], row[1], row[2], row[3], row[4])) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = SHOW_SHOWTIME_SUCCESS;
    }
    free_stmt_rows(rows);
    return code;
}

int handle_add_showtime(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "iiis", &fields)) return INVALID_REQUEST;
    const char *film = fields.value[0], *cinema = fields.value[1], *room = fields.value[2], *start = fields.value[3];

    int minutes = 0;
    if (stmt_get_int(session->db, &minutes, "SELECT show_time FROM films WHERE id=?", "s", film) != 0 || minutes <= 0) {
        return INVALID_FILM_ID;
    }

    int cinema_count = 0;
    if (stmt_get_int(session->db, &cinema_count, "SELECT COUNT(*) FROM cinemas WHERE id=?", "s", cinema) != 0 || cinema_count == 0) {
        return INVALID_CINEMA_ID;
    }

    int room_cinema_id = 0;
    if (stmt_get_int(session->db, &room_cinema_id, "SELECT cinema_id FROM rooms WHERE id=? AND status='ACTIVE'", "s", room) != 0) {
        return INVALID_ROOM_ID;
    }
    if (room_cinema_id != atoi(cinema)) {
        return ROOM_NOT_BELONG_CINEMA;
    }

    char end[32] = {0};
    if (stmt_get_string(session->db, end, sizeof(end), "SELECT DATE_ADD(?,INTERVAL ? MINUTE)", "si", start, minutes) != 0 || !end[0]) {
        return INVALID_DATETIME_FORMAT;
    }

    int no_conflict = 0;
    if (stmt_get_int(session->db, &no_conflict,
        "SELECT NOT EXISTS (SELECT 1 FROM showtimes WHERE room_id=? "
        "AND end_time>DATE_SUB(?,INTERVAL 15 MINUTE) AND start_time<DATE_ADD(?,INTERVAL 15 MINUTE))",
        "sss", room, start, end) != 0 || !no_conflict) {
        return SCHEDULING_CONFLICT;
    }

    int affected = stmt_execute(session->db,
        "INSERT INTO showtimes(film_id,room_id,start_time,end_time) VALUES(?,?,?,?)",
        "ssss", film, room, start, end);
    return affected == 1 ? ADD_SHOWTIME_SUCCESS : ADD_SHOWTIME_FAIL;
}
