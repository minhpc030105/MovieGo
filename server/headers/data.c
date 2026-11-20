#include "data.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PARAMS 32
#define MAX_COL_BUFFER 2048

bool escape_field(MYSQL *db, const char *input, EscapedField *out) {
    size_t n = strlen(input);
    return n <= FIELD_LIMIT && mysql_real_escape_string(db, out->text, input, n) < sizeof(out->text);
}

void free_stmt_rows(StmtRows *rows) {
    if (!rows) return;
    if (rows->data) {
        for (int i = 0; i < rows->count; i++) {
            if (rows->data[i]) {
                for (int j = 0; j < rows->cols; j++) {
                    free(rows->data[i][j]);
                }
                free(rows->data[i]);
            }
        }
        free(rows->data);
    }
    free(rows);
}

static bool bind_params_va(MYSQL_STMT *stmt, const char *types, va_list args,
                           MYSQL_BIND *binds, int *int_store,
                           unsigned long long *ull_store,
                           const char **str_store, unsigned long *len_store) {
    if (!types || !*types) return true;
    size_t n = strlen(types);
    if (n > MAX_PARAMS) return false;
    memset(binds, 0, sizeof(MYSQL_BIND) * n);

    for (size_t i = 0; i < n; i++) {
        char t = types[i];
        if (t == 's') {
            const char *str = va_arg(args, const char *);
            str_store[i] = str ? str : "";
            len_store[i] = (unsigned long)strlen(str_store[i]);
            binds[i].buffer_type = MYSQL_TYPE_STRING;
            binds[i].buffer = (void *)str_store[i];
            binds[i].buffer_length = len_store[i];
            binds[i].length = &len_store[i];
        } else if (t == 'i') {
            int_store[i] = va_arg(args, int);
            binds[i].buffer_type = MYSQL_TYPE_LONG;
            binds[i].buffer = (void *)&int_store[i];
            binds[i].is_unsigned = 0;
        } else if (t == 'u') {
            ull_store[i] = va_arg(args, unsigned long long);
            binds[i].buffer_type = MYSQL_TYPE_LONGLONG;
            binds[i].buffer = (void *)&ull_store[i];
            binds[i].is_unsigned = 1;
        } else {
            return false;
        }
    }
    return mysql_stmt_bind_param(stmt, binds) == 0;
}

StmtRows *stmt_select(MYSQL *db, const char *sql, const char *types, ...) {
    if (!db || !sql) return NULL;
    MYSQL_STMT *stmt = mysql_stmt_init(db);
    if (!stmt) return NULL;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return NULL;
    }

    MYSQL_BIND in_binds[MAX_PARAMS];
    int int_store[MAX_PARAMS];
    unsigned long long ull_store[MAX_PARAMS];
    const char *str_store[MAX_PARAMS];
    unsigned long len_store[MAX_PARAMS];

    va_list args;
    va_start(args, types);
    bool bind_ok = bind_params_va(stmt, types, args, in_binds,
                                  int_store, ull_store, str_store, len_store);
    va_end(args);

    if (!bind_ok || mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return NULL;
    }

    if (mysql_stmt_store_result(stmt) != 0) {
        mysql_stmt_close(stmt);
        return NULL;
    }

    MYSQL_RES *meta = mysql_stmt_result_metadata(stmt);
    if (!meta) {
        mysql_stmt_close(stmt);
        return NULL;
    }

    unsigned int num_fields = mysql_num_fields(meta);
    if (num_fields == 0) {
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return NULL;
    }

    MYSQL_BIND *out_binds = calloc(num_fields, sizeof(MYSQL_BIND));
    char **col_buffers = malloc(num_fields * sizeof(char *));
    unsigned long *col_lengths = malloc(num_fields * sizeof(unsigned long));
    mysql_bind_bool_t *is_null = malloc(num_fields * sizeof(mysql_bind_bool_t));

    for (unsigned int i = 0; i < num_fields; i++) {
        col_buffers[i] = malloc(MAX_COL_BUFFER);
        col_buffers[i][0] = '\0';
        out_binds[i].buffer_type = MYSQL_TYPE_STRING;
        out_binds[i].buffer = col_buffers[i];
        out_binds[i].buffer_length = MAX_COL_BUFFER - 1;
        out_binds[i].length = &col_lengths[i];
        out_binds[i].is_null = &is_null[i];
    }

    if (mysql_stmt_bind_result(stmt, out_binds) != 0) {
        for (unsigned int i = 0; i < num_fields; i++) free(col_buffers[i]);
        free(col_buffers);
        free(col_lengths);
        free(is_null);
        free(out_binds);
        mysql_free_result(meta);
        mysql_stmt_close(stmt);
        return NULL;
    }

    my_ulonglong total_rows = mysql_stmt_num_rows(stmt);
    StmtRows *res = calloc(1, sizeof(StmtRows));
    res->cols = (int)num_fields;
    res->data = malloc(sizeof(char **) * (total_rows > 0 ? (size_t)total_rows : 1));

    while (mysql_stmt_fetch(stmt) == 0) {
        char **row = malloc(sizeof(char *) * num_fields);
        for (unsigned int i = 0; i < num_fields; i++) {
            if (is_null[i]) {
                row[i] = strdup("");
            } else {
                col_buffers[i][col_lengths[i]] = '\0';
                row[i] = strdup(col_buffers[i]);
            }
        }
        res->data[res->count++] = row;
    }

    for (unsigned int i = 0; i < num_fields; i++) free(col_buffers[i]);
    free(col_buffers);
    free(col_lengths);
    free(is_null);
    free(out_binds);
    mysql_free_result(meta);
    mysql_stmt_close(stmt);
    return res;
}

int stmt_execute(MYSQL *db, const char *sql, const char *types, ...) {
    if (!db || !sql) return -1;
    MYSQL_STMT *stmt = mysql_stmt_init(db);
    if (!stmt) return -1;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND in_binds[MAX_PARAMS];
    int int_store[MAX_PARAMS];
    unsigned long long ull_store[MAX_PARAMS];
    const char *str_store[MAX_PARAMS];
    unsigned long len_store[MAX_PARAMS];

    va_list args;
    va_start(args, types);
    bool bind_ok = bind_params_va(stmt, types, args, in_binds,
                                  int_store, ull_store, str_store, len_store);
    va_end(args);

    if (!bind_ok || mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    my_ulonglong affected = mysql_stmt_affected_rows(stmt);
    mysql_stmt_close(stmt);
    return (int)affected;
}

int stmt_get_int(MYSQL *db, int *out_val, const char *sql, const char *types, ...) {
    if (!db || !sql || !out_val) return -1;
    MYSQL_STMT *stmt = mysql_stmt_init(db);
    if (!stmt) return -1;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND in_binds[MAX_PARAMS];
    int int_store[MAX_PARAMS];
    unsigned long long ull_store[MAX_PARAMS];
    const char *str_store[MAX_PARAMS];
    unsigned long len_store[MAX_PARAMS];

    va_list args;
    va_start(args, types);
    bool bind_ok = bind_params_va(stmt, types, args, in_binds,
                                  int_store, ull_store, str_store, len_store);
    va_end(args);

    if (!bind_ok || mysql_stmt_execute(stmt) != 0 || mysql_stmt_store_result(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    int val = 0;
    mysql_bind_bool_t is_null = 0;
    MYSQL_BIND out_bind;
    memset(&out_bind, 0, sizeof(out_bind));
    out_bind.buffer_type = MYSQL_TYPE_LONG;
    out_bind.buffer = &val;
    out_bind.is_null = &is_null;

    if (mysql_stmt_bind_result(stmt, &out_bind) != 0 || mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    *out_val = val;
    mysql_stmt_close(stmt);
    return 0;
}

int stmt_get_string(MYSQL *db, char *out_buf, size_t buf_size, const char *sql, const char *types, ...) {
    if (!db || !sql || !out_buf || buf_size == 0) return -1;
    MYSQL_STMT *stmt = mysql_stmt_init(db);
    if (!stmt) return -1;

    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    MYSQL_BIND in_binds[MAX_PARAMS];
    int int_store[MAX_PARAMS];
    unsigned long long ull_store[MAX_PARAMS];
    const char *str_store[MAX_PARAMS];
    unsigned long len_store[MAX_PARAMS];

    va_list args;
    va_start(args, types);
    bool bind_ok = bind_params_va(stmt, types, args, in_binds,
                                  int_store, ull_store, str_store, len_store);
    va_end(args);

    if (!bind_ok || mysql_stmt_execute(stmt) != 0 || mysql_stmt_store_result(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    unsigned long len = 0;
    mysql_bind_bool_t is_null = 0;
    MYSQL_BIND out_bind;
    memset(&out_bind, 0, sizeof(out_bind));
    out_bind.buffer_type = MYSQL_TYPE_STRING;
    out_bind.buffer = out_buf;
    out_bind.buffer_length = (unsigned long)(buf_size - 1);
    out_bind.length = &len;
    out_bind.is_null = &is_null;

    if (mysql_stmt_bind_result(stmt, &out_bind) != 0 || mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    if (len < buf_size) out_buf[len] = '\0';
    else out_buf[buf_size - 1] = '\0';

    mysql_stmt_close(stmt);
    return 0;
}

/* Các hàm tương thích ngược (khi cần) */
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
