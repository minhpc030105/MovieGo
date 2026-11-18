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

    printf("[Thread %d] Dang yeu cau muon ket noi tu db_pool...\n", warg->thread_id);
    MYSQL *conn = db_pool_acquire();
    if (conn == NULL) {
        printf("[Thread %d] Khong the muon ket noi (NULL)\n", warg->thread_id);
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

    /* Giữ kết nối một khoảng ngắn để kiểm tra tính đồng thời */
    usleep(50000); // 50ms

    printf("[Thread %d] -> Tra ket noi ve pool (%p)\n", warg->thread_id, (void*)conn);
    db_pool_release(conn);

    return NULL;
}

int main(void) {
    printf("=== RUNNING UNIT TEST: DB_POOL (10 CONNECTIONS & CONCURRENCY) ===\n");

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
        assert(db_pool_acquire() == NULL);
        db_pool_release(NULL);
        db_pool_destroy();
        printf("--> PASS: db_pool xu ly loi ket noi an toan, khong lam sap chuong trinh!\n\n");
        printf("=== TEST HOAN TAT (Offline mode PASS) ===\n");
        return 0;
    }

    printf("✓ Khoi tao thanh cong %d ket noi trong pool!\n\n", pool_size);

    /* Test 2: Khoi chay 12 luong dong thoi (vuot qua dung luong 10 de kiem tra hang doi cond_wait) */
    printf("[2] Khoi chay %d luong dong thoi mượn va tra ket noi tu pool...\n", NUM_THREADS);
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

    /* Test 3: Dong toan bo pool */
    printf("[3] Goi db_pool_destroy()...\n");
    db_pool_destroy();
    assert(db_pool_acquire() == NULL);
    printf("--> PASS: db_pool_destroy thu hoi toan bo ket noi an toan!\n\n");

    printf("=== TAT CA TESTS DB_POOL DEU PASS 100%% ===\n");
    return 0;
}
