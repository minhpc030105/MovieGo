#include "booking_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * SHOW_SEATS: every seat of the showtime room with its state.
 *   SEAT|seat_id|row-number|AVAILABLE   nobody holds or owns it
 *   SEAT|seat_id|row-number|HELD        an unexpired PENDING booking holds it
 *   SEAT|seat_id|row-number|BOOKED      a ticket exists
 * A hold whose expires_at has passed counts as AVAILABLE immediately; it does not
 * wait for the sweeper. Read only: no showtime lock, the dispatcher lends the
 * connection (session->db) for this short query, nothing is sent while it is held.
 * ------------------------------------------------------------------------- */
static const char *SEATS_SQL =
    "SELECT s.id, s.row_name, s.seat_number, "
    "  CASE WHEN t.id IS NOT NULL THEN 'BOOKED' "
    "       WHEN h.seat_id IS NOT NULL THEN 'HELD' ELSE 'AVAILABLE' END "
    "FROM showtimes st JOIN seats s ON s.room_id = st.room_id "
    "LEFT JOIN tickets t ON t.showtime_id = st.id AND t.seat_id = s.id "
    "LEFT JOIN (SELECT DISTINCT bs.seat_id FROM booking_seats bs "
    "           JOIN bookings b ON b.id = bs.booking_id "
    "           WHERE b.showtime_id = ? AND b.status = 'PENDING' AND b.expires_at > NOW()) h "
    "  ON h.seat_id = s.id "
    "WHERE st.id = ? ORDER BY s.row_name, LENGTH(s.seat_number), s.seat_number";

int handle_show_seats(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    unsigned long long showtime_id = strtoull(fields.value[0], NULL, 10);
    int exists = 0;
    if (stmt_get_int(session->db, &exists, "SELECT COUNT(*) FROM showtimes WHERE id = ?", "u", showtime_id) != 0)
        return CANT_LOAD_SEATS;
    if (!exists) return INVALID_SHOWTIME;

    StmtRows *rows = stmt_select(session->db, SEATS_SQL, "uu", showtime_id, showtime_id);
    if (!rows) return CANT_LOAD_SEATS;
    if (rows->count == 0) {
        free_stmt_rows(rows);
        return NO_SEATS;
    }
    ResponseBody body = {resp_buf, buf_size, 0};
    int code = VIEW_CHAIR_SUCCESS;
    for (int i = 0; i < rows->count; i++) {
        char **r = rows->data[i];
        if (!append_line(&body, "SEAT|%s|%s-%s|%s", r[0], r[1], r[2], r[3])) {
            code = RESPONSE_TOO_LARGE;
            break;
        }
    }
    free_stmt_rows(rows);
    return code;
}

/* ---------------------------------------------------------------------------
 * BOOK_TICKETS: hold 1..10 seats of one showtime for HOLD_MINUTES minutes.
 *   Request  BOOK_TICKETS CRLF showtime_id CRLF 12,13,14
 *   Success  1210 + one line  BOOKING|booking_id|PENDING|total|expires_at|12,13,14
 *   Errors   3037 bad seat list / seat not in the room, 3019 no such showtime,
 *            3032 a seat is taken (the WHOLE order is rejected), 3031 busy, 3022 database error.
 * total_amount is computed here from showtimes.price; the client never sends it.
 * Runs with the showtime lock held and with its OWN pooled connection (booking_common.h).
 * ------------------------------------------------------------------------- */
static int book_in_transaction(MYSQL *db, Session *session, unsigned long long showtime_id,
                               const unsigned long long *seats, int count,
                               char *resp_buf, size_t buf_size) {
    if (!tx_begin(db)) return DB_QUERY_ERROR;

    /* Row lock: also serialises other server processes (the in-process lock cannot). */
    StmtRows *locked = stmt_select(db, "SELECT id FROM showtimes WHERE id = ? FOR UPDATE", "u", showtime_id);
    if (!locked) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    int found = locked->count;
    free_stmt_rows(locked);
    if (!found) {
        tx_rollback(db);
        return INVALID_SHOWTIME;
    }

    /* All-or-nothing: if any seat is sold or held, reject the whole order. */
    for (int i = 0; i < count; i++) {
        int unavailable = seat_is_unavailable(db, showtime_id, seats[i]);
        if (unavailable < 0) {
            tx_rollback(db);
            return DB_QUERY_ERROR;
        }
        if (unavailable) {
            tx_rollback(db);
            return SEAT_TAKEN;
        }
    }

    int n = stmt_execute(db,
        "INSERT INTO bookings (user_id, showtime_id, total_amount, status, expires_at) "
        "SELECT ?, st.id, st.price * ?, 'PENDING', NOW() + INTERVAL " HOLD_MINUTES_STR " MINUTE "
        "FROM showtimes st WHERE st.id = ?",
        "uiu", session->user_id, count, showtime_id);
    if (n != 1) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    int booking_id = 0;   /* auto-increment id of the row this connection just inserted */
    if (stmt_get_int(db, &booking_id, "SELECT LAST_INSERT_ID()", NULL) != 0 || booking_id <= 0) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }

    for (int i = 0; i < count; i++) {
        /* The JOIN also proves the seat belongs to the showtime room. */
        n = stmt_execute(db,
            "INSERT INTO booking_seats (booking_id, seat_id, price) "
            "SELECT ?, s.id, st.price FROM showtimes st "
            "JOIN seats s ON s.room_id = st.room_id WHERE st.id = ? AND s.id = ?",
            "uuu", (unsigned long long)booking_id, showtime_id, seats[i]);
        if (n != 1) {
            tx_rollback(db);
            return n == 0 ? INVALID_SEAT_LIST : DB_QUERY_ERROR;
        }
    }

    StmtRows *info = stmt_select(db, "SELECT total_amount, expires_at FROM bookings WHERE id = ?",
                                 "u", (unsigned long long)booking_id);
    if (!info || info->count != 1) {
        if (info) free_stmt_rows(info);
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    char seat_csv[MAX_SEATS_PER_BOOKING * 21];
    size_t used = 0;
    seat_csv[0] = 0;
    for (int i = 0; i < count; i++)
        used += (size_t)snprintf(seat_csv + used, sizeof(seat_csv) - used, "%s%llu", i ? "," : "", seats[i]);

    ResponseBody body = {resp_buf, buf_size, 0};
    bool ok = append_line(&body, "BOOKING|%d|PENDING|%s|%s|%s", booking_id, info->data[0][0], info->data[0][1], seat_csv);
    free_stmt_rows(info);
    if (!ok) {
        tx_rollback(db);
        return RESPONSE_TOO_LARGE;
    }
    if (!tx_commit(db)) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    return BOOK_TICKETS_SUCCESS;
}

int handle_book_tickets(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "is", &fields)) return INVALID_REQUEST;
    unsigned long long showtime_id = strtoull(fields.value[0], NULL, 10);
    unsigned long long seats[MAX_SEATS_PER_BOOKING];
    int count = parse_seat_list(fields.value[1], seats);
    if (count < 1) return INVALID_SEAT_LIST;

    if (!showtime_lock(showtime_id)) return SERVER_BUSY;     /* 1. queue here, holding no connection */
    int code = SERVER_BUSY;
    MYSQL *db = booking_db_acquire();                        /* 2. only now borrow a connection */
    if (db) {
        code = book_in_transaction(db, session, showtime_id, seats, count, resp_buf, buf_size);
        booking_db_release(db);                              /* 3. connection back first */
    }
    showtime_unlock(showtime_id);                            /* 4. lock last */
    return code;
}

/* ---------------------------------------------------------------------------
 * BOOK_TICKET (single seat, immediate ticket, no wallet): kept ONLY so the week 6
 * clients and tests keep working. Same lock, same connection rule and same
 * availability test as BOOK_TICKETS, so it can never take a seat that is held.
 * Remove it from command_table once the clients use BOOK_TICKETS.
 * ------------------------------------------------------------------------- */
int handle_book_ticket(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "ii", &fields)) return INVALID_REQUEST;
    unsigned long long showtime_id = strtoull(fields.value[0], NULL, 10);
    unsigned long long seat_id = strtoull(fields.value[1], NULL, 10);

    if (!showtime_lock(showtime_id)) return SERVER_BUSY;
    int code = SERVER_BUSY;
    MYSQL *db = booking_db_acquire();
    if (db) {
        code = BOOK_TICKET_FAIL;
        if (seat_is_unavailable(db, showtime_id, seat_id) == 0 &&
            stmt_execute(db,
                "INSERT INTO tickets (user_id, showtime_id, seat_id) "
                "SELECT ?, st.id, s.id FROM showtimes st JOIN seats s ON s.room_id = st.room_id "
                "WHERE st.id = ? AND s.id = ?",
                "uuu", session->user_id, showtime_id, seat_id) == 1)
            code = BOOK_TICKET_SUCCESS;
        booking_db_release(db);   /* a single autocommit statement: nothing is left open */
    }
    showtime_unlock(showtime_id);
    return code;
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
