#define _GNU_SOURCE
#ifndef __UNIONFS_H
#define __UNIONFS_H
#include <dirent.h>
#include <stddef.h>
#include <stdbool.h>
#include "hashmap.h"
#include <sys/stat.h>

#define MAX_PATH 1024
#define MAX_ITEMS 1024
#define MAX_VALUE_SIZE 1*1024*1024
#define PREFIX_WH ".wh"
#define MAX_LAYERS 128

enum filetype{TYPE_FILE,TYPE_DIR,TYPE_LINK,TYPE_SOCK};
typedef DIR* (*OPENDIR)(const char* name);
typedef struct dirent* (*READDIR)(DIR* dirp);
typedef int (*__XSTAT)(int ver, const char *path, struct stat *buf);
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
static OPENDIR real_opendir = NULL;
static READDIR real_readdir = NULL;
static __XSTAT real_xstat = NULL;
extern struct dirent_obj * darr;
DIR * getDirents(const char* name, struct dirent_obj** darr, size_t *num);
DIR * getDirentsWithName(const char* name, struct dirent_obj** darr, size_t *num, char **names);
struct dirent_layers_entry* getDirContent(const char* abs_path);
struct dirent_obj* getDirContentLayers(const char* abs_path);
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
#endif
