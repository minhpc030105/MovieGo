#ifndef MOVIEGO_SWEEPER_H
#define MOVIEGO_SWEEPER_H

#include "data.h"

#define SWEEPER_INTERVAL_SEC 10

/* Marks PENDING bookings whose hold expired as CANCELLED (Minh: SQL). Returns rows changed, -1 on error.
 * Correctness never depends on it: SHOW_SEATS / BOOK_TICKETS already treat expired holds as free. */
int sweeper_cancel_expired(MYSQL *db);

/* Starts the background thread (Cao Anh: one call from main after db_pool_init). It borrows a pooled
 * connection for each pass and returns it, so it never keeps a connection while sleeping. */
int sweeper_start(void);

#endif
