#ifndef __UNIONFS_H
#define __UNIONFS_H 

#include <stddef.h>
#include <stdbool.h>
#define PATH_MAX_LEVEL 32
#define PATH_MAX_LENGTH 2048
int create_hardlink(const char * src);
int get_all_parents(const char * path, char ** parents, int * lengths, size_t *n);
bool b_parent_delete(int n, ...);
#endif
