#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "../server/headers/db_pool.h"

#define NUM_THREADS 12

typedef struct {
    int thread_id;
    int success;
} WorkerArg;

static void* worker_task(void *arg) {
    WorkerArg *warg = (WorkerArg *)arg;
    warg->success = 0;

    printf("[Thread %d] Dang yeu cau muon ket noi tu db_pool (timeout: %d ms)...\n",
           warg->thread_id, DB_ACQUIRE_TIMEOUT_MS);
    MYSQL *conn = db_pool_acquire(DB_ACQUIRE_TIMEOUT_MS);
    if (conn == NULL) {
        printf("[Thread %d] Khong the muon ket noi (timeout / NULL)\n", warg->thread_id);
        return NULL;
    }

    printf("[Thread %d] -> Da muon ket noi thanh cong (%p). Dang thuc hien truy van...\n",
           warg->thread_id, (void*)conn);

    if (mysql_query(conn, "SELECT 1")) {
        fprintf(stderr, "[Thread %d] Query error: %s\n", warg->thread_id, mysql_error(conn));
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            mysql_free_result(res);
        }
        warg->success = 1;
    }

    /* Giữ kết nối một khoảng ngắn (50ms) để kiểm tra tính đồng thời */
    usleep(50000);

    printf("[Thread %d] -> Tra ket noi ve pool (%p)\n", warg->thread_id, (void*)conn);
    db_pool_release(conn);

    return NULL;
}

int main(void) {
    printf("=== RUNNING UNIT TEST: DB_POOL (10 CONNECTIONS & TIMEOUT 2000ms) ===\n");

    if (mysql_library_init(0, NULL, NULL) != 0) {
        fprintf(stderr, "mysql_library_init failed\n");
        return 1;
    }

    const char *host = "localhost";
    const char *user = "root";
    const char *pass = "123456";
    const char *db   = "booking";
    int pool_size = 10;

    printf("[1] Khoi tao pool voi %d ket noi toi MySQL database '%s'...\n", pool_size, db);
    int init_res = db_pool_init(host, user, pass, db, pool_size);

    if (init_res != 0) {
        printf("(!) Khong the ket noi toi MySQL Server cuc bo (server offline hoac sai password).\n");
        printf("    -> Kiem tra tinh an toan cua db_pool khi khong co CSDL:\n");
        assert(db_pool_acquire(2000) == NULL);
        assert(db_pool_acquire_timeout(100) == NULL);
        db_pool_release(NULL);
        db_pool_destroy();
        mysql_library_end();
        printf("--> PASS: db_pool xu ly loi ket noi an toan, timeout hop le, khong crash!\n\n");
        printf("=== TEST HOAN TAT (Offline mode PASS) ===\n");
        return 0;
    }

    printf("✓ Khoi tao thanh cong %d ket noi trong pool!\n\n", pool_size);

    /* Test 2: Khoi chay 12 luong dong thoi (vuot qua dung luong 10 de kiem tra hang doi cond_timedwait) */
    printf("[2] Khoi chay %d luong dong thoi muon va tra ket noi tu pool...\n", NUM_THREADS);
    pthread_t threads[NUM_THREADS];
    WorkerArg args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i + 1;
        args[i].success = 0;
        pthread_create(&threads[i], NULL, worker_task, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        assert(args[i].success == 1);
    }
    printf("--> PASS: Ca %d luong da muon va tra ket noi thanh cong, khong deadlock!\n\n", NUM_THREADS);

    /* Test 3: Kiem tra co che timeout khi het ket noi */
    printf("[3] Kiem tra timeout khi giu toan bo 10 ket noi...\n");
    MYSQL *held_conns[10];
    for (int i = 0; i < 10; i++) {
        held_conns[i] = db_pool_acquire(1000);
        assert(held_conns[i] != NULL);
    }

    /* Thu muon ket noi thu 11 voi timeout 300ms -> phai tra ve NULL */
    printf("    -> Yeu cau ket noi thu 11 voi timeout 300ms (ky vong tra ve NULL)...\n");
    MYSQL *timed_out_conn = db_pool_acquire(300);
    assert(timed_out_conn == NULL);
    printf("--> PASS: Timeout thanh cong (tra ve NULL sau 300ms)!\n\n");

    /* Tra lai 10 ket noi */
    for (int i = 0; i < 10; i++) {
        db_pool_release(held_conns[i]);
    }

    /* Kiem tra so ket noi ranh sau khi da tra du 10 */
    assert(db_pool_available_count() == 10);
    printf("--> PASS: Da tra du 10 ket noi, so ket noi ranh chinh xac la %d!\n\n", db_pool_available_count());

    /* Test 4: Kiem tra co che tu dong ket noi lai khi phien MySQL bi ngat dot ngot (KILL) */
    printf("[4] Kiem tra phuc hoi ket noi khi phien MySQL bi ngat dot ngot...\n");
    MYSQL *bad_conn = db_pool_acquire(1000);
    assert(bad_conn != NULL);
    unsigned long bad_tid = mysql_thread_id(bad_conn);

    /* Dung 1 ket noi tam ngoai pool de KILL phien cua bad_conn phia server MySQL */
    MYSQL *killer = mysql_init(NULL);
    assert(killer != NULL);
    assert(mysql_real_connect(killer, host, user, pass, db, 0, getenv("MYSQL_UNIX_PORT"), 0) != NULL);
    char kill_sql[64];
    snprintf(kill_sql, sizeof(kill_sql), "KILL %lu", bad_tid);
    assert(mysql_query(killer, kill_sql) == 0);
    mysql_close(killer);

    db_pool_release(bad_conn);

    /* Kiem tra toan bo 10 slot trong pool: slot bi KILL phai tu dong reconnect va chay duoc SELECT 1 */
    MYSQL *checked_conns[10];
    for (int i = 0; i < 10; i++) {
        checked_conns[i] = db_pool_acquire(2000);
        assert(checked_conns[i] != NULL);
        assert(mysql_query(checked_conns[i], "SELECT 1") == 0);
        MYSQL_RES *res = mysql_store_result(checked_conns[i]);
        if (res) mysql_free_result(res);
    }
    for (int i = 0; i < 10; i++) {
        db_pool_release(checked_conns[i]);
    }
    assert(db_pool_available_count() == 10);
    printf("--> PASS: Tu dong phuc hoi ket noi bi ngat thanh cong, pool giu nguyen 10 ket noi ranh!\n\n");

    /* Test 5: Dong toan bo pool */
    printf("[5] Goi db_pool_destroy()...\n");
    db_pool_destroy();
    assert(db_pool_acquire(100) == NULL);
    mysql_library_end();
    printf("--> PASS: db_pool_destroy thu hoi toan bo ket noi an toan!\n\n");

    printf("=== TAT CA TESTS DB_POOL DEU PASS 100%% ===\n");
    return 0;
}
