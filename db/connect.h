#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

#define SERVER "localhost"
#define USER "root"
#define PASSWORD "123456"
#define DATABASE "booking"

void connectDatabase(MYSQL **connection) {
    *connection = mysql_init(NULL);

    if (*connection == NULL) {
        printf("Couldn't connect to database\n");
        return;
    }
    
    if(mysql_real_connect(*connection, SERVER, USER, PASSWORD, DATABASE, 0, getenv("MYSQL_UNIX_PORT"), CLIENT_FOUND_ROWS) == NULL) {
        printf("Error connecting to database: %s\n", mysql_error(*connection));
        mysql_close(*connection);
        *connection = NULL;
        return;
    }
}

