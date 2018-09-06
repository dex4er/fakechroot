#ifndef __HMAPPRIV_H
#define __HMAPPRIV_H
#include "hashmap.h"
#include <unistd.h>
#include <sys/types.h>

#define PNAME_SIZE 48
#define PATH_MAX_LENGTH 1024

struct ProcessInfo{
    pid_t pid;
    pid_t ppid;
    pid_t groupid;
    char pname[PNAME_SIZE];
};

bool hmap_priv_check(hmap_t* pmap, const char* abs_path);
void mem_priv_check_v(int n, ...);
bool rt_mem_check(int n, char** rt_paths, ...);
#endif




