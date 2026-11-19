#include "data.h"

typedef bool (*RowWriter)(ResponseBody *, MYSQL_ROW);

static bool id_name(ResponseBody *body, MYSQL_ROW row) {
    return append_line(body, "%s. %s", row[0], row[1]);
}

static bool time_range(ResponseBody *body, MYSQL_ROW row) {
    return append_line(body, "%s - %s", row[0], row[1]);
}

static bool showtime(ResponseBody *body, MYSQL_ROW row) {
    return append_line(body, "%s. %s - %s", row[0], row[1], row[2]);
}

static bool film_detail(ResponseBody *body, MYSQL_ROW row) {
    return append_line(body, "ID: %s | Title: %s | Category: %s | Show_time: %s minutes",
                       row[0], row[1], row[2], row[3]);
}

static int list_rows(MYSQL_RES *rows, RowWriter write_row, char *buffer, size_t size,
                     int success, int empty, int failure) {
    if (!rows) return failure;
    ResponseBody body = {buffer, size, 0};
    int code = empty;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(rows))) {
        if (!write_row(&body, row)) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = success;
    }
    mysql_free_result(rows);
    return code;
}

int handle_show_categories(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT id, name FROM categories");
    return list_rows(rows, id_name, resp_buf, buf_size,
                     SHOW_CATEGORY_SUCCESS, NO_CATEGORIES, CANT_LOAD_CATEGORIES);
}

int handle_show_cinemas(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT id, name FROM cinemas");
    return list_rows(rows, id_name, resp_buf, buf_size,
                     BROWSE_THEATER_SUCCESS, NO_CINEMAS, CANT_LOAD_CINEMAS);
}

int handle_show_times(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT DISTINCT TIME_FORMAT(start_time,'%%H:%%i') AS start, "
        "TIME_FORMAT(end_time,'%%H:%%i') AS end_time_slot FROM showtimes ORDER BY start");
    return list_rows(rows, time_range, resp_buf, buf_size,
                     BROWSE_TIME_SUCCESS, NO_SHOWTIMES, CANT_LOAD_SHOWTIMES);
}

int handle_show_films(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT id, title FROM films ORDER BY id");
    return list_rows(rows, id_name, resp_buf, buf_size,
                     FIND_FILM_SUCCESS, NO_FILMS, CANT_LOAD_FILMS);
}

int handle_film_cinemas(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT DISTINCT c.id, c.name FROM cinemas c JOIN rooms r ON r.cinema_id=c.id "
        "JOIN showtimes st ON st.room_id=r.id WHERE st.film_id=%s ORDER BY c.id", fields.value[0]);
    return list_rows(rows, id_name, resp_buf, buf_size,
                     BROWSE_THEATER_SUCCESS, NO_CINEMAS, CANT_LOAD_CINEMAS);
}

int handle_film_showtimes(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "ii", &fields)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT st.id, st.start_time, st.end_time FROM showtimes st JOIN rooms r ON st.room_id=r.id "
        "WHERE st.film_id=%s AND r.cinema_id=%s ORDER BY st.start_time", fields.value[0], fields.value[1]);
    return list_rows(rows, showtime, resp_buf, buf_size,
                     BROWSE_TIME_SUCCESS, NO_SHOWTIMES, CANT_LOAD_SHOWTIMES);
}

int handle_browse_category(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT f.id, f.title, c.name, f.show_time FROM films f JOIN categories c ON f.category_id=c.id "
        "WHERE f.category_id=%s", fields.value[0]);
    return list_rows(rows, film_detail, resp_buf, buf_size,
                     BROWSE_CATEGORY_SUCCESS, BROWSE_FAIL, BROWSE_FAIL);
}

int handle_browse_cinema(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT DISTINCT f.id, f.title, c.name, f.show_time FROM films f JOIN categories c ON f.category_id=c.id "
        "JOIN showtimes st ON st.film_id=f.id JOIN rooms r ON st.room_id=r.id "
        "WHERE r.cinema_id=%s", fields.value[0]);
    return list_rows(rows, film_detail, resp_buf, buf_size,
                     BROWSE_THEATER_SUCCESS, BROWSE_FAIL, BROWSE_FAIL);
}

int handle_browse_time(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "s", &fields)) return INVALID_REQUEST;
    EscapedField text;
    if (!escape_field(session->db, fields.value[0], &text)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT DISTINCT f.id, f.title, c.name, f.show_time FROM films f JOIN categories c ON f.category_id=c.id "
        "JOIN showtimes st ON st.film_id=f.id WHERE TIME_FORMAT(st.start_time,'%%H:%%i')='%s'", text.text);
    return list_rows(rows, film_detail, resp_buf, buf_size,
                     BROWSE_TIME_SUCCESS, BROWSE_FAIL, BROWSE_FAIL);
}

int handle_search_films(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "s", &fields)) return INVALID_REQUEST;
    EscapedField text;
    if (!escape_field(session->db, fields.value[0], &text)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT f.id, f.title, c.name, f.show_time FROM films f JOIN categories c ON f.category_id=c.id "
        "WHERE f.title LIKE '%%%s%%'", text.text);
    return list_rows(rows, film_detail, resp_buf, buf_size,
                     FIND_FILM_SUCCESS, FIND_FILM_FAIL, FIND_FILM_FAIL);
}
