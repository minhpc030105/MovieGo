#ifndef MOVIEGO_DATA_H
#define MOVIEGO_DATA_H
#include "protocol.h"

typedef struct { char text[2 * FIELD_LIMIT + 1]; } EscapedField;
bool escape_field(MYSQL *db, const char *input, EscapedField *out);
MYSQL_RES *select_rows(MYSQL *db, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
bool execute_update(MYSQL *db, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
#endif
