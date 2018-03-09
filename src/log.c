#include "log.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static const char *levels[] = {"DEBUG","INFO","WARN","ERROR","FATAL"};

const int BUFF_TIME_SIZE = 32;
const int BUFF_DATA_SIZE = 4096; 

pthread_mutex_t LOG_MUTEX = PTHREAD_MUTEX_INITIALIZER;

void set_log_fp(FILE *fp){
    slog.fp = fp;
} void loginfo(enum LOG_LEVEL level, const char * file, int line, char *fmt, ...){
    #ifdef __LOG_SWITCH
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char buff_time[BUFF_TIME_SIZE];
    char buff_data[BUFF_DATA_SIZE];
    va_list args;

    pthread_mutex_lock(&LOG_MUTEX);

    buff_time[strftime(buff_time,sizeof(buff_time),"%Y-%m-%d %H:%M:%S",lt)] = '\0';
    sprintf(buff_data,"{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\",\"line\":%d,\"msg\":\"",buff_time,levels[level],file,line);
    va_start(args, fmt);
    vsprintf(buff_data+strlen(buff_data),fmt,args);
    va_end(args);
    sprintf(buff_data+strlen(buff_data),"\"%s\n","}");

    #ifdef __LOG_TERMINAL
    if(level >= __LOG_LEVEL){
         fprintf(stdout,"%s",buff_data);
         fflush(stdout);
        
    }
    #endif

    #ifdef __LOG_FILE
    if(slog.fp){
        fprintf(slog.fp,"%s",buff_data);
        fflush(slog.fp);
    }
    #endif

    pthread_mutex_unlock(&LOG_MUTEX);

    #endif
}

/*
int main(int argc, char const* argv[])
{
    set_log_terminal(true);
    error("%s","hahahaha");
    return 0;
}
*/
