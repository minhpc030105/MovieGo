#include "booking_lock.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

static pthread_mutex_t g_showtime_locks[SHOWTIME_LOCK_SLOTS];
static pthread_once_t g_locks_once = PTHREAD_ONCE_INIT;

static void init_locks(void) {
    for (int i = 0; i < SHOWTIME_LOCK_SLOTS; i++) pthread_mutex_init(&g_showtime_locks[i], NULL);
}

bool showtime_lock(unsigned long long showtime_id) {
    pthread_once(&g_locks_once, init_locks);
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += SHOWTIME_LOCK_TIMEOUT_SEC;
    return pthread_mutex_timedlock(&g_showtime_locks[showtime_id % SHOWTIME_LOCK_SLOTS], &deadline) == 0;
}

void showtime_unlock(unsigned long long showtime_id) {
    pthread_mutex_unlock(&g_showtime_locks[showtime_id % SHOWTIME_LOCK_SLOTS]);
}

int parse_seat_list(const char *text, unsigned long long out[MAX_SEATS_PER_BOOKING]) {
    if (!text || !*text) return -1;
    int count = 0;
    const char *p = text;
    for (;;) {
        if (*p < '0' || *p > '9') return -1;            /* digit required: no spaces, signs, empty items */
        errno = 0;
        char *end = NULL;
        unsigned long long id = strtoull(p, &end, 10);
        if (errno != 0 || id == 0 || id > INT_MAX) return -1;
        if (count == MAX_SEATS_PER_BOOKING) return -1;
        for (int i = 0; i < count; i++)
            if (out[i] == id) return -1;                /* duplicate seat */
        out[count++] = id;
        if (*end == '\0') return count;
        if (*end != ',') return -1;                     /* only a comma may follow a number */
        p = end + 1;
    }
}
