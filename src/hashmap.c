#include "hashmap.h"
#include "log.h"

void add_item_list(const char* key, struct list_head* phead){
    struct list_item * item = (struct list_item*)malloc(sizeof(struct list_item));
    strcpy(item->keys, key);
    LIST_INSERT_HEAD(&phead->head,item,pointers);
}

void delete_item_list(const char* key, struct list_head* phead){
    if(!LIST_EMPTY(&phead->head)){
        struct list_item* pitem;
        LIST_FOREACH(pitem, &phead->head, pointers){
            if(strcmp(pitem->keys, key) == 0 ){
                LIST_REMOVE(pitem,pointers);
                free(pitem);
            }
        }
    }
}

bool find_item_list(const char* key, struct list_head* phead){
    if(!LIST_EMPTY(&phead->head)){
        struct list_item* pitem;
        LIST_FOREACH(pitem, &phead->head, pointers){
            if(strcmp(pitem->keys, key) == 0){
                return true;
            }else{
                continue;
            }
        }
    }
    return false;
}

void clear_item_list(struct list_head* phead){
    while(!LIST_EMPTY(&phead->head)){
        struct list_item* pitem = LIST_FIRST(&phead->head);
        LIST_REMOVE(pitem, pointers);
        free(pitem);
    }
}

bool is_empty_list(struct list_head* phead){
    if(phead == NULL){
        return true;
    }
    return LIST_EMPTY(&phead->head);
}

hmap_t* create_hmap(size_t h_max_size){
    hmap_t* pmap = (hmap_t*)malloc(sizeof(hmap_t));
    pmap->h_max_size = h_max_size;
    memset(&pmap->h_map,0,sizeof(pmap->h_map));
    hcreate_r(h_max_size,&pmap->h_map);
    LIST_INIT(&pmap->head.head);
    return pmap;
}

void destroy_hmap(hmap_t* pmap){
    hdestroy_r(&pmap->h_map);
    clear_item_list(&pmap->head);
    free(pmap);
    pmap = NULL;
}

int add_item_hmap(hmap_t* pmap, char* key, void* data){
    unsigned n = 0;
    ENTRY e, *ep;
    e.key = key;
    e.data = data;
    n = hsearch_r(e, ENTER, &ep, &pmap->h_map);
    if(n){
        add_item_list(key,&pmap->head);
    }
    return n;
}

void* get_item_hmap(hmap_t* pmap, char* key){
    if(!pmap){
      return NULL;
    }
    ENTRY e, *ep;
    e.key = key;
    if(hsearch_r(e, FIND, &ep, &pmap->h_map)){
        return ep->data;
    }else{
        return NULL;
    }
}

void delete_item_hmap(hmap_t* pmap, char* key){
  if(pmap){
    ENTRY e, *ep;
    e.key = key;
    if(hsearch_r(e, FIND, &ep, &pmap->h_map)){
        delete_item_list(key,&pmap->head);
    }
  }
}

bool contain_item_hmap(hmap_t* pmap, char* key){
    if(!pmap){
      return false;
    }
    ENTRY e, *ep;
    e.key = key;
    if(hsearch_r(e,FIND,&ep,&pmap->h_map)){
        if(find_item_list(key, &pmap->head)){
            return true;
        }else{
            return false;
        }
    }else{
        return false;
    }
}

bool is_empty_hmap(hmap_t* pmap){
    if(pmap == NULL || &pmap->head == NULL){
        return true;
    }
    return is_empty_list(&pmap->head);
}

/*
int main(void)
{
    char key[64] = "lever1";
    char data[64] = "data";

    hmap_t* pmap = create_hmap(64);
    add_item_hmap(pmap, key, data);
    log_debug("key: %s => value: %s",key,(char*)get_item_hmap(pmap, key));
    return 0;
}
*/
