#include "security_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

/**
 * @brief Sinh chuỗi Salt ngẫu nhiên dạng Hex (16 bytes = 32 hex chars)
 * @param salt_out Buffer nhận chuỗi hex (tối thiểu SALT_HEX_LEN bytes)
 * @param max_len Kích thước buffer truyền vào
 */
void generate_salt(char *salt_out, size_t max_len) {
    if (salt_out == NULL || max_len < SALT_HEX_LEN) {
        return;
    }

    unsigned char raw_salt[SALT_LEN];
    /* Sử dụng hàm tạo số ngẫu nhiên an toàn mật mã của OpenSSL */
    if (RAND_bytes(raw_salt, sizeof(raw_salt)) != 1) {
        /* Dự phòng: thử đọc từ /dev/urandom nếu RAND_bytes không thành công */
        FILE *fp = fopen("/dev/urandom", "rb");
        if (fp != NULL) {
            size_t read_bytes = fread(raw_salt, 1, sizeof(raw_salt), fp);
            fclose(fp);
            if (read_bytes != sizeof(raw_salt)) {
                for (size_t i = 0; i < sizeof(raw_salt); i++) {
                    raw_salt[i] = (unsigned char)(rand() % 256);
                }
            }
        } else {
            for (size_t i = 0; i < sizeof(raw_salt); i++) {
                raw_salt[i] = (unsigned char)(rand() % 256);
            }
        }
    }

    /* Chuyển đổi 16 bytes sang 32 ký tự hex viết thường */
    for (size_t i = 0; i < sizeof(raw_salt); i++) {
        snprintf(salt_out + (i * 2), 3, "%02x", raw_salt[i]);
    }
    salt_out[SALT_HEX_LEN - 1] = '\0';
}

/**
 * @brief Băm mật khẩu bằng OpenSSL PKCS5_PBKDF2_HMAC (SHA-256) 10.000 vòng lặp
 * @param password Mật khẩu gốc dạng plaintext
 * @param salt Chuỗi salt
 * @param hash_out Buffer nhận chuỗi băm hex (tối thiểu 65 bytes)
 */
void hash_password(const char *password, const char *salt, char *hash_out) {
    if (password == NULL || salt == NULL || hash_out == NULL) {
        return;
    }
    hash_out[0] = '\0';

    unsigned char derived_key[32]; // 32 bytes = 256 bits derived key
    int pass_len = (int)strlen(password);
    int salt_len = (int)strlen(salt);

    int ret = PKCS5_PBKDF2_HMAC(password, pass_len,
                                (const unsigned char *)salt, salt_len,
                                PBKDF2_ITERATIONS,
                                EVP_sha256(),
                                sizeof(derived_key), derived_key);
    if (ret != 1) {
        fprintf(stderr, "Error: PKCS5_PBKDF2_HMAC failed\n");
        return;
    }

    /* Chuyển đổi 32 bytes derived key sang 64 ký tự hex viết thường */
    for (size_t i = 0; i < sizeof(derived_key); i++) {
        snprintf(hash_out + (i * 2), 3, "%02x", derived_key[i]);
    }
    hash_out[HASH_HEX_LEN - 1] = '\0';
}

/**
 * @brief Xác thực mật khẩu nhập vào với hash và salt lưu trong CSDL
 * @param password Mật khẩu người dùng nhập vào
 * @param salt Chuỗi salt lấy từ CSDL
 * @param expected_hash Chuỗi hash lấy từ CSDL
 * @return 1 nếu khớp, 0 nếu sai
 */
int verify_password(const char *password, const char *salt, const char *expected_hash) {
    if (password == NULL || salt == NULL || expected_hash == NULL) {
        return 0;
    }

    char computed_hash[HASH_HEX_LEN];
    hash_password(password, salt, computed_hash);

    if (strlen(computed_hash) != strlen(expected_hash)) {
        return 0;
    }

    /* So sánh thời gian hằng số (constant-time) để phòng chống tấn công dò thời gian */
    int diff = 0;
    for (size_t i = 0; i < HASH_HEX_LEN - 1; i++) {
        diff |= (computed_hash[i] ^ expected_hash[i]);
    }

    return (diff == 0) ? 1 : 0;
}
