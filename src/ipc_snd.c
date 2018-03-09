#include "ipc_snd.h"

bool snd_data_msg(int msqid, struct DATA_MSG* msg){
    int sndlength = sizeof(struct DATA_MSG)-sizeof(long);
    int flag = msgsnd(msqid, msg, sndlength, 0);
    if(flag >= 0){
        return true;
    }
    printf("errno:%s",strerror(errno));
    return false;
}


void* system_snd_thread(void* arg){
    ;
}


void* data_snd_thread(void* arg){
    struct data_thread_info* info = (struct data_thread_info*)arg;
    snd_data_msg(info->pmid, &info->msg);
}


