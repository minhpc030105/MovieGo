#include "db_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

typedef struct {
    MYSQL *conn;
    int in_use;
} DBConnection;

typedef struct {
    DBConnection *connections;
    int capacity;
    int available_count;
    int is_initialized;
    int is_shutdown;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char host[128];
    char user[128];
    char pass[128];
    char db[128];
} DBPool;

static DBPool g_pool = {
    .connections = NULL,
    .capacity = 0,
    .available_count = 0,
    .is_initialized = 0,
    .is_shutdown = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .host = {0},
    .user = {0},
    .pass = {0},
    .db = {0}
};

static void get_timeout_timespec(unsigned int timeout_ms, struct timespec *ts) {
#if defined(CLOCK_REALTIME)
    clock_gettime(CLOCK_REALTIME, ts);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000L;
#endif
    ts->tv_sec += (time_t)(timeout_ms / 1000);
    ts->tv_nsec += (long)((timeout_ms % 1000) * 1000000L);
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000L;
    }
}

/**
 * @brief Khởi tạo pool kết nối MySQL
 */
int db_pool_init(const char *host, const char *user, const char *pass, const char *db, int pool_size) {
    if (host == NULL || user == NULL || pass == NULL || db == NULL) {
        fprintf(stderr, "db_pool_init error: invalid parameters (NULL pointer)\n");
        return -1;
    }

    if (pool_size <= 0) {
        pool_size = DEFAULT_DB_POOL_SIZE;
    }

    pthread_mutex_lock(&g_pool.lock);

    if (g_pool.is_initialized) {
        fprintf(stderr, "db_pool_init warning: pool is already initialized\n");
        pthread_mutex_unlock(&g_pool.lock);
        return 0;
    }

    g_pool.connections = (DBConnection *)calloc((size_t)pool_size, sizeof(DBConnection));
    if (g_pool.connections == NULL) {
        fprintf(stderr, "db_pool_init error: cannot allocate memory for connections array\n");
        pthread_mutex_unlock(&g_pool.lock);
        return -1;
    }

    /* Lưu thông tin cấu hình phục vụ reconnect */
    strncpy(g_pool.host, host, sizeof(g_pool.host) - 1);
    strncpy(g_pool.user, user, sizeof(g_pool.user) - 1);
    strncpy(g_pool.pass, pass, sizeof(g_pool.pass) - 1);
    strncpy(g_pool.db, db, sizeof(g_pool.db) - 1);

    for (int i = 0; i < pool_size; i++) {
        MYSQL *conn = mysql_init(NULL);
        if (conn == NULL) {
            fprintf(stderr, "db_pool_init error: mysql_init failed at index %d\n", i);
            /* Thu hồi các kết nối đã tạo trước đó */
            for (int j = 0; j < i; j++) {
                if (g_pool.connections[j].conn != NULL) {
                    mysql_close(g_pool.connections[j].conn);
                }
            }
            free(g_pool.connections);
            g_pool.connections = NULL;
            pthread_mutex_unlock(&g_pool.lock);
            return -1;
        }

        /* Bật tự động kết nối lại nếu phiên bản MySQL hỗ trợ */
        bool reconnect = 1;
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

        if (mysql_real_connect(conn, host, user, pass, db, 0, getenv("MYSQL_UNIX_PORT"), 0) == NULL) {
            fprintf(stderr, "db_pool_init error: connect to database '%s' failed at index %d: %s\n",
                    db, i, mysql_error(conn));
            mysql_close(conn);
            for (int j = 0; j < i; j++) {
                if (g_pool.connections[j].conn != NULL) {
                    mysql_close(g_pool.connections[j].conn);
                }
            }
            free(g_pool.connections);
            g_pool.connections = NULL;
            pthread_mutex_unlock(&g_pool.lock);
            return -1;
        }

        g_pool.connections[i].conn = conn;
        g_pool.connections[i].in_use = 0;
    }

    g_pool.capacity = pool_size;
    g_pool.available_count = pool_size;
    g_pool.is_shutdown = 0;
    g_pool.is_initialized = 1;

    pthread_mutex_unlock(&g_pool.lock);
    return 0;
}

/**
 * @brief Mượn 1 kết nối MySQL từ pool có timeout (dùng pthread_cond_timedwait)
 */
MYSQL* db_pool_acquire(unsigned int timeout_ms) {
    if (timeout_ms == 0) {
        timeout_ms = DB_ACQUIRE_TIMEOUT_MS;
    }

    pthread_mutex_lock(&g_pool.lock);

    if (!g_pool.is_initialized || g_pool.is_shutdown) {
        pthread_mutex_unlock(&g_pool.lock);
        return NULL;
    }

    struct timespec ts;
    get_timeout_timespec(timeout_ms, &ts);

    /* Chờ cho tới khi có ít nhất 1 kết nối rảnh, hoặc hết thời gian timeout, hoặc pool bị đóng */
    while (g_pool.available_count == 0 && !g_pool.is_shutdown) {
        int rc = pthread_cond_timedwait(&g_pool.cond, &g_pool.lock, &ts);
        if (rc == ETIMEDOUT) {
            /* Quá thời gian timeout chờ kết nối (mặc định 2000ms) -> trả NULL để server trả SERVER_BUSY */
            pthread_mutex_unlock(&g_pool.lock);
            return NULL;
        }
    }

    if (g_pool.is_shutdown || g_pool.available_count == 0) {
        pthread_mutex_unlock(&g_pool.lock);
        return NULL;
    }

    int found_idx = -1;
    for (int i = 0; i < g_pool.capacity; i++) {
        if (!g_pool.connections[i].in_use) {
            g_pool.connections[i].in_use = 1;
            g_pool.available_count--;
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) {
        pthread_mutex_unlock(&g_pool.lock);
        return NULL;
    }

    MYSQL *conn = g_pool.connections[found_idx].conn;
    pthread_mutex_unlock(&g_pool.lock);

    /* Kiểm tra tình trạng kết nối bằng mysql_ping() hoặc nếu ô kết nối đang chứa NULL */
    bool need_reconnect = (conn == NULL);
    if (conn != NULL && mysql_ping(conn) != 0) {
        fprintf(stderr, "db_pool_acquire: mysql_ping failed, reconnecting: %s\n", mysql_error(conn));
        mysql_close(conn);
        conn = NULL;
        need_reconnect = true;
    }

    if (need_reconnect) {
        MYSQL *new_conn = mysql_init(NULL);
        if (new_conn != NULL) {
            bool reconnect = 1;
            mysql_options(new_conn, MYSQL_OPT_RECONNECT, &reconnect);
            if (mysql_real_connect(new_conn, g_pool.host, g_pool.user, g_pool.pass, g_pool.db, 0, getenv("MYSQL_UNIX_PORT"), 0) != NULL) {
                conn = new_conn;
            } else {
                fprintf(stderr, "db_pool_acquire: reconnect failed: %s\n", mysql_error(new_conn));
                mysql_close(new_conn);
                conn = NULL;
            }
        } else {
            conn = NULL;
        }

        pthread_mutex_lock(&g_pool.lock);
        g_pool.connections[found_idx].conn = conn;
        if (conn == NULL) {
            /* Nếu tái kết nối thất bại, giải phóng slot và tăng lại available_count để lần mượn tiếp theo có thể thử lại */
            g_pool.connections[found_idx].in_use = 0;
            g_pool.available_count++;
            pthread_cond_signal(&g_pool.cond);
        }
        pthread_mutex_unlock(&g_pool.lock);
    }

    return conn;
}

MYSQL* db_pool_acquire_timeout(unsigned int timeout_ms) {
    return db_pool_acquire(timeout_ms);
}

int db_pool_available_count(void) {
    pthread_mutex_lock(&g_pool.lock);
    int count = g_pool.available_count;
    pthread_mutex_unlock(&g_pool.lock);
    return count;
}

/**
 * @brief Trả kết nối MySQL về lại pool ngay sau khi query xong
 */
void db_pool_release(MYSQL *conn) {
    if (conn == NULL) {
        return;
    }

    pthread_mutex_lock(&g_pool.lock);

    if (!g_pool.is_initialized) {
        pthread_mutex_unlock(&g_pool.lock);
        return;
    }

    int found = 0;
    for (int i = 0; i < g_pool.capacity; i++) {
        if (g_pool.connections[i].conn == conn) {
            if (g_pool.connections[i].in_use) {
                g_pool.connections[i].in_use = 0;
                g_pool.available_count++;
                pthread_cond_signal(&g_pool.cond);
            }
            found = 1;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "db_pool_release warning: connection %p does not belong to pool\n", (void*)conn);
    }

    pthread_mutex_unlock(&g_pool.lock);
}

/**
 * @brief Đóng và giải phóng toàn bộ kết nối khi tắt server
 */
void db_pool_destroy(void) {
    pthread_mutex_lock(&g_pool.lock);

    if (!g_pool.is_initialized) {
        pthread_mutex_unlock(&g_pool.lock);
        return;
    }

    g_pool.is_shutdown = 1;
    pthread_cond_broadcast(&g_pool.cond);

    if (g_pool.connections != NULL) {
        for (int i = 0; i < g_pool.capacity; i++) {
            if (g_pool.connections[i].conn != NULL) {
                mysql_close(g_pool.connections[i].conn);
                g_pool.connections[i].conn = NULL;
            }
        }
        free(g_pool.connections);
        g_pool.connections = NULL;
    }

    g_pool.capacity = 0;
    g_pool.available_count = 0;
    g_pool.is_initialized = 0;

    pthread_mutex_unlock(&g_pool.lock);
}
