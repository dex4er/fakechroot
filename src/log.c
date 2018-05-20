#include "log.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static const char *levels[] = {"DEBUG","INFO","WARN","ERROR","FATAL"};

const int BUFF_TIME_SIZE = 32;
const int BUFF_DATA_SIZE = 4096; 

void set_log_fp(FILE *fp){
    slog.fp = fp;
}

void loginfo(enum LOG_LEVEL level, const char * file, int line, char *fmt, ...){
    char *log_level= getenv("__LOG_LEVEL");
    char *log_switch = getenv("__LOG_SWITCH");

    if(log_switch){
      slog.l_switch = true;
    }else{
      slog.l_switch = false;
    }

    if(log_level && strlen(log_level) == 1){
      int l = log_level[0]-'0';
      slog.l_level = (enum LOG_LEVEL) l;
    }else{
      slog.l_level = INFO;
    }

    if(slog.l_switch){
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char buff_time[BUFF_TIME_SIZE];
    char buff_data[BUFF_DATA_SIZE];
    va_list args;

    buff_time[strftime(buff_time,sizeof(buff_time),"%Y-%m-%d %H:%M:%S",lt)] = '\0';
    sprintf(buff_data,"{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\",\"line\":%d,\"msg\":\"",buff_time,levels[level],file,line);
    va_start(args, fmt);
    vsprintf(buff_data+strlen(buff_data),fmt,args);
    va_end(args);
    sprintf(buff_data+strlen(buff_data),"\"%s\n","}");

    if(level >= slog.l_level){
         fprintf(stdout,"%s",buff_data);
         fflush(stdout);
        
    }

    if(slog.fp){
        fprintf(slog.fp,"%s",buff_data);
        fflush(slog.fp);
    }
    }
}

/*
int main(int argc, char const* argv[])
{
    set_log_terminal(true);
    error("%s","hahahaha");
    return 0;
}
*/
