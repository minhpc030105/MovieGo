#include "logger.h"
#include <pthread.h>
#include "../../lib/socket/socket.h"

static FILE* log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void initLogger() {
    log_file = fopen("server_messages.log", "a");
    if (log_file == NULL) {
        fprintf(stderr, "Warning: Cannot open log file\n");
    }
}

void closeLogger() {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}

void logMessage(int connfd, const char* direction, const char* message) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    char clean_msg[512];
    strncpy(clean_msg, message, sizeof(clean_msg) - 1);
    clean_msg[sizeof(clean_msg) - 1] = '\0';
    
    for (int i = 0; clean_msg[i] != '\0'; i++) {
        if (clean_msg[i] == '\r' || clean_msg[i] == '\n') {
            clean_msg[i] = ' ';
        }
    }
    
    if (log_file != NULL) {
        fprintf(log_file, "[%s] [FD:%d] [%s] %s\n", 
                timestamp, connfd, direction, clean_msg);
        fflush(log_file);
    }
    
    printf("[%s] [FD:%d] [%s] %s\n", 
           timestamp, connfd, direction, clean_msg);
    
    pthread_mutex_unlock(&log_mutex);
}

void logResult(int connfd, const char* direction, int result) {
    char msg[64];
    snprintf(msg, sizeof(msg), "RESULT=%d", result);
    logMessage(connfd, direction, msg);
}

void sendMessageLogged(int connfd, char *message) {
    logMessage(connfd, "SEND", message);
    sendMessage(connfd, message);
}

void sendResultLogged(int connfd, int result) {
    logResult(connfd, "SEND", result);
    sendResult(connfd, result);
}
