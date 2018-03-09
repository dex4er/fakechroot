#include "shm_rcv.h"

void shm_get(void* data, size_t size){
  key_t key = ftok(SHM_FILE, 0x01);
  int shmid;
  if((shmid = shmget(key, size, IPC_CREAT|0664)) < 0){
    log_fatal("shm_get %s failed", SHM_FILE);
    exit(EXIT_FAILURE);
  }

  char* shm;
  if((shm = shmat(shmid, NULL< SHM_RDONLY)) == (char*) -1){
    log_fatal("shmat %d failed", shmid);
    exit(EXIT_FAILURE);
  }
  data = (void*)shm;
}
