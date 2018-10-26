#define _GNU_SOURCE
#ifndef __UNIONFS_H
#define __UNIONFS_H
#include <dirent.h>
#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"
#include <sys/stat.h>
#include <dlfcn.h>

#define MAX_PATH 1024
#define MAX_ITEMS 1024
#define MAX_VALUE_SIZE 1*1024*1024
#define PREFIX_WH ".wh"
#define MAX_LAYERS 128

//macros for system calls
#define DECLARE_SYS(FUNCTION,RETURN_TYPE,ARGS) \
    typedef RETURN_TYPE (*fufs_##FUNCTION) ARGS; \
    static fufs_##FUNCTION real_##FUNCTION = NULL;

#define INITIAL_SYS(FUNCTION) \
    if(!real_##FUNCTION){ \
        real_##FUNCTION = (fufs_##FUNCTION)dlsym(RTLD_NEXT, #FUNCTION); \
    }

#define RETURN_SYS(FUNCTION,ARGS) \
    real_##FUNCTION ARGS;

#define WRAPPER_FUFS(NAME,FUNCTION,ARGS) \
    fufs_##NAME##_impl(#FUNCTION,ARGS);


enum filetype{TYPE_FILE,TYPE_DIR,TYPE_LINK,TYPE_SOCK};
// declare original syscalls
/**
  typedef DIR* (*OPENDIR)(const char* name);
  typedef struct dirent* (*READDIR)(DIR* dirp);
  typedef int (*__XSTAT)(int ver, const char *path, struct stat *buf);
  typedef int (*OPEN)(const char *path, int oflag, ...);
  typedef int (*OPENAT)(int dirfd, const char *path, int oflag, ...);
  typedef int (*OPEN64)(const char *path, int oflag, ...);

  static OPENDIR real_opendir = NULL;
  static READDIR real_readdir = NULL;
  static __XSTAT real_xstat = NULL;
  static OPEN real_open = NULL;
  static OPENAT real_openat = NULL;
  static OPENAT real_open64= NULL;
 **/

// declaration ends
    DECLARE_SYS(opendir,DIR*,(const char* name))
    DECLARE_SYS(readdir,struct dirent*,(DIR* dirp))
    DECLARE_SYS(__xstat,int,(int ver, const char *path, struct stat *buf))
    DECLARE_SYS(open,int,(const char *path, int oflag, ...))
    DECLARE_SYS(openat,int,(int dirfd, const char *path, int oflag, ...))
    DECLARE_SYS(open64,int,(const char *path, int oflag, ...))
    DECLARE_SYS(unlink,int,(const char *path))
    DECLARE_SYS(unlinkat,int,(int dirfd, const char *path, int oflag))
    DECLARE_SYS(mkdir,int,(const char *path))
    DECLARE_SYS(mkdirat,int,(int dirfd, const char *path, mode_t mode))
    DECLARE_SYS(chdir,int,(const char *path))
    DECLARE_SYS(linkat,int,(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags))
    DECLARE_SYS(link,int,(const char *oldpath, const char *newpath))
    DECLARE_SYS(symlinkat,int,(const char *target, int newdirfd, const char *linkpath))
    DECLARE_SYS(symlink,int,(const char *target, const char *linkpath))
    DECLARE_SYS(creat64,int,(const char *path, mode_t mode))
    DECLARE_SYS(creat,int,(const char *path, mode_t mode))

    struct dirent_obj {
        struct dirent* dp;
        char abs_path[MAX_PATH];
        struct dirent_obj* next;
    };

struct dirent_layers_entry{
    char path[MAX_PATH];
    struct dirent_obj * data;
    size_t folder_num;
    char ** wh_masked;
    size_t wh_masked_num;
    size_t file_num;
};

enum hash_type{md5,sha256};
extern struct dirent_obj * darr;
DIR * getDirents(const char* name, struct dirent_obj** darr, size_t *num);
DIR * getDirentsWithName(const char* name, struct dirent_obj** darr, size_t *num, char **names);
struct dirent_layers_entry* getDirContent(const char* abs_path);
char ** getLayerPaths(size_t *num);
void filterMemDirents(const char* name, struct dirent_obj* darr, size_t num);
void deleteItemInChain(struct dirent_obj** darr, size_t num);
void deleteItemInChainByPointer(struct dirent_obj** darr, struct dirent_obj** curr);
void addItemToHead(struct dirent_obj** darr, struct dirent* item);
struct dirent * popItemFromHead(struct dirent_obj ** darr);
void clearItems(struct dirent_obj** darr);
char *struct2hash(void* pointer,enum hash_type type);
int get_abs_path(const char * path, char * abs_path, bool force);
int get_relative_path(const char * path, char * rel_path);
int get_abs_path_base(const char *base, const char *path, char * abs_path, bool force);
int get_relative_path_base(const char *base, const char *path, char * rel_path);
int get_relative_path_layer(const char *path, char * rel_path, char * layer_path);
int append_to_diff(const char* content);
bool is_file_type(const char *path,enum filetype t);
bool transWh2path(const char *name, const char *pre, char *tname);
int getParentWh(const char *abs_path);
bool xstat(const char *abs_path);
bool pathExcluded(const char *abs_path);

//fake union fs functions

#endif
