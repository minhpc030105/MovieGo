/*
 * Week 7 unit test, NO database needed (links only booking_lock.c).
 *  1. parse_seat_list accepts/rejects the right inputs.
 *  2. showtime_lock really excludes.
 *  3. Starvation experiment with a simulated pool of 10 connections:
 *       lock-first  (the booking handlers)       -> 0 failures, other showtime never blocked
 *       pool-first  (the naive order, for contrast) -> requests time out
 * Build and run:  make test_booking_lock && ./test_booking_lock_out
 */
#include "../server/headers/booking_lock.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SIM_POOL_SIZE 10
#define SIM_ACQUIRE_TIMEOUT_SEC 2     /* same as DB_ACQUIRE_TIMEOUT_MS */
#define SIM_WORK_USEC 30000           /* one short transaction: 30 ms */
#define HOT_CLIENTS 100
#define OTHER_CLIENTS 30

static void test_parser(void) {
    unsigned long long ids[MAX_SEATS_PER_BOOKING];
    assert(parse_seat_list("12", ids) == 1 && ids[0] == 12);
    assert(parse_seat_list("12,13,14", ids) == 3 && ids[2] == 14);
    assert(parse_seat_list("1,2,3,4,5,6,7,8,9,10", ids) == 10);
    assert(parse_seat_list("1,2,3,4,5,6,7,8,9,10,11", ids) == -1);   /* more than 10 seats */
    assert(parse_seat_list("", ids) == -1);
    assert(parse_seat_list(NULL, ids) == -1);
    assert(parse_seat_list("1,,2", ids) == -1);
    assert(parse_seat_list(",1", ids) == -1);
    assert(parse_seat_list("1,", ids) == -1);
    assert(parse_seat_list("1, 2", ids) == -1);
    assert(parse_seat_list(" 1", ids) == -1);
    assert(parse_seat_list("-1", ids) == -1);
    assert(parse_seat_list("+1", ids) == -1);
    assert(parse_seat_list("0", ids) == -1);
    assert(parse_seat_list("1,1", ids) == -1);                         /* duplicate */
    assert(parse_seat_list("1;2", ids) == -1);
    assert(parse_seat_list("2147483647", ids) == 1);
    assert(parse_seat_list("2147483648", ids) == -1);                  /* above INT_MAX */
    assert(parse_seat_list("99999999999999999999999", ids) == -1);     /* overflow */
    printf("[1] parse_seat_list ok\n");
}

static int g_counter;
static void *bump(void *arg) {
    (void)arg;
    for (int i = 0; i < 2000; i++) {
        assert(showtime_lock(7));
        int v = g_counter;
        g_counter = v + 1;
        showtime_unlock(7);
    }
    return NULL;
}

static void test_mutual_exclusion(void) {
    pthread_t t[8];
    for (int i = 0; i < 8; i++) pthread_create(&t[i], NULL, bump, NULL);
    for (int i = 0; i < 8; i++) pthread_join(t[i], NULL);
    assert(g_counter == 8 * 2000);
    /* different showtimes use different locks: holding 7 must not block 8 */
    assert(showtime_lock(7));
    assert(showtime_lock(8));
    showtime_unlock(8);
    showtime_unlock(7);
    printf("[2] showtime_lock excludes and is per showtime ok\n");
}

/* ---- simulated pool ---- */
static sem_t g_pool;
static pthread_mutex_t g_stat = PTHREAD_MUTEX_INITIALIZER;
static int g_in_use, g_hot_in_use, g_max_hot_in_use, g_failed_hot, g_failed_other;

static bool sim_acquire(void) {
    struct timespec dl;
    clock_gettime(CLOCK_REALTIME, &dl);
    dl.tv_sec += SIM_ACQUIRE_TIMEOUT_SEC;
    while (sem_timedwait(&g_pool, &dl) != 0)
        if (errno != EINTR) return false;
    return true;
}

typedef struct { unsigned long long showtime; bool hot; bool lock_first; } Client;

static void *client_main(void *arg) {
    Client *c = arg;
    bool locked = false, have = false;
    if (c->lock_first) {                    /* what the booking handlers do */
        locked = showtime_lock(c->showtime);
        if (!locked) goto fail;
        have = sim_acquire();
    } else {                                /* naive: connection first, then wait for the lock */
        have = sim_acquire();
        if (have) locked = showtime_lock(c->showtime);
    }
    if (!have || !locked) goto fail;
    pthread_mutex_lock(&g_stat);
    g_in_use++;
    if (c->hot && ++g_hot_in_use > g_max_hot_in_use) g_max_hot_in_use = g_hot_in_use;
    pthread_mutex_unlock(&g_stat);
    usleep(SIM_WORK_USEC);                  /* the transaction */
    pthread_mutex_lock(&g_stat);
    g_in_use--;
    if (c->hot) g_hot_in_use--;
    pthread_mutex_unlock(&g_stat);
    sem_post(&g_pool);                      /* connection back first */
    showtime_unlock(c->showtime);           /* lock last */
    return NULL;
fail:
    if (have) sem_post(&g_pool);
    if (locked) showtime_unlock(c->showtime);
    pthread_mutex_lock(&g_stat);
    if (c->hot) g_failed_hot++; else g_failed_other++;
    pthread_mutex_unlock(&g_stat);
    return NULL;
}

static void run_scenario(bool lock_first) {
    g_in_use = g_hot_in_use = g_max_hot_in_use = g_failed_hot = g_failed_other = 0;
    sem_init(&g_pool, 0, SIM_POOL_SIZE);
    static Client hot[HOT_CLIENTS], other[OTHER_CLIENTS];
    static pthread_t th[HOT_CLIENTS + OTHER_CLIENTS];
    int n = 0;
    for (int i = 0; i < HOT_CLIENTS; i++) {
        hot[i] = (Client){1, true, lock_first};
        pthread_create(&th[n++], NULL, client_main, &hot[i]);
    }
    usleep(100000);                         /* the popular showtime is already saturated */
    for (int i = 0; i < OTHER_CLIENTS; i++) {
        other[i] = (Client){2, false, lock_first};
        pthread_create(&th[n++], NULL, client_main, &other[i]);
    }
    for (int i = 0; i < n; i++) pthread_join(th[i], NULL);
    sem_destroy(&g_pool);
}

static void test_starvation(void) {
    run_scenario(true);
    printf("[3] lock-first : hot failed %d/%d, other-showtime failed %d/%d, max connections used by hot showtime %d\n",
           g_failed_hot, HOT_CLIENTS, g_failed_other, OTHER_CLIENTS, g_max_hot_in_use);
    assert(g_failed_hot == 0);
    assert(g_failed_other == 0);            /* a hot showtime never starves the others */
    assert(g_max_hot_in_use == 1);          /* 100 waiting clients hold ZERO connections */

    run_scenario(false);
    printf("    pool-first : hot failed %d/%d, other-showtime failed %d/%d (SERVER_BUSY in the real server)\n",
           g_failed_hot, HOT_CLIENTS, g_failed_other, OTHER_CLIENTS);
    assert(g_failed_hot + g_failed_other > 0);   /* the problem this design removes */
}

int main(void) {
    test_parser();
    test_mutual_exclusion();
    test_starvation();
    printf("ALL BOOKING LOCK TESTS PASSED\n");
    return 0;
}
