#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>

pid_t getpid(void){
  printf("This is the fake getpid\n");
  pid_t a=-1;
  return a;
}


