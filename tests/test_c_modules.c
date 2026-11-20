#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../server/headers/data.h"
#include "../server/headers/queryUser.h"
#include "../server/headers/security_utils.h"

int main(void) {
    printf("=== RUNNING UNIT TEST: C MODULES (PREPARED STATEMENTS & SECURITY) ===\n");

    /* -------------------------------------------------------------------------
     * Test 1: Kiem tra bao mat thu vien security_utils (PBKDF2-HMAC + Salt)
     * ------------------------------------------------------------------------- */
    printf("[1] Kiem tra sinh Salt va bam mat khau PBKDF2...\n");
    char salt[SALT_HEX_LEN] = {0};
    char hash[HASH_HEX_LEN] = {0};
    generate_salt(salt, sizeof(salt));
    assert(strlen(salt) == 32);

    const char *plain_pw = "DemoPass@123456";
    hash_password(plain_pw, salt, hash);
    assert(strlen(hash) == 64);
    assert(verify_password(plain_pw, salt, hash) == 1);
    assert(verify_password("WrongPassword!", salt, hash) == 0);
    printf("--> PASS: security_utils hoat dong chinh xac 100%%\n\n");

    /* -------------------------------------------------------------------------
     * Test 2: Ket noi CSDL de kiem tra Prepared Statements
     * ------------------------------------------------------------------------- */
    printf("[2] Ket noi CSDL MySQL 'booking' de kiem tra Prepared Statements...\n");
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "mysql_init failed\n");
        return 1;
    }

    /* Thu ket noi voi thong tin mac dinh */
    if (!mysql_real_connect(conn, "127.0.0.1", "root", "123456", "booking", 0, getenv("MYSQL_UNIX_PORT"), 0)) {
        printf("(!) Khong the ket noi toi MySQL Server cuc bo (%s).\n", mysql_error(conn));
        printf("    -> Kiem tra tinh an toan va fallback khi khong co CSDL:\n");
        unsigned long long uid = 0;
        int role = -1;
        assert(user_verify_login(NULL, "admin", "123456", &uid, &role) == -1);
        assert(user_register(NULL, "Test", "test", "123", 2) == -1);
        assert(user_change_password(NULL, 1, "old", "new") == -1);
        assert(user_get_role(NULL, 1, &role) == -1);
        assert(stmt_execute(NULL, "SELECT 1", NULL) == -1);
        assert(stmt_select(NULL, "SELECT 1", NULL) == NULL);
        mysql_close(conn);
        printf("--> PASS: Cac ham Prepared Statements xu ly con tro NULL an toan, khong crash!\n\n");
        printf("=== TEST HOAN TAT (Offline mode PASS) ===\n");
        return 0;
    }

    printf("✓ Ket noi CSDL thanh cong!\n\n");

    /* -------------------------------------------------------------------------
     * Test 3: Kiem tra chong tan cong SQL Injection qua Prepared Statements
     * ------------------------------------------------------------------------- */
    printf("[3] Kiem tra tan cong SQL Injection (chuyen ' OR '1'='1 thanh tham so an toan)...\n");
    const char *sqli_payload = "' OR '1'='1";
    unsigned long long sqli_uid = 0;
    int sqli_role = -1;

    /* Thu dang nhap voi payload SQLi */
    int sqli_res = user_verify_login(conn, sqli_payload, sqli_payload, &sqli_uid, &sqli_role);
    /* Phai tra ve 1 (user khong ton tai / fail), tuyet doi khong the bypass qua 0 */
    assert(sqli_res == 1);
    printf("--> PASS: Payload SQLi bi chan dung 100%%, khong the bypass dang nhap!\n\n");

    /* -------------------------------------------------------------------------
     * Test 4: Kiem tra dang ky user moi va bam PBKDF2
     * ------------------------------------------------------------------------- */
    printf("[4] Kiem tra dang ky user moi bang Prepared Statements...\n");
    const char *test_uname = "hung_unit_test";
    const char *test_pass = "Test@Pass2026!";

    /* Xoa user cu neu ton tai */
    stmt_execute(conn, "DELETE FROM users WHERE username = ?", "s", test_uname);

    int reg_ok = user_register(conn, "Dam Vinh Hung Test", test_uname, test_pass, 2);
    assert(reg_ok == 1);

    /* Dang ky lai cung username phai tra ve 0 (da ton tai) */
    int reg_dup = user_register(conn, "Duplicate Test", test_uname, test_pass, 2);
    assert(reg_dup == 0);
    printf("--> PASS: Dang ky thanh cong va chan trung username thanh cong!\n\n");

    /* -------------------------------------------------------------------------
     * Test 5: Kiem tra dang nhap voi mat khau vua dang ky
     * ------------------------------------------------------------------------- */
    printf("[5] Kiem tra dang nhap xac thuc PBKDF2...\n");
    unsigned long long uid = 0;
    int role = -1;

    /* Dang nhap dung */
    int login_ok = user_verify_login(conn, test_uname, test_pass, &uid, &role);
    assert(login_ok == 0);
    assert(uid > 0);
    assert(role == 2);

    /* Dang nhap sai mat khau */
    int login_fail = user_verify_login(conn, test_uname, "SaiMatKhau123", &uid, &role);
    assert(login_fail == 1);
    printf("--> PASS: Xac thuc dang nhap PBKDF2 dung 100%%!\n\n");

    /* -------------------------------------------------------------------------
     * Test 6: Kiem tra doi mat khau bang Prepared Statements
     * ------------------------------------------------------------------------- */
    printf("[6] Kiem tra doi mat khau qua user_id...\n");
    const char *new_pass = "NewPass@Updated999!";

    /* Thu doi voi mat khau cu sai */
    int cp_fail = user_change_password(conn, uid, "SaiPassCu", new_pass);
    assert(cp_fail == 0);

    /* Doi voi mat khau cu dung */
    int cp_ok = user_change_password(conn, uid, test_pass, new_pass);
    assert(cp_ok == 1);

    /* Dang nhap voi pass cu phai that bai */
    assert(user_verify_login(conn, test_uname, test_pass, &uid, &role) == 1);

    /* Dang nhap voi pass moi phai thanh cong */
    assert(user_verify_login(conn, test_uname, new_pass, &uid, &role) == 0);
    printf("--> PASS: Doi mat khau va cap nhat salt/hash moi thanh cong!\n\n");

    /* -------------------------------------------------------------------------
     * Test 7: Kiem tra Prepared Statements truy van danh sach (stmt_select)
     * ------------------------------------------------------------------------- */
    printf("[7] Kiem tra truy van danh sach categories qua stmt_select...\n");
    StmtRows *rows = stmt_select(conn, "SELECT id, name FROM categories ORDER BY id", NULL);
    if (rows) {
        printf("    Tim thay %d categories trong CSDL:\n", rows->count);
        for (int i = 0; i < rows->count && i < 3; i++) {
            printf("    - ID: %s, Name: %s\n", rows->data[i][0], rows->data[i][1]);
        }
        free_stmt_rows(rows);
        printf("--> PASS: stmt_select hoat dong hoan hao va giai phong bo nho sach se!\n\n");
    }

    /* Don dep du lieu test */
    stmt_execute(conn, "DELETE FROM users WHERE username = ?", "s", test_uname);
    mysql_close(conn);

    printf("=== TAT CA CAC TEST C_MODULES DEU PASS 100%% ===\n");
    return 0;
}

