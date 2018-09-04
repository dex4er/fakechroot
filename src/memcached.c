#include "memcached_client.h"
#include "log.h"

int main(int argc, char const* argv[])
{
    if(argc > 1){
        char* key = "container:touch:allow";
        if(strcmp(argv[1],"set") == 0){
            log_debug("setvalue: key=> container:touch:allow, value=>/tmp, result: %d",setValue(key,"/tmp"));
        }
        if(strcmp(argv[1],"set2") == 0){
            log_debug("setvalue: key=> container:touch:allow, value=>/home/jason, result: %d",setValue(key,"/home/jason"));
        }
        if(strcmp(argv[1],"del") == 0){
            log_debug("deleteByKey: key=> container:touch:allow, result: %d",deleteByKey(key));
        }
        if(strcmp(argv[1],"get") == 0){
            log_debug("get: key=> container:touch:allow, result: %s",getValue(key));
        }
    }else{
        char *keys[] = {"fudge","son","food"};
        size_t key_length[] = {5,3,4};
        existKeys(keys, key_length, 3);
    }
    return 0;
}
