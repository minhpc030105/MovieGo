#include <mysql/mysql.h>

#define SERVER "localhost"
#define USER "root"
#define PASSWORD "123456"
#define DATABASE "booking"

void connectDatabase(MYSQL **connection) {
    *connection = mysql_init(NULL);

    if (*connection == NULL) {
        printf("Couldn't connect to database\n");
        exit(EXIT_FAILURE);
    }
    
    if(mysql_real_connect(*connection, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0) == NULL) {
        printf("Error connecting to database: %s\n", mysql_error(*connection));
        mysql_close(*connection);
        exit(EXIT_FAILURE);
    }
}

