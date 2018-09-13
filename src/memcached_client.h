#ifndef __MEMCACHED_CLIENT_H
#define __MEMCACHED_CLIENT_H
#include <libmemcached/memcached.h>
#define MEM_SERVER "localhost"
#define MEM_PORT 11211
#define MEM_CONFIG 128

char* getValue(const char* key);
bool setValue(char* key, char* value);
bool existKey(const char* key);
bool deleteByKey(const char* key);
bool existKeys(const char **keys, const size_t *key_length, int n);
void getMultipleValues(const char** keys, const size_t* key_length, int n, char** values);
#endif
