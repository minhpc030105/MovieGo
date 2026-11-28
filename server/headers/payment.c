#include "booking_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Wallet payment (internal virtual wallet: users.balance + wallet_transactions).
 * The status flip PENDING -> PAID is the idempotency key: no extra token or table is
 * needed, a repeated PAY_BOOKING cannot charge twice. */

static int booking_status(MYSQL *db, unsigned long long booking_id, unsigned long long user_id, char status[16]) {
    return stmt_get_string(db, status, 16,
        "SELECT status FROM bookings WHERE id = ? AND user_id = ?", "uu", booking_id, user_id);
}

static int pay_in_transaction(MYSQL *db, Session *session, unsigned long long booking_id,
                              char *resp_buf, size_t buf_size) {
    if (!tx_begin(db)) return DB_QUERY_ERROR;

    /* 1. Conditional update: only a PENDING, unexpired booking of THIS user can be paid.
     *    The user_id condition closes the IDOR hole (paying someone else's order). */
    int n = stmt_execute(db,
        "UPDATE bookings SET status = 'PAID' "
        "WHERE id = ? AND user_id = ? AND status = 'PENDING' AND expires_at > NOW()",
        "uu", booking_id, session->user_id);
    if (n < 0) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    if (n == 0) {
        char status[16] = "";
        int found = booking_status(db, booking_id, session->user_id, status);
        tx_rollback(db);
        if (found != 0) return BOOKING_NOT_FOUND;
        if (strcmp(status, "PAID") == 0) return PAY_ALREADY_DONE;   /* no second charge */
        return BOOKING_EXPIRED;                                     /* expired hold or cancelled */
    }

    /* 2. Charge the wallet only if the balance covers the server-computed total. */
    n = stmt_execute(db,
        "UPDATE users u JOIN bookings b ON b.user_id = u.id "
        "SET u.balance = u.balance - b.total_amount "
        "WHERE b.id = ? AND u.id = ? AND u.balance >= b.total_amount",
        "uu", booking_id, session->user_id);
    if (n < 0) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    if (n == 0) {
        tx_rollback(db);                       /* also undoes the PAID flip */
        return NOT_ENOUGH_BALANCE;
    }

    /* 3. Issue the real tickets. Explicit check first so SEAT_TAKEN does not depend on
     *    reading a MySQL error number; UNIQUE(showtime_id, seat_id) stays the last safety net. */
    int seat_count = 0, clash = 0;
    if (stmt_get_int(db, &seat_count, "SELECT COUNT(*) FROM booking_seats WHERE booking_id = ?",
                     "u", booking_id) != 0 || seat_count < 1 ||
        stmt_get_int(db, &clash,
                     "SELECT COUNT(*) FROM booking_seats bs JOIN bookings b ON b.id = bs.booking_id "
                     "JOIN tickets t ON t.showtime_id = b.showtime_id AND t.seat_id = bs.seat_id "
                     "WHERE bs.booking_id = ?", "u", booking_id) != 0) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    if (clash > 0) {
        tx_rollback(db);
        return SEAT_TAKEN;
    }
    n = stmt_execute(db,
        "INSERT INTO tickets (user_id, showtime_id, seat_id, booking_id) "
        "SELECT b.user_id, b.showtime_id, bs.seat_id, b.id "
        "FROM bookings b JOIN booking_seats bs ON bs.booking_id = b.id WHERE b.id = ?",
        "u", booking_id);
    if (n != seat_count) {
        int duplicate = n < 0 && mysql_errno(db) == 1062;   /* ER_DUP_ENTRY, best effort */
        tx_rollback(db);
        return duplicate ? SEAT_TAKEN : DB_QUERY_ERROR;
    }

    /* 4. Ledger line. */
    n = stmt_execute(db,
        "INSERT INTO wallet_transactions (user_id, booking_id, amount, type) "
        "SELECT user_id, id, total_amount, 'PAYMENT' FROM bookings WHERE id = ?",
        "u", booking_id);
    if (n != 1) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }

    StmtRows *info = stmt_select(db,
        "SELECT b.total_amount, u.balance FROM bookings b JOIN users u ON u.id = b.user_id WHERE b.id = ?",
        "u", booking_id);
    if (!info || info->count != 1) {
        if (info) free_stmt_rows(info);
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    ResponseBody body = {resp_buf, buf_size, 0};
    bool ok = append_line(&body, "BOOKING|%llu|PAID|%s|%s", booking_id, info->data[0][0], info->data[0][1]);
    free_stmt_rows(info);
    if (!ok) {
        tx_rollback(db);
        return RESPONSE_TOO_LARGE;
    }
    if (!tx_commit(db)) {
        tx_rollback(db);
        return DB_QUERY_ERROR;
    }
    return PAY_BOOKING_SUCCESS;
}

/* PAY_BOOKING CRLF booking_id
 * Success 1220 + BOOKING|id|PAID|total|balance_after
 * Errors  3036 not yours / not found, 3033 already paid, 3034 hold expired or cancelled,
 *         3035 not enough balance, 3032 seat sold meanwhile, 3031 busy, 3022 database error. */
int handle_pay_booking(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    unsigned long long booking_id = strtoull(fields.value[0], NULL, 10);

    /* Peek at the showtime with a SHORT borrow (no lock, no transaction) so the showtime lock
     * can be taken FIRST. Paying uses the same lock as booking, so a payment and a new hold on
     * the same showtime never interleave inside this process. */
    MYSQL *db = booking_db_acquire();
    if (!db) return SERVER_BUSY;
    int showtime_id = 0;
    int found = stmt_get_int(db, &showtime_id,
        "SELECT showtime_id FROM bookings WHERE id = ? AND user_id = ?", "uu", booking_id, session->user_id);
    booking_db_release(db);
    if (found != 0) return BOOKING_NOT_FOUND;

    if (!showtime_lock((unsigned long long)showtime_id)) return SERVER_BUSY;
    int code = SERVER_BUSY;
    db = booking_db_acquire();
    if (db) {
        code = pay_in_transaction(db, session, booking_id, resp_buf, buf_size);
        booking_db_release(db);
    }
    showtime_unlock((unsigned long long)showtime_id);
    return code;
}

/* CANCEL_BOOKING CRLF booking_id : release the seats of an unpaid booking now.
 * Success 1230. Errors 3036 not found, 3038 already paid or already cancelled.
 * Cancelling a PAID ticket (refund) is the separate week 8 feature.
 * One conditional UPDATE, so no showtime lock is needed: freeing seats cannot create a
 * double booking, and a race with PAY_BOOKING is decided by the row lock on the booking. */
int handle_cancel_booking(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "i", &fields)) return INVALID_REQUEST;
    unsigned long long booking_id = strtoull(fields.value[0], NULL, 10);

    MYSQL *db = booking_db_acquire();
    if (!db) return SERVER_BUSY;
    int code;
    int n = stmt_execute(db,
        "UPDATE bookings SET status = 'CANCELLED' WHERE id = ? AND user_id = ? AND status = 'PENDING'",
        "uu", booking_id, session->user_id);
    if (n == 1) {
        code = CANCEL_BOOKING_SUCCESS;
    } else if (n == 0) {
        char status[16] = "";
        code = booking_status(db, booking_id, session->user_id, status) == 0 ? BOOKING_NOT_PENDING : BOOKING_NOT_FOUND;
    } else {
        code = DB_QUERY_ERROR;
    }
    booking_db_release(db);
    return code;
}
