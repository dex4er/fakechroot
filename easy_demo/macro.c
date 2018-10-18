#define _GNU_SOURCE
#include <dirent.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dlfcn.h>

#define DECLARE_SYS(FUNCTION,RETURN_TYPE,ARGS) \
    typedef RETURN_TYPE (*fufs_##FUNCTION) ARGS; \
    static fufs_##FUNCTION real_##FUNCTION = NULL;
#define INITIAL_SYS(FUNCTION) \
{\
    if(!real_##FUNCTION){ \
        real_##FUNCTION = (fufs_##FUNCTION)dlsym(RTLD_NEXT, #FUNCTION); \
    }\
}

#define RETURN_SYS(FUNCTION,ARGS) \
    real_##FUNCTION ARGS;

#define WRAPPER_FUFS(NAME,FUNCTION,ARGS) \
    fufs_##NAME##_impl(#FUNCTION,ARGS);

    DECLARE_SYS(opendir,DIR*,(const char* name))
    DECLARE_SYS(__xstat,int,(int ver, const char *path, struct stat *buf))
    DECLARE_SYS(open,int,(const char *path, int oflag, ...))
    DECLARE_SYS(openat,int,(int dirfd, const char *path, int oflag, ...))
DECLARE_SYS(open64,int,(const char *path, int oflag, ...))

    int main(){
        INITIAL_SYS(opendir)
            const char * ret = "/home/jason";
        RETURN_SYS(opendir,(ret))
        return 0;
    }


