#include "data.h"
#include "cache.h"
#include <stdlib.h>

int handle_show_seats(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    ShowtimeCache *st = findShowtime(atoi(fields.value[0]));
    if (!st) return INVALID_SHOWTIME;
    ResponseBody body = {resp_buf, buf_size, 0};
    int code = NO_SEATS_AVAILABLE;
    pthread_mutex_lock(&st->lock);
    for (int i = 0; i < st->seat_count; i++) {
        Seat *s = &st->seats[i];
        if (s->is_booked) continue;
        if (!append_line(&body, "SEAT|%d|%s-%s", s->seat_id, s->row_name, s->seat_number)) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
        code = VIEW_CHAIR_SUCCESS;
    }
    pthread_mutex_unlock(&st->lock);
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
