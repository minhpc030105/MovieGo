#include "protocol.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool read_fields(const char *payload, const char *types, Fields *fields) {
    size_t length = strlen(payload);
    if (length >= sizeof(fields->storage) || strlen(types) > 4) return false;
    memcpy(fields->storage, payload, length + 1);
    char *context = NULL;
    char *value = strtok_r(fields->storage, "\r\n", &context);
    for (size_t i = 0; types[i]; i++) {
        if (!value || strlen(value) > FIELD_LIMIT) return false;
        if (types[i] == 'i') {
            unsigned long id = strtoul(value, NULL, 10);
            if (strspn(value, "0123456789") != strlen(value) || id == 0 || id > INT_MAX) return false;
        }
        if (types[i] == 'r' && (strlen(value) != 1 || !strchr("012", *value))) return false;
        fields->value[i] = value;
        value = strtok_r(NULL, "\r\n", &context);
    }
    return value == NULL;
}

bool append_line(ResponseBody *body, const char *format, ...) {
    size_t left = body->size - body->used;
    if (left < 3) return false;
    char *dest = body->data + body->used;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(dest + 2, left - 2, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= left - 2) return false;
    memcpy(dest, "\r\n", 2);
    // Database text is one field, even if it contains a line break.
    for (int i = 0; i < n; i++) {
        if (dest[i + 2] == '\r' || dest[i + 2] == '\n') dest[i + 2] = ' ';
    }
    body->used += (size_t)n + 2;
    return true;
}
