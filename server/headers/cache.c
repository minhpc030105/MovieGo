#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global cache for showtimes
ShowtimeCache g_showtimes[MAX_SHOWTIMES];
int g_showtime_count = 0;

// Find showtime by ID
ShowtimeCache* findShowtime(int showtime_id) {
    for (int i = 0; i < g_showtime_count; i++) {
        if (g_showtimes[i].showtime_id == showtime_id)
            return &g_showtimes[i];
    }
    return NULL;
}

// Find seat in a showtime
Seat* findSeat(ShowtimeCache *st, int seat_id) {
    for (int i = 0; i < st->seat_count; i++) {
        if (st->seats[i].seat_id == seat_id)
            return &st->seats[i];
    }
    return NULL;
}

// Load showtimes from DB to RAM
static void loadShowtimes(MYSQL *conn) {
    MYSQL_RES *res;
    MYSQL_ROW row;

    mysql_query(conn, "SELECT id, room_id FROM showtimes");
    res = mysql_store_result(conn);

    while ((row = mysql_fetch_row(res))) {
        ShowtimeCache *st = &g_showtimes[g_showtime_count++];
        
        st->showtime_id = atoi(row[0]);
        st->room_id     = atoi(row[1]);
        st->seat_count  = 0;
        
        pthread_mutex_init(&st->lock, NULL);
    }
    mysql_free_result(res);
    printf("[CACHE] Loaded %d showtimes\n", g_showtime_count);
}

// Load seats for each showtime
static void loadSeats(MYSQL *conn) {
    char query[512];
    MYSQL_RES *res;
    MYSQL_ROW row;

    for (int i = 0; i < g_showtime_count; i++) {
        ShowtimeCache *st = &g_showtimes[i];

        sprintf(query,
            "SELECT id, row_name, seat_number "
            "FROM seats WHERE room_id = %d",
            st->room_id);

        mysql_query(conn, query);
        res = mysql_store_result(conn);

        while ((row = mysql_fetch_row(res))) {
            Seat *s = &st->seats[st->seat_count++];
            
            s->seat_id = atoi(row[0]);
            strcpy(s->row_name, row[1]);
            strcpy(s->seat_number, row[2]);
            s->is_booked = 0;
            s->user_id   = 0;
        }
        mysql_free_result(res);
    }
    printf("[CACHE] Loaded seats for all showtimes\n");
}

// Load existing tickets and mark seats as booked
static void loadTickets(MYSQL *conn) {
    MYSQL_RES *res;
    MYSQL_ROW row;

    mysql_query(conn,
        "SELECT showtime_id, seat_id, user_id FROM tickets");
    res = mysql_store_result(conn);

    int ticket_count = 0;
    while ((row = mysql_fetch_row(res))) {
        int showtime_id = atoi(row[0]);
        int seat_id     = atoi(row[1]);
        int user_id     = atoi(row[2]);

        ShowtimeCache *st = findShowtime(showtime_id);
        if (!st) continue;

        Seat *s = findSeat(st, seat_id);
        if (!s) continue;

        s->is_booked = 1;
        s->user_id   = user_id;
        ticket_count++;
    }
    mysql_free_result(res);
    printf("[CACHE] Loaded %d booked tickets\n", ticket_count);
}

// Initialize cache (call on server startup)
void initCache(MYSQL *conn) {
    printf("[CACHE] Initializing cache...\n");
    loadShowtimes(conn);
    loadSeats(conn);
    loadTickets(conn);
    printf("[CACHE] Cache initialization complete!!!\n");
}