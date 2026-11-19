#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>

void logMessage(int connfd, const char* direction, const char* message);
void logResult(int connfd, const char* direction, int result);
void initLogger();
void closeLogger();

#endif
