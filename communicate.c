/*
  copyright : GPL
    (author: joost witteveen, joostje@debian.org)


 This file contains the code (wrapper functions) that gets linked with 
 the programes run from inside fakeroot. These programes then communicate
 with the fakeroot daemon, that keeps information about the "fake" 
 ownerships etc. of the files etc.
    
 */

#include "communicate.h"
#include <dlfcn.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>



int msg_snd=-1;
int msg_get=-1;
int sem_id=-1;


#ifndef _UTSNAME_LENGTH
/* for LINUX libc5 */
  #define _UTSNAME_LENGTH _SYS_NMLN
#endif


const char *env_var_set(const char *env){
  const char *s;
  
  s=getenv(env);
  
  if(s && *s)
    return s;
  else
    return NULL;
}

void cpyfakemstat(struct fake_msg *f,
                 const struct stat     *st){
  
  f->st.mode =st->st_mode;
  f->st.ino  =st->st_ino ;
  f->st.uid  =st->st_uid ;
  f->st.gid  =st->st_gid ;
  f->st.dev  =st->st_dev ;
  f->st.rdev =st->st_rdev;

  /* DO copy the nlink count. Although the system knows this
     one better, we need it for unlink().
     This actually opens up a race condition, if another command
     makes a hardlink on a file, while we try to unlink it. This
     may cause the record to be deleted, while the link continues
     to live on the disk. But the chance is small, and unlikely
     to occur in practical fakeroot conditions. */

  f->st.nlink=st->st_nlink;
}
void cpystatfakem(struct     stat *st,
                 const struct fake_msg *f){
  st->st_mode =f->st.mode;
  st->st_ino  =f->st.ino ;
  st->st_uid  =f->st.uid ;
  st->st_gid  =f->st.gid ;
  st->st_dev  =f->st.dev ;
  st->st_rdev =f->st.rdev;
  /* DON'T copy the nlink count! The system always knows
     this one better! */
  /*  st->st_nlink=f->st.nlink;*/
}
void cpyfakefake(struct fakestat *dest,
                 const struct fakestat *source){
  dest->mode =source->mode;
  dest->ino  =source->ino ;
  dest->uid  =source->uid ;
  dest->gid  =source->gid ;
  dest->dev  =source->dev ;
  dest->rdev =source->rdev;
  /* DON'T copy the nlink count! The system always knows
     this one better! */
  /*  dest->nlink=source->nlink;*/
}


#ifdef _LARGEFILE_SOURCE

void stat64from32(struct stat64 *s64, const struct stat *s32)
{
  /* I've added st_size and st_blocks here. 
     Don't know why they were missing -- joost*/
   s64->st_dev = s32->st_dev;
   s64->st_ino = s32->st_ino;
   s64->st_mode = s32->st_mode;
   s64->st_nlink = s32->st_nlink;
   s64->st_uid = s32->st_uid;
   s64->st_gid = s32->st_gid;
   s64->st_rdev = s32->st_rdev;
   s64->st_size = s32->st_size;
   s64->st_blksize = s32->st_blksize;
   s64->st_blocks = s32->st_blocks;
   s64->st_atime = s32->st_atime;
   s64->st_mtime = s32->st_mtime;
   s64->st_ctime = s32->st_ctime;
}

/* This assumes that the 64 bit structure is actually filled in and does not
   down case the sizes from the 32 bit one.. */
void stat32from64(struct stat *s32, const struct stat64 *s64)
{
   s32->st_dev = s64->st_dev;
   s32->st_ino = s64->st_ino;
   s32->st_mode = s64->st_mode;
   s32->st_nlink = s64->st_nlink;
   s32->st_uid = s64->st_uid;
   s32->st_gid = s64->st_gid;
   s32->st_rdev = s64->st_rdev;
   s32->st_size = (long)s64->st_size;
   s32->st_blksize = s64->st_blksize;
   s32->st_blocks = (long)s64->st_blocks;
   s32->st_atime = s64->st_atime;
   s32->st_mtime = s64->st_mtime;
   s32->st_ctime = s64->st_ctime;
}

#endif


void semaphore_up(){
  struct sembuf op;
  if(sem_id==-1)
    sem_id=semget(get_ipc_key()+2,1,IPC_CREAT|0600);
  op.sem_num=0;
  op.sem_op=-1;
  op.sem_flg=SEM_UNDO;
  init_get_msg();
  while (1) {
    if (semop(sem_id,&op,1)) {
      if (errno != EINTR) {
	perror("semop(1): encountered an error");
        exit(1);
      }
    } else {
      break;
    }
  }
}
void semaphore_down(){
  struct sembuf op;
  if(sem_id==-1)
    sem_id=semget(get_ipc_key()+2,1,IPC_CREAT|0600);
  op.sem_num=0;
  op.sem_op=1;
  op.sem_flg=SEM_UNDO;
  while (1) {
    if (semop(sem_id,&op,1)) {
      if (errno != EINTR) {
	perror("semop(2): encountered an error");
        exit(1);
      }
    } else {
      break;
    }
  }
}

void  send_fakem(const struct fake_msg *buf){
  int r;
  
  if(init_get_msg()!=-1){
    ((struct fake_msg *)buf)->mtype=1;
    r=msgsnd(msg_snd, (struct fake_msg *)buf, 
	     sizeof(*buf)-sizeof(buf->mtype), 0);
    if(r==-1)
      perror("libtricks, when sending message");         
  }
}

void send_get_fakem(struct fake_msg *buf){
  /* 
     send and get a struct fakestat from the daemon. 
     We have to use serial/pid numbers in addidtion to 
     the semaphore locking, to prevent the following:

     Client 1 locks and sends a stat() request to deamon.
     meantime, client 2 tries to up the semaphore too, but blocks.
     While client 1 is waiting, it recieves a KILL signal, and dies.
     SysV semaphores can eighter be automatically cleaned up when
     a client dies, or they can stay in place. We have to use the
     cleanup version, as otherwise client 2 will block forever.
     So, the semaphore is cleaned up when client 1 recieves the KILL signal.
     Now, client 1 falls through the semaphore_up, and
     sends a stat() request to the daemon --  it will now recieve 
     the answer intended for client 1, and hell breaks lose (yes,
     this has actually happened, and yes, it was hell (to debug)).
     
     I realise that I may well do away with the semaphore stuff,
     if I put the serial/pid numbers in the mtype field. But I cannot
     store both PID and serial in mtype (just 32 bits on Linux). So 
     there will always be some (small) chance it will go wrong.
  */

  int l;
  pid_t pid;
  static int serial=0;
  

  if(init_get_msg()!=-1){

    pid=getpid();
    serial++;
    buf->serial=serial;

    semaphore_up();

    buf->pid=pid;
    send_fakem(buf);
    
    do
      l=msgrcv(msg_get,
	       (struct my_msgbuf*)buf, 
	       sizeof(*buf)-sizeof(buf->mtype),0,0);
    while((buf->serial!=serial)||buf->pid!=pid);
  
    semaphore_down();
  
    /*
      (nah, may be wrong, due to allignment)
      
      if(l!=sizeof(*buf)-sizeof(buf->mtype))
      printf("libfakeroot/fakeroot, internal bug!! get_fake: length=%i != l=%i",
      sizeof(*buf)-sizeof(buf->mtype),l);
    */
  }
}
void send_stat(const struct stat *st,
	       func_id f){
  struct fake_msg buf;
  
  if(init_get_msg()!=-1){
    cpyfakemstat(&buf,st);
    buf.id=f;
    send_fakem(&buf);
  }
}

void send_get_stat(struct stat *st){
  struct fake_msg buf;

  if(init_get_msg()!=-1){
    cpyfakemstat(&buf,st);
    
    buf.id=stat_func;
    send_get_fakem(&buf);
    cpystatfakem(st,&buf);
  }
}


key_t get_ipc_key(){
  const char *s;
  static key_t key=-1;

  if(key==-1){
    if((s=env_var_set(FAKEROOTKEY_ENV)))
      key=atoi(s);
    else
      key=0;
  };
  return key;
}


int init_get_msg(){
  /* a msgget call generates a fstat() call. As fstat() is wrapped,
     that call will in turn call semaphore_up(). So, before 
     the semaphores are setup, we should make sure we already have
     the msg_get and msg_set id.
     This is why semaphore_up() calls this function.*/
  static int done=0;
  key_t key;

  if((!done)&&(msg_get==-1)){
    key=get_ipc_key();
    if(key){
      msg_snd=msgget(get_ipc_key(),IPC_CREAT|00600);
      msg_get=msgget(get_ipc_key()+1,IPC_CREAT|00600);
    }
    else{
      msg_get=-1;
      msg_snd=-1;
    }
    done=1;
  }
  return msg_snd;
}







