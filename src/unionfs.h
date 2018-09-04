#ifndef __UNIONFS_H
#define __UNIONFS_H 

#define PATH_MAX_LEVEL 32
int create_hardlink(const char * src);
int get_all_parents(const char * path, char ** parents, int * lengths, int *n);
#endif
