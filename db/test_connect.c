#include <stdio.h>
#include <stdlib.h>
#include "connect.h"

int main() {
    MYSQL *conn;
    MYSQL_RES *result;
    MYSQL_ROW row;
    
    printf("Connecting to MySQL database...\n");
    
    // Kết nối database
    connectDatabase(&conn);
    
    if (conn == NULL) {
        printf("Connection failed!\n");
        return 1;
    }
    
    printf("✓ Connected successfully to database '%s'\n\n", DATABASE);
    
    // Test query 1: Đếm số phim
    printf("=== Test 1: Count films ===\n");
    if (mysql_query(conn, "SELECT COUNT(*) FROM films")) {
        printf("Query failed: %s\n", mysql_error(conn));
    } else {
        result = mysql_store_result(conn);
        if ((row = mysql_fetch_row(result))) {
            printf("Total films: %s\n\n", row[0]);
        }
        mysql_free_result(result);
    }
    
    // Test query 2: Lấy danh sách phim
    printf("=== Test 2: List films ===\n");
    if (mysql_query(conn, "SELECT title, show_time FROM films LIMIT 5")) {
        printf("Query failed: %s\n", mysql_error(conn));
    } else {
        result = mysql_store_result(conn);
        while ((row = mysql_fetch_row(result))) {
            printf("- %s (%s minutes)\n", row[0], row[1]);
        }
        mysql_free_result(result);
        printf("\n");
    }
    
    // Test query 3: Lấy thông tin rạp
    printf("=== Test 3: List cinemas ===\n");
    if (mysql_query(conn, "SELECT name, address FROM cinemas")) {
        printf("Query failed: %s\n", mysql_error(conn));
    } else {
        result = mysql_store_result(conn);
        while ((row = mysql_fetch_row(result))) {
            printf("- %s\n  %s\n", row[0], row[1]);
        }
        mysql_free_result(result);
        printf("\n");
    }
    
    // Test query 4: Đếm số vé đã đặt
    printf("=== Test 4: Count tickets ===\n");
    if (mysql_query(conn, "SELECT COUNT(*) FROM tickets")) {
        printf("Query failed: %s\n", mysql_error(conn));
    } else {
        result = mysql_store_result(conn);
        if ((row = mysql_fetch_row(result))) {
            printf("Total tickets booked: %s\n\n", row[0]);
        }
        mysql_free_result(result);
    }
    
    printf("✓ All tests completed!\n");
    
    // Đóng kết nối
    mysql_close(conn);
    
    return 0;
}
