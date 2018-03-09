#include "ipc_rcv.h"
#include "log.h"

void rcv_data_msg(int msqid, long* msgid, long* msgcount, long* seqid,char* top_key,char* second_key,char* third_key,void* data){
    int recvlength = sizeof(struct DATA_MSG)-sizeof(long);
    struct DATA_MSG msg;
    memset(&msg,0x00,sizeof(struct DATA_MSG));
    int flag = msgrcv(msqid,&msg,recvlength,DATA,IPC_NOWAIT);
    if(flag >= 0){
       *msgid = msg.msgid;
       *msgcount = msg.msgcount;
       *seqid = msg.seqid;
       strcpy(top_key,msg.top_key);
       strcpy(second_key,msg.second_key);
       strcpy(third_key, msg.third_key);
       strcpy(data,(void*)msg.data);
    }else{
        *msgid = -1;
        *msgcount = -1;
        *seqid = -1;
        top_key = NULL;
        second_key = NULL;
        third_key = NULL;
        data = NULL;
    }
}


void* data_rcv_thread(void* arg){
    log_debug("data_rcv_thread started....");
    struct data_thread_info* info = (struct data_thread_info*)arg;
    while(1){
        rcv_data_msg(info->pmid,&info->msg.msgid,&info->msg.msgcount,&info->msg.seqid,info->msg.top_key,info->msg.second_key,info->msg.third_key,info->msg.data);
        if(info->msg.msgid != -1){
            log_debug("update hashmap: [top_key:%s],[second_key:%s],[third_key:%s],[data:%s] \n",info->msg.top_key,info->msg.second_key,info->msg.third_key,info->msg.data);
            update_complex_hmap(info->pmap,info->msg.top_key, info->msg.second_key, info->msg.third_key, info->msg.data);
        }
        sleep(1);
    }
}

void data_rcv(hmap_t* pmap){
    key_t key = ftok(MSG_FILE_NAME, 0x01);
    int msqid_rcv = msgget(key, 0);
    struct data_thread_info info;
    info.pmid = msqid_rcv;
    rcv_data_msg(info.pmid,&info.msg.msgid,&info.msg.msgcount,&info.msg.seqid,info.msg.top_key,info.msg.second_key,info.msg.third_key,info.msg.data);
    if(info.msg.msgid != -1){
       log_debug("update hashmap: [top_key:%s],[second_key:%s],[third_key:%s],[data:%s] \n",info.msg.top_key,info.msg.second_key,info.msg.third_key,info.msg.data);
       update_complex_hmap(pmap,info.msg.top_key, info.msg.second_key, info.msg.third_key, info.msg.data);
  }
}

void start_rcv_thread(hmap_t* pmap){
    pthread_t t_data_rcv;
    key_t key = ftok(MSG_FILE_NAME, 0x01);
    int msqid_rcv = msgget(key, 0);
    struct data_thread_info data_rcv;
    data_rcv.pmid = msqid_rcv;
    data_rcv.pmap = pmap;
    pthread_create(&t_data_rcv, NULL, data_rcv_thread, &data_rcv);
    pthread_join(t_data_rcv, NULL);
}




