#ifndef __MEMCACHED_CLIENT_H
#define __MEMCACHED_CLIENT_H
#include <libmemcached/memcached.h>
#define MEM_SERVER "localhost"
#define MEM_PORT 11211

char* getValue(const char* key);
bool setValue(char* key, char* value);
bool existKey(const char* key);
bool deleteByKey(const char* key);
#endif
