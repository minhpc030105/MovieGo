#ifndef CONNECT_H
#define CONNECT_H

#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

#define SERVER "localhost"
#define USER "root"
#define PASSWORD "123456"
#define DATABASE "booking"

/**
 * @brief Kết nối tới cơ sở dữ liệu MySQL
 * @param connection Con trỏ kép nhận đối tượng MYSQL*
 * @return 0 nếu kết nối thành công, -1 nếu thất bại (không dùng exit để tránh sập server)
 */
static inline int connectDatabase(MYSQL **connection) {
    if (connection == NULL) {
        fprintf(stderr, "Error: connection pointer is NULL\n");
        return -1;
    }

    *connection = mysql_init(NULL);
    if (*connection == NULL) {
        fprintf(stderr, "Error: mysql_init failed (not enough memory)\n");
        return -1;
    }
    
    if (mysql_real_connect(*connection, SERVER, USER, PASSWORD, DATABASE, 0, getenv("MYSQL_UNIX_PORT"), CLIENT_FOUND_ROWS) == NULL) {
        fprintf(stderr, "Error connecting to database '%s': %s\n", DATABASE, mysql_error(*connection));
        mysql_close(*connection);
        *connection = NULL;
        return -1;
    }

    return 0;
}

#endif // CONNECT_H
