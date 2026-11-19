#include "data.h"
#include "queryUser.h"
#include <pthread.h>
#include <stdio.h>
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
    int role = -1;
    if (user_get_role(session->db, session->user_id, &role) == 0) {
        session->role_id = role;
        return 0;
    }
    clear_session(session);
    return AUTH_REQUIRED;
}

int handle_login(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "ss", &fields)) return INVALID_REQUEST;
    const char *username = fields.value[0];
    const char *password = fields.value[1];

    unsigned long long user_id = 0;
    int role_id = -1;
    int verify_res = user_verify_login(session->db, username, password, &user_id, &role_id);
    if (verify_res == -1) return DB_QUERY_ERROR;
    if (verify_res != 0) return LOGIN_FAIL;

    int code = LOGIN_FAIL;
    pthread_mutex_lock(&online_lock);
    Session *other = online;
    while (other && other->user_id != user_id) other = other->next_online;
    if (other) {
        code = LOGIN_ALREADY;
    } else {
        static const int success[] = {LOGIN_SUCCESS_ADMIN, LOGIN_SUCCESS_MANAGER, LOGIN_SUCCESS_USER};
        session->user_id = user_id;
        session->role_id = role_id;
        snprintf(session->username, sizeof(session->username), "%s", username);
        session->is_auth = true;
        session->next_online = online;
        online = session;
        code = success[session->role_id];
    }
    pthread_mutex_unlock(&online_lock);
    return code;
}

int handle_logout(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    if (*payload) return INVALID_REQUEST;
    clear_session(session);
    return LOGOUT_SUCCESS;
}

int handle_exit(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    if (*payload) return INVALID_REQUEST;
    session->closing = true;
    return LOGOUT_SUCCESS;
}

int handle_register(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "sss", &fields)) return INVALID_REQUEST;

    pthread_mutex_lock(&registration_lock);
    int res = user_register(session->db, fields.value[0], fields.value[1], fields.value[2], 2);
    pthread_mutex_unlock(&registration_lock);

    return (res == 1) ? REGISTER_SUCCESS : REGISTER_FAIL;
}

int handle_change_password(Session *session, const char *payload, char *resp_buf, size_t buf_size) {
    (void)resp_buf;
    (void)buf_size;
    Fields fields;
    if (!read_fields(payload, "ss", &fields)) return INVALID_REQUEST;

    int res = user_change_password(session->db, session->user_id, fields.value[0], fields.value[1]);
    return (res == 1) ? CHANGE_PASSWORD_SUCCESS : CHANGE_PASSWORD_FAIL;
}
