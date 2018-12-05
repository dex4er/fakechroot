#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

typedef int (*real_open_p)(const char *pathname, int flags, mode_t mode);


int open(const char *pathname, int flags, mode_t mode){
    real_open_p real_open;
    real_open = (real_open_p)dlsym(RTLD_NEXT,"open");
    printf("***************** intercepted open");
    return real_open(pathname, flags, mode);
}
