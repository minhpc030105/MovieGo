#include "data.h"
#include <stdlib.h>
#include <string.h>

int handle_show_users(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db, "SELECT id,name,username,role_id FROM users ORDER BY id");
    if (!rows) return SHOW_ALL_USERS_FAIL;
    ResponseBody body = {resp_buf, buf_size, 0};
    MYSQL_ROW row;
    int code = SHOW_ALL_USERS_SUCCESS;
    while ((row = mysql_fetch_row(rows))) {
        const char *roles[] = {"Admin", "Manager", "User"};
        int role = atoi(row[3]);
        if (!append_line(&body, "%s|%s|%s|%s", row[0], row[1], row[2],
                         role >= 0 && role <= 2 ? roles[role] : "Unknown")) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
    }
    mysql_free_result(rows);
    return code;
}

int handle_delete_user(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    bool ok = execute_update(session->db, "DELETE FROM users WHERE id=%s", fields.value[0]);
    return ok && mysql_affected_rows(session->db) == 1 ? DELETE_USER_SUCCESS : DELETE_USER_FAIL;
}

int handle_change_role(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "ir", &fields)) return INVALID_REQUEST;
    bool ok = execute_update(session->db, "UPDATE users SET role_id=%s WHERE id=%s",
                             fields.value[1], fields.value[0]);
    return ok && mysql_affected_rows(session->db) == 1 ? UPDATE_USER_ROLE_SUCCESS : UPDATE_USER_ROLE_FAIL;
}

int handle_add_film(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "sii", &fields)) return INVALID_REQUEST;
    EscapedField title;
    if (!escape_field(session->db, fields.value[0], &title)) return INVALID_REQUEST;
    bool ok = execute_update(session->db,
        "INSERT INTO films (title,category_id,show_time) SELECT '%s',id,%s FROM categories "
        "WHERE id=%s AND NOT EXISTS (SELECT 1 FROM films WHERE LOWER(title)=LOWER('%s'))",
        title.text, fields.value[2], fields.value[1], title.text);
    return ok && mysql_affected_rows(session->db) == 1 ? ADD_FILM_SUCCESS : ADD_FILM_FAIL;
}

// Check a single COUNT/EXISTS result and preserve distinct protocol errors.
static int require_match(MYSQL_RES *rows, int missing_code) {
    if (!rows) return DB_QUERY_ERROR;
    MYSQL_ROW row = mysql_fetch_row(rows);
    bool found = row && atoi(row[0]) > 0;
    mysql_free_result(rows);
    return found ? 0 : missing_code;
}

int handle_cinema_rooms(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    const char *cinema = fields.value[0];
    int error = require_match(select_rows(session->db, "SELECT COUNT(*) FROM cinemas WHERE id=%s", cinema),
                              INVALID_CINEMA);
    if (error) return error;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT id,name,total_seats FROM rooms WHERE cinema_id=%s AND status='ACTIVE' ORDER BY id", cinema);
    if (!rows) return FAILED_ROOMS;
    ResponseBody body = {resp_buf, buf_size, 0};
    MYSQL_ROW row;
    int code = NO_ROOMS;
    while ((row = mysql_fetch_row(rows))) {
        if (!append_line(&body, "ID: %s | %s | %s seats", row[0], row[1], row[2])) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = VIEW_CHAIR_SUCCESS;
    }
    mysql_free_result(rows);
    return code;
}

int handle_room_showtimes(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT s.id,f.title,s.start_time,s.end_time,TIMESTAMPDIFF(MINUTE,s.start_time,s.end_time) "
        "FROM showtimes s JOIN films f ON s.film_id=f.id WHERE s.room_id=%s AND DATE(s.start_time)>=CURDATE() "
        "ORDER BY s.start_time", fields.value[0]);
    if (!rows) return SHOW_TIME_FAIL;
    ResponseBody body = {resp_buf, buf_size, 0};
    MYSQL_ROW row;
    int code = SHOW_TIME_FAIL;
    while ((row = mysql_fetch_row(rows))) {
        if (!append_line(&body, "ID: %s | Title: %s | %s - %s (%s minutes)",
                         row[0], row[1], row[2], row[3], row[4])) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = SHOW_SHOWTIME_SUCCESS;
    }
    mysql_free_result(rows);
    return code;
}

int handle_add_showtime(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "iiis", &fields)) return INVALID_REQUEST;
    const char *film = fields.value[0], *cinema = fields.value[1], *room = fields.value[2];
    EscapedField start;
    if (!escape_field(session->db, fields.value[3], &start)) return INVALID_DATETIME_FORMAT;
    MYSQL_RES *rows = select_rows(session->db, "SELECT show_time FROM films WHERE id=%s", film);
    if (!rows) return DB_QUERY_ERROR;
    MYSQL_ROW row = mysql_fetch_row(rows);
    int minutes = row ? atoi(row[0]) : 0;
    mysql_free_result(rows);
    if (!minutes) return INVALID_FILM_ID;
    int error = require_match(select_rows(session->db, "SELECT COUNT(*) FROM cinemas WHERE id=%s", cinema),
                              INVALID_CINEMA_ID);
    if (error) return error;
    rows = select_rows(session->db, "SELECT cinema_id FROM rooms WHERE id=%s AND status='ACTIVE'", room);
    if (!rows) return DB_QUERY_ERROR;
    row = mysql_fetch_row(rows);
    if (!row) error = INVALID_ROOM_ID;
    else if (atoi(row[0]) != atoi(cinema)) error = ROOM_NOT_BELONG_CINEMA;
    mysql_free_result(rows);
    if (error) return error;
    rows = select_rows(session->db, "SELECT DATE_ADD('%s',INTERVAL %d MINUTE)", start.text, minutes);
    if (!rows) return INVALID_DATETIME_FORMAT;
    row = mysql_fetch_row(rows);
    char end[32];
    if (!row || !row[0] || strlen(row[0]) >= sizeof(end)) error = INVALID_DATETIME_FORMAT;
    else strcpy(end, row[0]);
    mysql_free_result(rows);
    if (error) return error;
    rows = select_rows(session->db,
        "SELECT NOT EXISTS (SELECT 1 FROM showtimes WHERE room_id=%s "
        "AND end_time>DATE_SUB('%s',INTERVAL 15 MINUTE) AND start_time<DATE_ADD('%s',INTERVAL 15 MINUTE))",
        room, start.text, end);
    error = require_match(rows, SCHEDULING_CONFLICT);
    if (error) return error;
    bool ok = execute_update(session->db,
        "INSERT INTO showtimes(film_id,room_id,start_time,end_time) VALUES(%s,%s,'%s','%s')",
        film, room, start.text, end);
    return ok ? ADD_SHOWTIME_SUCCESS : ADD_SHOWTIME_FAIL;
}
