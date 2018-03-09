#include "msg.h"
#include <string.h>

void* serialStr(char* input, size_t insize, size_t* outsize){
  msgpack_sbuffer* buf = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buf, msgpack_sbuffer_write);

  msgpack_pack_str(pk, insize);
  msgpack_pack_str_body(pk, input, insize);

  if(buf->size <= 0){
    log_fatal("msgpack pack %s failed",(char*)input);
    return NULL;
  }

  void* output = (void*)malloc(buf->size);
  memcpy(output, (void*)buf->data,buf->size);
  *outsize = buf->size;

  msgpack_sbuffer_free(buf);
  msgpack_packer_free(pk);

  return output;
}


char* deserialStr(void* input, size_t insize, size_t* outsize){
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  msgpack_unpack_return ret = msgpack_unpack_next(&msg, input, insize, NULL);

  if(ret != MSGPACK_UNPACK_SUCCESS ){
    log_fatal("msgpack unpack failed with code: %d ",ret);
    return NULL;
  }

  *outsize = msg.data.via.str.size;
  char* output = (char*)malloc(*outsize+1); 
  memcpy(output,msg.data.via.str.ptr,*outsize);
  output[*outsize] = '\0';
  return output;
}

void* serialHmap(hmap_t* hmap, size_t* count, size_t* outsize){
  if(hmap != NULL){
    msgpack_sbuffer* buf = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buf, msgpack_sbuffer_write);

    struct list_item* pitem;
    *count = 0;
    struct list_head* phead = &hmap->head;
   LIST_FOREACH(pitem, &phead->head, pointers){
     char* key = pitem->keys;
     void* data = get_item_hmap(hmap, key);

     //serial key
     msgpack_pack_str(pk, strlen(key));
     msgpack_pack_str_body(pk, key, strlen(key));
    
     //serial value
     msgpack_pack_str(pk,strlen((char*)data));
     msgpack_pack_str_body(pk, data, strlen((char*)data));
     
     if(buf->size <= 0){
       log_fatal("msgpack serialize key:%s => value: %s failed", key, (char*)data);
       return NULL;
     }

     (*count)++;
   }
  
   void* output = (void*)malloc(buf->size);
   memcpy(output, (void*)buf->data,buf->size);
   *outsize = buf->size;
     
   msgpack_sbuffer_free(buf);
   msgpack_packer_free(pk);

   return output;
  }
  return NULL;
}

hmap_t* deserialHmap(void* input, size_t insize, size_t count){
  msgpack_unpacker pac;
  msgpack_unpacker_init(&pac, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

  msgpack_unpacker_reserve_buffer(&pac, insize);
  memcpy(msgpack_unpacker_buffer(&pac), input, insize);
  msgpack_unpacker_buffer_consumed(&pac, insize);

  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);

  hmap_t* hmap = create_hmap(count);

  msgpack_unpack_return ret = MSGPACK_UNPACK_SUCCESS; 
  while( ret == MSGPACK_UNPACK_SUCCESS ){
    ret = msgpack_unpacker_next(&pac, &msg);
    if(ret != MSGPACK_UNPACK_SUCCESS){
      break;
    }
    size_t key_len = msg.data.via.str.size;
    char* key = (char*)malloc(key_len+1); 
    memcpy(key, msg.data.via.str.ptr, key_len);
    key[key_len] = '\0';

    ret = msgpack_unpacker_next(&pac, &msg);
    if(ret != MSGPACK_UNPACK_SUCCESS){
      break;
    }
    size_t data_len = msg.data.via.str.size;
    char* data = (char*)malloc(data_len+1);
    memcpy(data, msg.data.via.str.ptr, data_len);
    data[data_len] = '\0';
    add_item_hmap(hmap,key,(void*)data);

    //free(key);
    //free(data);
  }

  return hmap;
}

/*
int main(int argc, char *argv[])
{
  //char *test = "sdfsldkjldkjdf";
  //size_t length = 15;
  //void* output;
  //size_t outsize; 
  //output = serialStr((void*)test,length,&outsize);

  //size_t newsize;
  //void* result;
  //result = deserialStr(output,outsize,&newsize);
  //log_debug("%s", (char*)result);
  //return 0;

  hmap_t* hmap = create_hmap(3);
  add_item_hmap(hmap, "k1","v1");
  add_item_hmap(hmap, "k2","v2");
  add_item_hmap(hmap, "k3","v3");

  size_t count, outsize;
  void* data = serialHmap(hmap, &count, &outsize);

  hmap_t* new = deserialHmap(data, outsize, count);
  bool result = contain_item_hmap(new,"k2");
  log_debug("%s => %s", "k2", (char*)get_item_hmap(new, "k2"));
}
*/
