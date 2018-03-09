#ifndef __SHM_H
#define __SHM_H
#include "hashmap.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "log.h"

#define SHM_FILE "/tmp/shm"
#define SHM_SIZE 4*TOP_HMAP_SIZE*SECOND_HMAP_SIZE*THIRD_HMAP_SIZE
#endif
