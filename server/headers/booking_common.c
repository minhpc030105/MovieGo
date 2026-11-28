#include "booking_common.h"

MYSQL *booking_db_acquire(void) { return db_pool_acquire(DB_ACQUIRE_TIMEOUT_MS); }
void booking_db_release(MYSQL *db) { db_pool_release(db); }

bool tx_begin(MYSQL *db) { return mysql_query(db, "START TRANSACTION") == 0; }
bool tx_commit(MYSQL *db) { return mysql_query(db, "COMMIT") == 0; }
void tx_rollback(MYSQL *db) { mysql_query(db, "ROLLBACK"); }

int seat_is_unavailable(MYSQL *db, unsigned long long showtime_id, unsigned long long seat_id) {
    int found = 0;
    if (stmt_get_int(db, &found,
            "SELECT EXISTS("
            " SELECT 1 FROM tickets WHERE showtime_id = ? AND seat_id = ?"
            " UNION ALL"
            " SELECT 1 FROM booking_seats bs JOIN bookings b ON b.id = bs.booking_id"
            "  WHERE b.showtime_id = ? AND bs.seat_id = ?"
            "    AND b.status = 'PENDING' AND b.expires_at > NOW())",
            "uuuu", showtime_id, seat_id, showtime_id, seat_id) != 0)
        return -1;
    return found ? 1 : 0;
}
