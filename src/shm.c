#include "shm_rcv.h"
#include "shm_snd.h"

int main(int argc, char const* argv[])
{
    hmap_t* pmap = create_hmap(TOP_HMAP_SIZE);
    char top_key[64] = "lever1";
    char second_key[64] = "lever2";
    char third_key[64] = "lever3";
    char data[64] = "data";
    update_complex_hmap(pmap,top_key, second_key,third_key, data);
    log_debug("data:%s\n",(char*)get_complex_hmap(pmap,top_key,second_key,third_key));

    shm_snd(pmap);

    log_debug("--------------pmap send finished -------------");

    hmap_t* pnmap = shm_get();
    log_debug("data:%s\n",(char*)get_complex_hmap(pnmap,top_key,second_key,third_key));
 
  return 0;
}
