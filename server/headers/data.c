#include "data.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

bool escape_field(MYSQL *db, const char *input, EscapedField *out) {
    size_t n = strlen(input);
    return n <= FIELD_LIMIT && mysql_real_escape_string(db, out->text, input, n) < sizeof(out->text);
}

static bool run_query(MYSQL *db, const char *format, va_list args) {
    char sql[4096];
    int n = vsnprintf(sql, sizeof(sql), format, args);
    return n >= 0 && (size_t)n < sizeof(sql) && mysql_real_query(db, sql, (size_t)n) == 0;
}

MYSQL_RES *select_rows(MYSQL *db, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bool ok = run_query(db, format, args);
    va_end(args);
    return ok ? mysql_store_result(db) : NULL;
}

bool execute_update(MYSQL *db, const char *format, ...) {
    va_list args;
    va_start(args, format);
    bool ok = run_query(db, format, args);
    va_end(args);
    return ok;
}
