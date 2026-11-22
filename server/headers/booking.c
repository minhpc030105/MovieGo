#include "data.h"
#include "cache.h"
#include <stdlib.h>

static const char *SEATS_SQL =
    "SELECT s.id, s.row_name, s.seat_number, "
    "  CASE WHEN t.id IS NOT NULL THEN 'BOOKED' "
    "       WHEN h.seat_id IS NOT NULL THEN 'HELD' ELSE 'AVAILABLE' END "
    "FROM showtimes st JOIN seats s ON s.room_id = st.room_id "
    "LEFT JOIN tickets t ON t.showtime_id = st.id AND t.seat_id = s.id "
    "LEFT JOIN (SELECT bs.seat_id FROM booking_seats bs "
    "           JOIN bookings b ON b.id = bs.booking_id "
    "           WHERE b.showtime_id = ? AND b.status = 'PENDING' AND b.expires_at > NOW()) h "
    "  ON h.seat_id = s.id "
    "WHERE st.id = ? ORDER BY s.row_name, LENGTH(s.seat_number), s.seat_number";

int handle_show_seats(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    int showtime_id = atoi(fields.value[0]), exists = 0;
    if (stmt_get_int(session->db, &exists, "SELECT COUNT(*) FROM showtimes WHERE id = ?", "i", showtime_id) != 0)
        return CANT_LOAD_SEATS;
    if (!exists) return INVALID_SHOWTIME;

    StmtRows *rows = stmt_select(session->db, SEATS_SQL, "ii", showtime_id, showtime_id);
    if (!rows) return CANT_LOAD_SEATS;
    ResponseBody body = {resp_buf, buf_size, 0};
    int code = NO_SEATS_AVAILABLE;
    for (int i = 0; i < rows->count; i++) {
        if (strcmp(rows->data[i][3], "AVAILABLE") != 0) continue;
        if (!append_line(&body, "SEAT|%s|%s-%s", rows->data[i][0], rows->data[i][1], rows->data[i][2])) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = VIEW_CHAIR_SUCCESS;
    }
    free_stmt_rows(rows);
    return code;
}

int handle_book_ticket(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "ii", &fields)) return INVALID_REQUEST;
    ShowtimeCache *st = findShowtime(atoi(fields.value[0]));
    if (!st) return BOOK_TICKET_FAIL;
    pthread_mutex_lock(&st->lock);
    Seat *seat = findSeat(st, atoi(fields.value[1]));
    bool booked = seat && !seat->is_booked && execute_update(session->db,
        "INSERT INTO tickets(user_id,showtime_id,seat_id) VALUES(%llu,%d,%d)",
        session->user_id, st->showtime_id, seat->seat_id);
    if (booked) {
        seat->is_booked = 1;
        seat->user_id = session->user_id;
    }
    pthread_mutex_unlock(&st->lock);
    return booked ? BOOK_TICKET_SUCCESS : BOOK_TICKET_FAIL;
}

int handle_view_tickets(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT t.id,f.title,c.name,r.name,CONCAT(s.row_name,'-',s.seat_number),st.start_time,t.booked_at "
        "FROM tickets t JOIN showtimes st ON t.showtime_id=st.id JOIN films f ON st.film_id=f.id "
        "JOIN rooms r ON st.room_id=r.id JOIN cinemas c ON r.cinema_id=c.id JOIN seats s ON t.seat_id=s.id "
        "WHERE t.user_id=%llu ORDER BY st.start_time DESC", session->user_id);
    if (!rows) return FAILED_RETRIEVE;
    if (!mysql_num_rows(rows)) {
        mysql_free_result(rows);
        return NO_TICKETS;
    }
    ResponseBody body = {resp_buf, buf_size, 0};
    const char *format = "%-5s | %-20s | %-20s | %-10s | %-5s | %-20s | %-20s";
    bool ok = append_line(&body, format, "ID", "Film", "Cinema", "Room", "Seat", "Start Time", "Booked At");
    MYSQL_ROW row;
    while (ok && (row = mysql_fetch_row(rows)))
        ok = append_line(&body, format, row[0], row[1], row[2], row[3], row[4], row[5], row[6]);
    if (ok) ok = append_line(&body, "You has total: %llu ticket(s)", (unsigned long long)mysql_num_rows(rows));
    mysql_free_result(rows);
    return ok ? VIEW_CHAIR_SUCCESS : RESPONSE_TOO_LARGE;
}

int handle_ticket_detail(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT u.name,u.username,f.title,c.name,c.address,r.name,r.total_seats,CONCAT(s.row_name,s.seat_number),"
        "st.start_time,st.end_time,t.booked_at FROM tickets t JOIN users u ON t.user_id=u.id "
        "JOIN showtimes st ON t.showtime_id=st.id JOIN films f ON st.film_id=f.id JOIN rooms r ON st.room_id=r.id "
        "JOIN cinemas c ON r.cinema_id=c.id JOIN seats s ON t.seat_id=s.id WHERE t.id=%s AND t.user_id=%llu",
        fields.value[0], session->user_id);
    if (!rows) return VIEW_CHAIR_FAIL;
    MYSQL_ROW row = mysql_fetch_row(rows);
    int code = TICKET_NOT_FOUND;
    if (row) {
        ResponseBody body = {resp_buf, buf_size, 0};
        bool ok = append_line(&body, "=== TICKET DETAIL ===")
            && append_line(&body, "Customer: %s (%s)", row[0], row[1])
            && append_line(&body, "Film: %s", row[2])
            && append_line(&body, "Cinema: %s | Address: %s", row[3], row[4])
            && append_line(&body, "Room: %s | Capacity: %s seats |  Seat: %s", row[5], row[6], row[7])
            && append_line(&body, "Showtime: %s - %s", row[8], row[9])
            && append_line(&body, "Booked at: %s", row[10]);
        code = ok ? VIEW_CHAIR_SUCCESS : RESPONSE_TOO_LARGE;
    }
    mysql_free_result(rows);
    return code;
}
