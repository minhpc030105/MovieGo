#include "queryUser.h"
#include "data.h"
#include "security_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int user_verify_login(MYSQL *db, const char *username, const char *password,
                      unsigned long long *out_user_id, int *out_role_id) {
    if (!db || !username || !password || !out_user_id || !out_role_id) return -1;

    const char *sql = "SELECT id, password, salt, role_id FROM users WHERE BINARY username = ?";
    StmtRows *rows = stmt_select(db, sql, "s", username);
    if (!rows) return -1;
    if (rows->count == 0) {
        free_stmt_rows(rows);
        return 1; /* User not found */
    }

    char *id_str = rows->data[0][0];
    char *stored_hash = rows->data[0][1];
    char *stored_salt = rows->data[0][2];
    char *role_str = rows->data[0][3];

    int verified = 0;
    if (stored_salt && strlen(stored_salt) > 0) {
        verified = verify_password(password, stored_salt, stored_hash);
    } else {
        /* Fallback cho tài khoản chưa có salt (backward compatibility) */
        verified = (strcmp(stored_hash, password) == 0);
    }

    if (!verified) {
        free_stmt_rows(rows);
        return 1; /* Password incorrect */
    }

    *out_user_id = strtoull(id_str, NULL, 10);
    *out_role_id = atoi(role_str);
    free_stmt_rows(rows);
    return 0;
}

int user_register(MYSQL *db, const char *name, const char *username, const char *password, int role_id) {
    if (!db || !name || !username || !password) return -1;

    int exists = 0;
    if (stmt_get_int(db, &exists, "SELECT COUNT(*) FROM users WHERE username = ?", "s", username) != 0) {
        return -1;
    }
    if (exists > 0) {
        return 0; /* Username already exists */
    }

    char salt[SALT_HEX_LEN] = {0};
    char hash[HASH_HEX_LEN] = {0};
    generate_salt(salt, sizeof(salt));
    hash_password(password, salt, hash);

    const char *sql = "INSERT INTO users (name, username, password, salt, role_id, balance) VALUES (?, ?, ?, ?, ?, 500000.00)";
    int affected = stmt_execute(db, sql, "ssssi", name, username, hash, salt, role_id);
    return (affected == 1) ? 1 : -1;
}

int user_change_password(MYSQL *db, unsigned long long user_id,
                         const char *old_password, const char *new_password) {
    if (!db || !old_password || !new_password) return -1;

    const char *sel_sql = "SELECT password, salt FROM users WHERE id = ?";
    StmtRows *rows = stmt_select(db, sel_sql, "u", user_id);
    if (!rows) return -1;
    if (rows->count == 0) {
        free_stmt_rows(rows);
        return 0;
    }

    char *stored_hash = rows->data[0][0];
    char *stored_salt = rows->data[0][1];

    int verified = 0;
    if (stored_salt && strlen(stored_salt) > 0) {
        verified = verify_password(old_password, stored_salt, stored_hash);
    } else {
        verified = (strcmp(stored_hash, old_password) == 0);
    }
    free_stmt_rows(rows);

    if (!verified) {
        return 0; /* Wrong old password */
    }

    char new_salt[SALT_HEX_LEN] = {0};
    char new_hash[HASH_HEX_LEN] = {0};
    generate_salt(new_salt, sizeof(new_salt));
    hash_password(new_password, new_salt, new_hash);

    const char *upd_sql = "UPDATE users SET password = ?, salt = ? WHERE id = ?";
    int affected = stmt_execute(db, upd_sql, "ssu", new_hash, new_salt, user_id);
    return (affected == 1) ? 1 : -1;
}

int user_get_role(MYSQL *db, unsigned long long user_id, int *out_role_id) {
    if (!db || !out_role_id) return -1;
    int role = -1;
    if (stmt_get_int(db, &role, "SELECT role_id FROM users WHERE id = ?", "u", user_id) != 0) {
        return -1;
    }
    if (role >= 0 && role <= 2) {
        *out_role_id = role;
        return 0;
    }
    return -1;
}

int getUserIdByUsername(MYSQL *db, const char *username) {
    if (!db || !username) return -1;
    int id = -1;
    if (stmt_get_int(db, &id, "SELECT id FROM users WHERE username = ?", "s", username) != 0) {
        return -1;
    }
    return id;
}
