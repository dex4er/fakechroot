#ifndef __HASH_MAP_H
#define __HASH_MAP_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include "stdbool.h" 
#include <pthread.h>

#define __USE_GNU 1
#include <search.h>

#define KEY_SIZE 256

struct list_item{
    char keys[KEY_SIZE];
    LIST_ENTRY(list_item) pointers;
};

struct list_head{
    LIST_HEAD(linked_keys, list_item) head;
};

void add_item_list(const char* key, struct list_head* phead);
void delete_item_list(const char* key, struct list_head* phead);
bool find_item_list(const char* key, struct list_head* phead);
void clear_item_list(struct list_head* phead);
bool is_empty_list(struct list_head* phead);

typedef struct hmap_t{
    struct hsearch_data h_map;
    size_t h_max_size;
    struct list_head head; //list is used for storing the keys already included in maps
}hmap_t;

hmap_t* create_hmap(size_t h_max_size);
void destroy_hmap(hmap_t* pmap);
int add_item_hmap(hmap_t* pmap, char* key, void* data);
void* get_item_hmap(hmap_t* pmap, char* key);
void delete_item_hmap(hmap_t* pmap,char* key);
bool contain_item_hmap(hmap_t* pmap,char* key);
bool is_empty_hmap(hmap_t* pmap);
#endif
