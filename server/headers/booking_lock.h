#ifndef MOVIEGO_BOOKING_LOCK_H
#define MOVIEGO_BOOKING_LOCK_H

#include <stdbool.h>

/*
 * Pure logic with NO MySQL dependency, so it can be unit-tested without a database.
 *
 * Per-showtime in-process lock (the "hot showtime" fix).
 *
 * LOCK ORDER (every handler that books or pays MUST follow it):
 *   1. showtime_lock(id)        -- wait here while holding NO database connection
 *   2. booking_db_acquire()     -- only then borrow a pooled connection
 *   3. START TRANSACTION ... COMMIT / ROLLBACK
 *   4. booking_db_release()     -- give the connection back first
 *   5. showtime_unlock(id)
 * Never wait for a showtime lock while holding a pool connection, and never take
 * a second showtime lock while holding one (no lock-ordering deadlock possible).
 *
 * Why: if 100 clients book the same showtime and each first borrowed a connection,
 * 10 of them would hold all 10 pooled connections while waiting for the lock and
 * every other showtime would get SERVER_BUSY. With lock-first, the 99 waiters hold
 * nothing: only ONE connection is used per hot showtime.
 *
 * The lock only protects this process. Correctness across processes still comes
 * from SELECT ... FOR UPDATE and UNIQUE(showtime_id, seat_id).
 */
#define SHOWTIME_LOCK_SLOTS 256           /* showtime_id % SLOTS; a collision only queues, never fails */
#ifndef SHOWTIME_LOCK_TIMEOUT_SEC
#define SHOWTIME_LOCK_TIMEOUT_SEC 30      /* long on purpose: queue instead of failing fast */
#endif

/* Returns false if the lock was not obtained in the timeout (handler answers SERVER_BUSY). */
bool showtime_lock(unsigned long long showtime_id);
void showtime_unlock(unsigned long long showtime_id);

#define MAX_SEATS_PER_BOOKING 10          /* one order holds 1..10 seats of one showtime */

/*
 * Parse "12,13,14" into seat ids. Only digits and single commas are allowed, ids
 * must be non-zero, distinct and fit in INT_MAX; 1..MAX_SEATS_PER_BOOKING items.
 * Returns the count, or -1 if the text is invalid.
 */
int parse_seat_list(const char *text, unsigned long long out[MAX_SEATS_PER_BOOKING]);

#endif
