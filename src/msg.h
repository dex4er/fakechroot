#ifndef __MSG_H
#define __MSG_H 
#include "log.h"
#include <msgpack.h>
#include <stdbool.h>
#include "hashmap.h"

void* serialStr(char* input, size_t insize, size_t* outsize);
char* deserialStr(void* input, size_t insize, size_t* outsize);

void* serialHmap(hmap_t* hmap, size_t* count, size_t* outsize);
hmap_t* deserialHmap(void* input, size_t insize, size_t count);

#endif /* ifndef __MSG_H */
