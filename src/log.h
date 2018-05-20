#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

enum LOG_LEVEL{DEBUG, INFO, WARN, ERROR, FATAL};

#define log_debug(...) loginfo(DEBUG,__FILE__, __LINE__, __VA_ARGS__) 
#define log_info(...) loginfo(INFO,__FILE__, __LINE__, __VA_ARGS__) 
#define log_warn(...) loginfo(WARN,__FILE__, __LINE__, __VA_ARGS__) 
#define log_error(...) loginfo(ERROR,__FILE__, __LINE__, __VA_ARGS__) 
#define log_fatal(...) loginfo(FATAL,__FILE__, __LINE__, __VA_ARGS__) 

struct LOG{
    FILE *fp ;
    enum LOG_LEVEL l_level;
    bool l_switch;
    bool l_file;
};

struct LOG slog;

void set_log_fp(FILE *fp);

void loginfo(enum LOG_LEVEL level, const char * file, int line, char *fmt, ...);

#endif
