#include "ipc_rcv.h"
#include "ipc_snd.h"

int main(int argc, char const* argv[])
{
    pthread_t t_data_snd,t_data_rcv;
    key_t key = ftok("/tmp/msg", 0x01);
    int msqid_snd= msgget(key,IPC_CREAT|0600);
    int msqid_rcv= msgget(key,0);
    struct data_thread_info data_rcv, data_snd;
    data_rcv.pmid = msqid_rcv;
    data_snd.pmid = msqid_snd;

    data_snd.msg.mtype = DATA;
    data_snd.msg.msgid = 1;
    data_snd.msg.msgcount = 1;
    data_snd.msg.seqid = 1;
    strcpy(data_snd.msg.top_key,"level1");
    strcpy(data_snd.msg.second_key,"level2");
    strcpy(data_snd.msg.third_key,"level3");
    strcpy(data_snd.msg.data,"data");
    
    pthread_create(&t_data_rcv,NULL,data_rcv_thread,&data_rcv);
    //pthread_create(&t_data_snd,NULL,data_snd_thread,&data_snd);
    pthread_join(t_data_rcv,NULL);
    //pthread_join(t_data_snd,NULL);
    return 0;
}
