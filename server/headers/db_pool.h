#ifndef DB_POOL_H
#define DB_POOL_H

#if defined(__has_include)
  #if __has_include(<mysql/mysql.h>)
    #include <mysql/mysql.h>
  #elif __has_include(<mysql.h>)
    #include <mysql.h>
  #else
    #include <mysql/mysql.h>
  #endif
#else
  #include <mysql/mysql.h>
#endif

#include <pthread.h>

#define DEFAULT_DB_POOL_SIZE 10

/**
 * @brief Khởi tạo pool kết nối MySQL
 * @param host Địa chỉ host database (ví dụ: "localhost")
 * @param user Tên đăng nhập database (ví dụ: "root")
 * @param pass Mật khẩu đăng nhập
 * @param db Tên cơ sở dữ liệu (ví dụ: "booking")
 * @param pool_size Số lượng kết nối khởi tạo (truyền <= 0 để dùng DEFAULT_DB_POOL_SIZE)
 * @return 0 nếu thành công, -1 nếu thất bại (không dùng exit)
 */
int db_pool_init(const char *host, const char *user, const char *pass, const char *db, int pool_size);

/**
 * @brief Mượn 1 kết nối MySQL từ pool (chờ nếu đang hết kết nối)
 * @note Tự động kiểm tra mysql_ping() để reconnect nếu kết nối bị đứt
 * @return Con trỏ MYSQL* hợp lệ, hoặc NULL nếu lỗi / pool đã bị hủy
 */
MYSQL* db_pool_acquire(void);

/**
 * @brief Trả kết nối MySQL về lại pool ngay sau khi query xong
 * @param conn Con trỏ kết nối đã mượn
 */
void db_pool_release(MYSQL *conn);

/**
 * @brief Đóng và giải phóng toàn bộ kết nối khi tắt server
 */
void db_pool_destroy(void);

#endif // DB_POOL_H
