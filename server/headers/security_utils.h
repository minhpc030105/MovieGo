#ifndef SECURITY_UTILS_H
#define SECURITY_UTILS_H

#include <stddef.h>

#define SALT_LEN 16
#define SALT_HEX_LEN 33 // 16 bytes = 32 hex chars + '\0'
#define HASH_HEX_LEN 65 // 32 bytes SHA256 = 64 hex chars + '\0'

/**
 * @brief Sinh chuỗi Salt ngẫu nhiên dạng Hex (16 bytes = 32 hex chars)
 * @param salt_out Buffer nhận chuỗi hex (tối thiểu SALT_HEX_LEN bytes)
 * @param max_len Kích thước buffer truyền vào
 */
void generate_salt(char *salt_out, size_t max_len);

/**
 * @brief Băm mật khẩu kèm Salt: SHA256(password + salt)
 * @param password Mật khẩu gốc dạng plaintext
 * @param salt Chuỗi salt ngẫu nhiên
 * @param hash_out Buffer nhận chuỗi băm hex (tối thiểu 65 bytes)
 */
void hash_password(const char *password, const char *salt, char *hash_out);

/**
 * @brief Xác thực mật khẩu nhập vào với hash và salt lưu trong CSDL
 * @param password Mật khẩu người dùng nhập vào
 * @param salt Chuỗi salt lấy từ CSDL
 * @param expected_hash Chuỗi hash lấy từ CSDL
 * @return 1 nếu khớp, 0 nếu sai
 */
int verify_password(const char *password, const char *salt, const char *expected_hash);

#endif // SECURITY_UTILS_H

