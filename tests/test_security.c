#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../server/headers/security_utils.h"

int main(void) {
    printf("=== RUNNING UNIT TEST: SECURITY_UTILS (SHA-256 + SALT) ===\n");

    /* Test 1: Sinh Salt ngẫu nhiên */
    char salt1[SALT_HEX_LEN] = {0};
    char salt2[SALT_HEX_LEN] = {0};

    generate_salt(salt1, sizeof(salt1));
    generate_salt(salt2, sizeof(salt2));

    printf("[1] Salt 1: %s (len: %zu)\n", salt1, strlen(salt1));
    printf("[2] Salt 2: %s (len: %zu)\n", salt2, strlen(salt2));

    assert(strlen(salt1) == 32);
    assert(strlen(salt2) == 32);
    assert(strcmp(salt1, salt2) != 0); // Hai salt phải khác nhau
    printf("--> PASS: Sinh salt thanh cong va ngau nhien\n\n");

    /* Test 2: Bam mat khau */
    const char *password = "Pass@123456";
    char hash1[HASH_HEX_LEN] = {0};
    char hash2[HASH_HEX_LEN] = {0};

    hash_password(password, salt1, hash1);
    hash_password(password, salt1, hash2);

    printf("[3] Password: %s\n", password);
    printf("    Hash (salt1): %s (len: %zu)\n", hash1, strlen(hash1));

    assert(strlen(hash1) == 64);
    assert(strcmp(hash1, hash2) == 0); // Cung pass + salt phai ra cung hash
    printf("--> PASS: Tinh bam nhat quan va dung do dai 64 hex\n\n");

    /* Test 3: Xac thuc dung mat khau */
    int verify_ok = verify_password(password, salt1, hash1);
    printf("[4] Verify dung mat khau: %d (ky vong 1)\n", verify_ok);
    assert(verify_ok == 1);
    printf("--> PASS: Xac thuc thanh cong voi dung mat khau\n\n");

    /* Test 4: Xac thuc sai mat khau */
    int verify_wrong_pwd = verify_password("SaiMatKhau", salt1, hash1);
    printf("[5] Verify sai mat khau: %d (ky vong 0)\n", verify_wrong_pwd);
    assert(verify_wrong_pwd == 0);
    printf("--> PASS: Chan dung khi nhap sai mat khau\n\n");

    /* Test 5: Xac thuc dung pass nhung sai salt */
    int verify_wrong_salt = verify_password(password, salt2, hash1);
    printf("[6] Verify sai salt: %d (ky vong 0)\n", verify_wrong_salt);
    assert(verify_wrong_salt == 0);
    printf("--> PASS: Chan dung khi dung sai salt\n\n");

    /* Test 6: Truyen NULL pointer */
    assert(verify_password(NULL, salt1, hash1) == 0);
    assert(verify_password(password, NULL, hash1) == 0);
    assert(verify_password(password, salt1, NULL) == 0);
    printf("--> PASS: Xu ly an toan khi truyen con tro NULL\n\n");

    printf("=== TAT CA TESTS SECURITY_UTILS DEU PASS 100%% ===\n");
    return 0;
}

