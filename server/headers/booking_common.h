#ifndef MOVIEGO_BOOKING_COMMON_H
#define MOVIEGO_BOOKING_COMMON_H

#include "booking_lock.h"
#include "data.h"
#include "db_pool.h"

#define HOLD_MINUTES 10                 /* seat hold time; used in the SQL via HOLD_MINUTES_STR */
#define HOLD_MINUTES_STR "10"

/*
 * Booking handlers borrow their OWN connection (the dispatcher does not, see the
 * OWN_DB flag in dispatch.c), so they can take the showtime lock first.
 * acquire returns NULL when the pool is exhausted for DB_ACQUIRE_TIMEOUT_MS
 * (the handler then answers SERVER_BUSY).
 * ALWAYS ROLLBACK or COMMIT before booking_db_release (pool rule from the sprint plan).
 */
MYSQL *booking_db_acquire(void);
void booking_db_release(MYSQL *db);

bool tx_begin(MYSQL *db);
bool tx_commit(MYSQL *db);
void tx_rollback(MYSQL *db);

/*
 * 1 if the seat is sold (tickets) or held by an unexpired PENDING booking,
 * 0 if it is free, -1 on database error. An expired hold counts as free at once.
 */
int seat_is_unavailable(MYSQL *db, unsigned long long showtime_id, unsigned long long seat_id);

#endif
