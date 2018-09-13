#include "memcached_client.h"
#include "log.h"

memcached_st* init_mem_server()
{
    memcached_server_st* servers = NULL;
    memcached_return rc;
    memcached_st* memc = memcached_create(NULL);
    servers = memcached_server_list_append(servers, MEM_SERVER, MEM_PORT, &rc);
    rc = memcached_server_push(memc, servers);
    if (rc != MEMCACHED_SUCCESS) {
        log_fatal("connect memcached server failed with address: %s, port: %d, error: %s", MEM_SERVER, MEM_PORT, memcached_strerror(memc, rc));
        exit(EXIT_FAILURE);
    }
    return memc;
}

memcached_st* init_mem_unix_server()
{
    char* pid = getenv("MEMCACHED_PID");
    if (!pid) {
        log_fatal("can't connect to memcached server because of no env virable 'MEMCACHED_PID' set");
        exit(EXIT_FAILURE);
    }

    memcached_st* memc = memcached_create(NULL);
    memcached_return rc = memcached_server_add_unix_socket(memc, pid);
    if (rc != MEMCACHED_SUCCESS) {
        log_fatal("connect memcached server failed with socket: %s", pid);
        exit(EXIT_FAILURE);
    }
    return memc;
}

char* getValue(const char* key)
{
    memcached_st* memc = init_mem_unix_server();
    size_t return_value_length;
    uint32_t flags;
    memcached_return rc;
    return memcached_get(memc, key, strlen(key), &return_value_length, &flags, &rc);
}

bool setValue(char* key, char* value)
{
    memcached_st* memc = init_mem_unix_server();
    memcached_return rc;
    rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);
    if (rc == MEMCACHED_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

bool existKey(const char* key)
{
    memcached_st* memc = init_mem_unix_server();
    memcached_return rc;
    rc = memcached_exist(memc, key, strlen(key));
    if (rc == MEMCACHED_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

bool deleteByKey(const char* key)
{
    memcached_st* memc = init_mem_unix_server();
    memcached_return rc;
    rc = memcached_delete(memc, key, strlen(key), (time_t)0);
    if (rc == MEMCACHED_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

bool existKeys(const char** keys, const size_t* key_length, int n)
{
    memcached_st* memc = init_mem_unix_server();
    memcached_return_t rc;

    char return_key[MEMCACHED_MAX_KEY];
    size_t return_key_length;
    char* return_value;
    size_t return_value_length;
    uint32_t flags;

    rc = memcached_mget(memc, keys, key_length, n);
    while ((return_value = memcached_fetch(memc, return_key, &return_key_length, &return_value_length, &flags, &rc))) {
        log_debug("return value %s", return_value);
        if ((return_value != NULL) && (*return_value != '\0')) {
            free(return_value);
            return true;
        }
        free(return_value);
    }
    return false;
}

void getMultipleValues(const char** keys, const size_t* key_length, int n, char** values)
{
    memcached_st* memc = init_mem_unix_server();
    memcached_return_t rc;

    char return_key[MEMCACHED_MAX_KEY];
    size_t return_key_length;
    char* return_value;
    size_t return_value_length;
    uint32_t flags;

    rc = memcached_mget(memc, keys, key_length, n);
    int i = 0;
    while ((return_value = memcached_fetch(memc, return_key, &return_key_length, &return_value_length, &flags, &rc))) {
        log_debug("return value %s", return_value);
        if (return_value != NULL) {
            strcpy(values[i], return_value);
            free(return_value);
        }
    }
}
