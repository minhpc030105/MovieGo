#ifndef QUERYUSER_H
#define QUERYUSER_H

#include <stdbool.h>
#include <mysql/mysql.h>

/**
 * @brief Xác thực đăng nhập của người dùng qua MySQL Prepared Statements
 * @param db Kết nối CSDL
 * @param username Tên đăng nhập
 * @param password Mật khẩu người dùng nhập vào
 * @param out_user_id Nhận id của người dùng nếu thành công
 * @param out_role_id Nhận role_id (0: Admin, 1: Manager, 2: User)
 * @return 0 nếu đăng nhập thành công, 1 nếu sai thông tin / không tìm thấy, -1 nếu lỗi DB
 */
int user_verify_login(MYSQL *db, const char *username, const char *password,
                      unsigned long long *out_user_id, int *out_role_id);

/**
 * @brief Đăng ký người dùng mới bằng Prepared Statements (băm PBKDF2 + salt ngẫu nhiên)
 * @param db Kết nối CSDL
 * @param name Tên đầy đủ
 * @param username Tên đăng nhập
 * @param password Mật khẩu
 * @param role_id Vai trò (mặc định 2 cho khách hàng)
 * @return 1 nếu đăng ký thành công, 0 nếu username đã tồn tại, -1 nếu lỗi DB
 */
int user_register(MYSQL *db, const char *name, const char *username, const char *password, int role_id);

/**
 * @brief Đổi mật khẩu người dùng bằng Prepared Statements (xác thực PBKDF2 cũ, sinh salt mới)
 * @param db Kết nối CSDL
 * @param user_id ID người dùng từ session
 * @param old_password Mật khẩu cũ
 * @param new_password Mật khẩu mới
 * @return 1 nếu đổi thành công, 0 nếu mật khẩu cũ không đúng, -1 nếu lỗi DB
 */
int user_change_password(MYSQL *db, unsigned long long user_id,
                         const char *old_password, const char *new_password);

/**
 * @brief Lấy vai trò role_id của người dùng qua ID (dùng cho refresh_role)
 * @param db Kết nối CSDL
 * @param user_id ID người dùng
 * @param out_role_id Nhận role_id
 * @return 0 nếu thành công, -1 nếu lỗi hoặc không tìm thấy
 */
int user_get_role(MYSQL *db, unsigned long long user_id, int *out_role_id);

/**
 * @brief Lấy ID người dùng theo username bằng Prepared Statements
 * @return ID (>= 0), hoặc -1 nếu không tìm thấy / lỗi
 */
int getUserIdByUsername(MYSQL *db, const char *username);

#endif /* QUERYUSER_H */
