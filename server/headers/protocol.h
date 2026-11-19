#ifndef MOVIEGO_PROTOCOL_H
#define MOVIEGO_PROTOCOL_H
#include "function.h"

#define FIELD_LIMIT 255
typedef struct {
    char storage[4 * (FIELD_LIMIT + 2) + 1];
    char *value[4];
} Fields;

// 's' = text, 'i' = positive decimal id, 'r' = role 0..2.
bool read_fields(const char *payload, const char *types, Fields *fields);

typedef struct {
    char *data;
    size_t size;
    size_t used;
} ResponseBody;
bool append_line(ResponseBody *body, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
#endif
