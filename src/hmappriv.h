#ifndef __HMAPPRIV_H
#define __HMAPPRIV_H
#include "hashmap.h"
#include <unistd.h>
#include <sys/types.h>

#define PNAME_SIZE 48

struct ProcessInfo{
    pid_t pid;
    pid_t ppid;
    pid_t groupid;
    char pname[PNAME_SIZE];
};

bool hmap_priv_check(hmap_t* pmap, const char* abs_path);
bool mem_priv_check(const char* abs_path);
#endif




