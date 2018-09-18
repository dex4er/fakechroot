#define _GNU_SOURCE
#ifndef __UNIONFS_H
#define __UNIONFS_H
#include <dirent.h>
#include <stddef.h>

#define MAX_PATH 1024
#define MAX_VALUE_SIZE 1*1024*1024
typedef DIR* (*OPENDIR)(const char* name);
typedef struct dirent* (*READDIR)(DIR* dirp);
struct dirent_obj {
    struct dirent* dp;
    struct dirent_obj* next;
};

enum hash_type{md5,sha256};
static OPENDIR real_opendir = NULL;
static READDIR real_readdir = NULL;
extern struct dirent_obj * darr;
DIR * getDirents(const char* name, struct dirent_obj** darr, size_t *num);
DIR * getDirentsWithName(const char* name, struct dirent_obj** darr, size_t *num, char **names);
void filterMemDirents(const char* name, struct dirent_obj* darr, size_t num);
void deleteItemInChain(struct dirent_obj** darr, size_t num);
void addItemToHead(struct dirent_obj** darr, struct dirent* item);
struct dirent * popItemFromHead(struct dirent_obj ** darr);
void clearItems(struct dirent_obj** darr);
char *struct2hash(void* pointer,enum hash_type type);
#endif
