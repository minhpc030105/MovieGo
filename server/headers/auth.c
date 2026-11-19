#include "data.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Only authenticated connections are linked. Each worker owns its Session until cleanup.
static pthread_mutex_t online_lock = PTHREAD_MUTEX_INITIALIZER;
static Session *online;
static pthread_mutex_t registration_lock = PTHREAD_MUTEX_INITIALIZER;

void clear_session(Session *session) {
    pthread_mutex_lock(&online_lock);
    Session **link = &online;
    while (*link && *link != session) link = &(*link)->next_online;
    if (*link) *link = session->next_online;
    session->next_online = NULL;
    session->is_auth = false;
    session->user_id = 0;
    session->role_id = -1;
    session->username[0] = '\0';
    pthread_mutex_unlock(&online_lock);
}

int refresh_role(Session *session) {
    MYSQL_RES *rows = select_rows(session->db, "SELECT role_id FROM users WHERE id=%llu", session->user_id);
    if (!rows) return DB_QUERY_ERROR;
    MYSQL_ROW row = mysql_fetch_row(rows);
    bool valid = row && strlen(row[0]) == 1 && strchr("012", row[0][0]);
    if (valid) session->role_id = row[0][0] - '0';
    mysql_free_result(rows);
    if (valid) return 0;
    clear_session(session);
    return AUTH_REQUIRED;
}

int handle_login(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    EscapedField username;
    if (!read_fields(payload, "ss", &fields)) return INVALID_REQUEST;
    if (!escape_field(session->db, fields.value[0], &username)) return LOGIN_FAIL;
    MYSQL_RES *rows = select_rows(session->db,
        "SELECT id, username, password, role_id FROM users WHERE BINARY username='%s'", username.text);
    if (!rows) return DB_QUERY_ERROR;
    MYSQL_ROW row = mysql_fetch_row(rows);
    int code = LOGIN_FAIL;
    if (row && strcmp(row[2], fields.value[1]) == 0 && strlen(row[3]) == 1 && strchr("012", row[3][0])) {
        unsigned long long id = strtoull(row[0], NULL, 10);
        pthread_mutex_lock(&online_lock);
        Session *other = online;
        while (other && other->user_id != id) other = other->next_online;
        if (other) {
            code = LOGIN_ALREADY;
        } else {
            static const int success[] = {LOGIN_SUCCESS_ADMIN, LOGIN_SUCCESS_MANAGER, LOGIN_SUCCESS_USER};
            session->user_id = id;
            session->role_id = row[3][0] - '0';
            strcpy(session->username, fields.value[0]);
            session->is_auth = true;
            session->next_online = online;
            online = session;
            code = success[session->role_id];
        }
        pthread_mutex_unlock(&online_lock);
    }
    mysql_free_result(rows);
    return code;
}

int handle_logout(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    clear_session(session);
    return LOGOUT_SUCCESS;
}

int handle_exit(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    if (*payload) return INVALID_REQUEST;
    session->closing = true;
    return LOGOUT_SUCCESS;
}

int handle_register(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    EscapedField name, username, password;
    if (!read_fields(payload, "sss", &fields)) return INVALID_REQUEST;
    if (!escape_field(session->db, fields.value[0], &name) ||
        !escape_field(session->db, fields.value[1], &username) ||
        !escape_field(session->db, fields.value[2], &password)) return REGISTER_FAIL;
    // The existing schema has no UNIQUE index on username.
    pthread_mutex_lock(&registration_lock);
    bool ok = execute_update(session->db,
        "INSERT INTO users(name,username,password,role_id) SELECT '%s','%s','%s',2 "
        "WHERE NOT EXISTS (SELECT 1 FROM users WHERE username='%s')",
        name.text, username.text, password.text, username.text) && mysql_affected_rows(session->db) == 1;
    pthread_mutex_unlock(&registration_lock);
    return ok ? REGISTER_SUCCESS : REGISTER_FAIL;
}

int handle_change_password(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    Fields fields;
    if (!read_fields(payload, "ss", &fields)) return INVALID_REQUEST;
    EscapedField old_password, new_password;
    if (!escape_field(session->db, fields.value[0], &old_password) ||
        !escape_field(session->db, fields.value[1], &new_password)) return CHANGE_PASSWORD_FAIL;
    bool ok = execute_update(session->db,
        "UPDATE users SET password='%s' WHERE id=%llu AND BINARY password='%s'",
        new_password.text, session->user_id, old_password.text);
    return ok && mysql_affected_rows(session->db) == 1 ? CHANGE_PASSWORD_SUCCESS : CHANGE_PASSWORD_FAIL;
}
