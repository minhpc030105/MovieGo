#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <mysql/mysql.h>

#define MAX_SHOWTIMES 50
#define MAX_SEATS     200

// Seat structure
typedef struct {
    int seat_id;
    char row_name[5];
    char seat_number[10];
    int is_booked;
    int user_id;
} Seat;

// Showtime cache structure
typedef struct {
    int showtime_id;
    int room_id;
    int seat_count;
    Seat seats[MAX_SEATS];
    pthread_mutex_t lock;
} ShowtimeCache;

// Global cache (declare extern để có thể dùng tại nhiều file)
extern ShowtimeCache g_showtimes[MAX_SHOWTIMES];
extern int g_showtime_count;

// Cache functions
void initCache(MYSQL *conn);
ShowtimeCache* findShowtime(int showtime_id);
Seat* findSeat(ShowtimeCache *st, int seat_id);

#endif