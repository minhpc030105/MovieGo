#ifndef MOVIEGO_DATA_H
#define MOVIEGO_DATA_H

#include "function.h"
#include "protocol.h"
#include <stdbool.h>
#include <mysql/mysql.h>

#define FIELD_LIMIT 255

typedef struct EscapedField {
    char text[FIELD_LIMIT * 2 + 1];
} EscapedField;

bool escape_field(MYSQL *db, const char *input, EscapedField *out);

/* Cấu trúc chứa kết quả truy vấn từ MySQL Prepared Statements */
typedef struct StmtRows {
    int count;
    int cols;
    char ***data; /* data[row_idx][col_idx] */
} StmtRows;

/* Giải phóng bộ nhớ của StmtRows */
void free_stmt_rows(StmtRows *rows);

/**
 * @brief Thực thi câu lệnh SELECT bằng MySQL Prepared Statements
 * @param db Con trỏ MYSQL kết nối
 * @param sql Câu lệnh SQL có chứa dấu ?
 * @param types Chuỗi định dạng kiểu tham số: 's' (chuỗi), 'i' (int), 'u' (uint64)
 * @return Con trỏ StmtRows* chứa danh sách các dòng kết quả, hoặc NULL nếu lỗi
 */
StmtRows *stmt_select(MYSQL *db, const char *sql, const char *types, ...);

/**
 * @brief Thực thi câu lệnh INSERT / UPDATE / DELETE bằng MySQL Prepared Statements
 * @param db Con trỏ MYSQL kết nối
 * @param sql Câu lệnh SQL có chứa dấu ?
 * @param types Chuỗi định dạng kiểu tham số
 * @return Số dòng bị ảnh hưởng (>= 0), hoặc -1 nếu lỗi
 */
int stmt_execute(MYSQL *db, const char *sql, const char *types, ...);

/**
 * @brief Lấy 1 giá trị số nguyên từ cột đầu tiên của dòng đầu tiên (e.g. COUNT(*), show_time)
 * @return 0 nếu tìm thấy giá trị (ghi vào *out_val), -1 nếu không tìm thấy hoặc lỗi
 */
int stmt_get_int(MYSQL *db, int *out_val, const char *sql, const char *types, ...);

/**
 * @brief Lấy 1 chuỗi ký tự từ cột đầu tiên của dòng đầu tiên (e.g. DATE_ADD)
 * @return 0 nếu tìm thấy (ghi vào out_buf), -1 nếu lỗi
 */
int stmt_get_string(MYSQL *db, char *out_buf, size_t buf_size, const char *sql, const char *types, ...);

/* Các hàm phụ trợ tương thích ngược */
MYSQL_RES *select_rows(MYSQL *db, const char *format, ...);
bool execute_update(MYSQL *db, const char *format, ...);

#endif /* MOVIEGO_DATA_H */
