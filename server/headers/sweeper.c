#include "sweeper.h"
#include "db_pool.h"
#include <pthread.h>
#include <unistd.h>

int sweeper_cancel_expired(MYSQL *db) {
    return stmt_execute(db,
        "UPDATE bookings SET status = 'CANCELLED' WHERE status = 'PENDING' AND expires_at <= NOW()", NULL);
}

static void *sweeper_main(void *arg) {
    (void)arg;
    if (mysql_thread_init() != 0) return NULL;
    for (;;) {
        MYSQL *db = db_pool_acquire(DB_ACQUIRE_TIMEOUT_MS);
        if (db) {
            sweeper_cancel_expired(db);
            db_pool_release(db);
        }
        sleep(SWEEPER_INTERVAL_SEC);
    }
    return NULL;
}

int sweeper_start(void) {
    pthread_t tid;
    if (pthread_create(&tid, NULL, sweeper_main, NULL) != 0) return -1;
    pthread_detach(tid);
    return 0;
}
