#ifndef __IPC_H
#define __IPC_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include "hashmap.h"

#define MSG_MAX 1024*3 
#define MSG_FILE_NAME "/tmp/msg"

enum M_TYPE{SYSTEM = 1,DATA};

struct DATA_MSG{
    long mtype;
    long msgid;
    long msgcount;
    long seqid;
    char top_key[KEY_SIZE];
    char second_key[KEY_SIZE];
    char third_key[KEY_SIZE];
    char data[MSG_MAX];
};

struct SYS_MSG{
    long mtype;
    long msgid;
    long msgcount;
    long seqid;
    int systype;
    char data[MSG_MAX];
};

struct data_thread_info{
    int pmid;
    hmap_t* pmap;
    struct DATA_MSG msg;
};
#endif
