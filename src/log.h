#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>

enum LOG_LEVEL{DEBUG, INFO, WARN, ERROR, FATAL};

#define log_debug(...) loginfo(DEBUG,__FILE__, __LINE__, __VA_ARGS__) 
#define log_info(...) loginfo(INFO,__FILE__, __LINE__, __VA_ARGS__) 
#define log_warn(...) loginfo(WARN,__FILE__, __LINE__, __VA_ARGS__) 
#define log_error(...) loginfo(ERROR,__FILE__, __LINE__, __VA_ARGS__) 
#define log_fatal(...) loginfo(FATAL,__FILE__, __LINE__, __VA_ARGS__) 

#define __LOG_SWITCH
#define __LOG_TERMINAL
#define __LOG_LEVEL DEBUG

struct LOG{
    FILE *fp ;
};

struct LOG slog;

extern pthread_mutex_t LOG_MUTEX;

void set_log_fp(FILE *fp);

void loginfo(enum LOG_LEVEL level, const char * file, int line, char *fmt, ...);

#endif
