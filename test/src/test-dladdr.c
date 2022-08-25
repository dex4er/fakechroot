#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  Dl_info info;
  memset(&info, 0xfe, sizeof(info)); /* fill with inaccessible address */
  int ret = dladdr(NULL, &info);
  printf("%ld\n", ret);
  return 0;
}
