/*
  copyright   : GPL
  title       : fakeroot
  description : create a "fake" root shell, by wrapping 
                functions like chown, stat, etc. Useful for debian
                packaging mechanism
  author      : joost witteveen, joostje@debian.org
*/

/*
  upon startup, the fakeroot script (/usr/bin/fakeroot) 
  forks faked (this program), and the shell or user program that
  will run with the libtricks.so.0.0 wrapper.
  
  These tree running programs have the following tasks:
    
    fakeroot scripot
       starts the other two processes, waits for the user process to
       die, and then send a SIGTERM signal to faked, causing
       Faked to clear the ipc message queues.

    faked
       the ``main'' daemon, creates ipc message queues, and later
       receives ipc messages from the user program, maintains
       fake inode<->ownership database (actually just a 
       lot of struct stat entries). Will clear ipc message ques
       upon receipt of a SIGTERM. Will show debug output upon
       receipt of a SIGUSR1 (if started with -d debug option)

    user program
       Any shell or other programme, run with 
       LD_PRELOAD=libtricks.so.0.0, and FAKEROOTKEY=ipc-key,
       thus the executed commands will communicate with
       faked. libtricks will wrap all file ownership etc modification
       calls, and send the info to faked. Also the stat() function
       is wrapped, it will first ask the database kept by faked
       and report the `fake' data if available.

  The following functions are currently wrapped:
     getuid(), geteuid(), getgid(), getegid(),
     mknod()
     chown(), fchown() lchown()
     chmod(), fchmod() 
     mkdir(),
     lstat(), fstat(), stat() (actually, __xlstat, ...)
     unlink(), remove(), rmdir(), rename()
    
  comments:
    I need to wrap unlink because of the following:
        install -o admin foo bar
	rm bar
        touch bar         //bar now may have the same inode:dev as old bar,
	                  //but unless the rm was caught,
			  //fakeroot still has the old entry.
        ls -al bar
    Same goes for all other ways to remove inodes form the filesystem,
    like rename(existing_file, any_file).

    The communication between client (user progamme) and faked happens 
    with inode/dev information, not filenames. This is 
    needed, as the client is the only one who knows what cwd is,
    so it's much easier to stat in the client. Otherwise, the daemon
    needs to keep a list of client pids vs cwd, and I'd have to wrap
    fork e.d., as they inherit their parent's cwd. Very compilcated.
    
    */			       
/* ipc documentation bugs: msgsnd(2): MSGMAX=4056, not 4080 
   (def in ./linux/msg.h, couldn't find other def in /usr/include/ 
   */


#include "communicate.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <string.h>
#include <iostream>
#include <signal.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif


#if HAVE_SEMUN_DEF == 0
  union semun {
    int val;
    struct semid_ds *buf;
    u_short *array;
  };
#endif

void process_chown(struct fake_msg *buf);
void process_chmod(struct fake_msg *buf);
void process_mknod(struct fake_msg *buf);
void process_stat(struct fake_msg *buf);
void process_unlink(struct fake_msg *buf);

bool operator <(const struct fakestat &a, const struct fakestat &b);

typedef void (*process_func)(struct fake_msg *);

process_func func_arr[]={process_chown,
			 process_chmod,
			 process_mknod,
			 process_stat,
			 process_unlink,
			 };

int highest_funcid = sizeof(func_arr)/sizeof(func_arr[0]);

key_t msg_key=0;
//static int msg_get,msg_send, sem_id;
bool debug=false;
char *save_file=0;


// our main (and only) data base, contains all inodes and other info:
bool operator <(const struct fakestat &a, const struct fakestat &b){
  
  if(a.ino<b.ino)
    return true;
  else if(a.ino> b.ino)
    return false;
  else 
    return a.dev<b.dev;
};

std::set <struct fakestat, std::less<struct fakestat> > data;




/*********************************/
/*                               */
/* data base maintainance        */
/*                               */
/*********************************/
void debug_stat(const struct fakestat *st){
  fprintf(stderr,"dev:ino=(%lx:%li), mode=0%lo, own=(%li,%li), nlink=%li, rdev=%li\n",
	  st->dev,
	  st->ino,
	  st->mode,
	  st->uid,
	  st->gid,
	  st->nlink,
	  st->rdev);
}


/*int init_get_msg(){
  return 1;
}
key_t get_ipc_key(){
  return msg_key;
}
*/
void insert_or_overwrite(std::set <struct fakestat, std::less<struct fakestat> > &data,
			 struct fakestat &st){
  // just doing a data.insert(stat) would not update data
  // of inodes that are already present (if operator compares "equal",
  // stat isn't writen).

  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;

  
  i=data.find(st);
  if(i==data.end()){
    if(debug){
      fprintf(stderr,"FAKEROOT: insert_or_overwrite unknown stat:\n");
      debug_stat(&st);
    }
    data.insert(st);
  }
  else
    memcpy(((struct fakestat *)&(*i)),&st,sizeof(st));
}


/*void copy_stat(struct fakestat *dest, const struct stat *source){
  //copy only the things we fake (i.e. "know better"), and leave
  //the other info (like file size etc).
    dest->st_dev =source->st_dev;
    dest->st_rdev=source->st_rdev;
    dest->st_ino =source->st_ino;
    dest->st_uid =source->st_uid;
    dest->st_gid =source->st_gid;
    dest->st_mode=source->st_mode;
}
*/

int save_database(std::set <struct fakestat,std::less<struct fakestat> > &data){
  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;
  FILE *f;

  if(!save_file)
    return 1;

  f=fopen(save_file, "w");
  if(!f)
    return 0;

  for(i=data.begin();i!=data.end();i++) {
    fprintf(f,"dev=%llx,ino=%llu,mode=%lo,uid=%lu,gid=%lu,nlink=%lu,rdev=%lu\n",
	i->dev,i->ino,i->mode,i->uid,i->gid,i->nlink,i->rdev);
  }

  return fclose(f);
}

int load_database(std::set <struct fakestat,std::less<struct fakestat> > &data){
  int r;
  struct fakestat st;
  while(1){
    r=scanf("dev=%llx,ino=%llu,mode=%lo,uid=%lu,gid=%lu,nlink=%lu,rdev=%lu\n",
	&(st.dev), &(st.ino), &(st.mode), &(st.uid), &(st.gid), &(st.nlink),
	&(st.rdev));
    if(r==7)
      data.insert(st);
    else
      break;
  }
  if(!r||r==EOF)
    return 1;
  else
    return 0;
}

/*******************************************/
/*                                         */
/* process requests from wrapper functions */
/*                                         */
/*******************************************/


void process_chown(struct fake_msg *buf){
  struct fakestat *stptr;
  struct fakestat st;
  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;
  
  if(debug){
    fprintf(stderr,"FAKEROOT: chown ");
    debug_stat(&buf->st);
  }
  i=data.find(buf->st);
  if(i!=data.end()){
    stptr=(struct fakestat *)&(*i); //to remove the const type.
    /* From chown(2): If  the owner or group is specified as -1, 
       then that ID is not changed. 
       Cannot put that test in libtricks, as at that point it isn't
       known what the fake user/group is (so cannot specify `unchanged')
       
       I typecast to (uint32_t), as st.uid may be bigger than uid_t.
       In that case, the msb in st.uid should be discarded.
       I don't typecaset to (uid_t), as the size of uid_t may vary
       depending on what libc (headers) were used to compile. So,
       different clients might actually use different uid_t's
       concurrently. Yes, this does seem farfeched, but was
       actually the case with the libc5/6 transition.
    */
    if ((uint32_t)buf->st.uid != (uint32_t)-1) 
      stptr->uid=buf->st.uid;
    if ((uint32_t)buf->st.gid != (uint32_t)-1)
      stptr->gid=buf->st.gid;
  }
  else{
    st=buf->st;
    /* See comment above.  We pretend that unknown files are owned
       by root.root, so we have to maintain that pretense when the
       caller asks to leave an id unchanged. */
    if ((uint32_t)st.uid == (uint32_t)-1)
       st.uid = 0;
    if ((uint32_t)st.gid == (uint32_t)-1)
       st.gid = 0;
    insert_or_overwrite(data,st);
  }
}

void process_chmod(struct fake_msg *buf){
  struct fakestat *st;
  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;
  
  if(debug)
    fprintf(stderr,"FAKEROOT: chmod, mode=%lo\n",
	    buf->st.mode);
  
  i=data.find(buf->st);
  if(i!=data.end()){
    st=(struct fakestat *)&(*i);
    st->mode = (buf->st.mode&~S_IFMT) | (st->mode&S_IFMT);
  }
  else{
    st=&buf->st;
    st->uid=0;
    st->gid=0;
  }
  insert_or_overwrite(data,*st);
}
void process_mknod(struct fake_msg *buf){
  struct fakestat *st;
  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;
  
  if(debug)
    fprintf(stderr,"FAKEROOT: chmod, mode=%lo\n",
	    buf->st.mode);
  
  i=data.find(buf->st);
  if(i!=data.end()){
    st=(struct fakestat *)&(*i);
    st->mode = buf->st.mode;
    st->rdev = buf->st.rdev;
  }
  else{
    st=&buf->st;
    st->uid=0;
    st->gid=0;
  }
  insert_or_overwrite(data,*st);
}

void process_stat(struct fake_msg *buf){
  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;

  i=data.find(buf->st);
  if(debug){
    fprintf(stderr,"FAKEROOT: process stat oldstate=");
    debug_stat(&buf->st);
  }
  if(i==data.end()){
    if (debug)
      fprintf(stderr,"FAKEROOT:    (previously unknown)\n");
    buf->st.uid=0;
    buf->st.gid=0;
  }
  else{
    cpyfakefake(&buf->st,&(*i));
    if(debug){
      fprintf(stderr,"FAKEROOT: (previously known): fake=");
      debug_stat(&buf->st);      
    }

  }
  send_fakem(buf);
}
//void process_fstat(struct fake_msg *buf){
//  process_stat(buf);
//}

void process_unlink(struct fake_msg *buf){

  if((buf->st.nlink==1)||
     (S_ISDIR(buf->st.mode)&&(buf->st.nlink==2))){
    std::set <struct fakestat, std::less<struct fakestat> >::iterator i;
    i=data.find(buf->st);
    if(i!=data.end()){
      if(debug){
	fprintf(stderr,"FAKEROOT: unlink known file, old stat=");
	debug_stat(&(*i));
      }
      data.erase(i);
    }
    if(data.find(buf->st)!=data.end()){
      fprintf(stderr,"FAKEROOT************************************************* cannot remove stat (a \"cannot happen\")\n");
    }
  }
}

void debugdata(int){
  std::set <struct fakestat, std::less<struct fakestat> >::iterator i;

  fprintf(stderr," FAKED keeps data of %i inodes:\n",data.size());
  for(i=data.begin(); i!=data.end(); i++)
    debug_stat(&(*i));
}


void process_msg(struct fake_msg *buf){

  func_id f;
  f= buf->id;
  if (f <= highest_funcid)
    func_arr[f]((struct fake_msg*)buf);
  
}

void get_msg(){

  struct fake_msg buf;
  int r=0;

  if(debug)
    fprintf(stderr,"FAKEROOT: msg=%i, key=%i\n",msg_get,msg_key);
  do{
    r=msgrcv(msg_get,&buf,sizeof(fake_msg),0,0);
    if(debug)
      fprintf(stderr,"FAKEROOT: r=%i, received message type=%li, message=%i\n",r,buf.mtype,buf.id);
    if(r!=-1)
      process_msg(&buf);
  }while ((r!=-1)||(errno==EINTR));
  if(debug){
    perror("FAKEROOT, get_msg");
    fprintf(stderr,"r=%i, EINTR=%i\n",errno,EINTR);
  }
}

/***********/
/*         */
/* misc    */
/*         */
/***********/



void save(int){
  if(!save_database(data)) {
    if(debug)
      fprintf(stderr, "fakeroot: saved database in %s\n", save_file);
  } else
    if(debug)
      fprintf(stderr, "fakeroot: database save FAILED\n");
}

void cleanup(int g){
  union semun sem_union;
  if(debug)
    fprintf(stderr, "fakeroot: clearing up message queues and semaphores,"
	    " signal=%i\n",  g);
  msgctl (msg_get, IPC_RMID,NULL);
  msgctl (msg_snd, IPC_RMID,NULL);
  semctl (sem_id,0,IPC_RMID,sem_union);
  save(0);
  if(g!=-1)
    exit(0);
}

/*************/
/*           */
/*   main    */
/*           */
/*************/


static long int read_intarg(char **argv){
  if(!*argv){
    fprintf(stderr,"%s needs numeric argument\n",*(argv-1));
    exit(1);
  } else{
    return atoi(*argv);
  }
}

int main(int /*argc*/, char **argv){
  union semun sem_union;
  struct sigaction sa,sa_debug,sa_save;
  int i;
  bool justcleanup=false;
  bool foreground=false;
  bool load=false;
  int pid;

  if(getenv(FAKEROOTKEY_ENV)){
    //(I'm not sure -- maybe this can work?)
    fprintf(stderr,"Please, don't run fakeroot from within fakeroot!\n");
    exit(1);
  }
  while(*(++argv)){
    if(!strcmp(*argv,"--key"))
      msg_key=read_intarg(++argv);
    else if(!strcmp(*argv,"--cleanup")){
      msg_key=read_intarg(++argv);
      justcleanup=true;
    }
    else if(!strcmp(*argv,"--foreground"))
      foreground=true;
    else if(!strcmp(*argv,"--debug"))
      debug=true;
    else if(!strcmp(*argv,"--save-file"))
      save_file=*(++argv);
    else if(!strcmp(*argv,"--load"))
      load=true;
    else {
      fprintf(stderr,"faked, daemon for fake root enfironment\n");
      fprintf(stderr,"Best used from the shell script `fakeroot'\n");
      fprintf(stderr,"options for fakeroot: --key, --cleanup, --foreground, --debug, --save-file, --load\n");
      exit(1);
    }
  }
  if(load)
    if(!load_database(data)){
      fprintf(stderr,"Database load failed\n");
      exit(1);
    }
  if(!msg_key){
    srandom(time(NULL)+getpid()*33151);
    while(!msg_key && (msg_key!=-1))  /* values 0 and -1 are treated
					 specially by libfake */
      msg_key=random();
    
  }
  if(debug)
    fprintf(stderr,"using %i as msg key\n",msg_key);
  
  msg_get=msgget(msg_key,IPC_CREAT|0600);
  msg_snd=msgget(msg_key+1,IPC_CREAT|0600);
  sem_id=semget(msg_key+2,1,IPC_CREAT|0600);
  sem_union.val=1;
  semctl (sem_id,0,SETVAL,sem_union);

  if((msg_get==-1)||(msg_snd==-1)||(sem_id==-1)){
    perror("fakeroot, while creating message channels");
    fprintf(stderr, "This may be due to a lack of SYSV IPC support.\n");
    cleanup(-1);
    exit(1);
  }
  if(debug)
    fprintf(stderr,"msg_key=%i\n",msg_key);

  if(justcleanup)
    cleanup(0);

  sa.sa_handler=cleanup;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags=0;
  //  sa.sa_restorer=0;


  sa_debug.sa_handler=debugdata;
  sigemptyset(&sa_debug.sa_mask);
  sa_debug.sa_flags=0;
  //  sa_debug.sa_restorer=0;
  
  sa_save.sa_handler=save;
  sigemptyset(&sa_save.sa_mask);
  sa_save.sa_flags=0;

  for(i=1; i< NSIG; i++){
    switch (i){
    case SIGKILL:
    case SIGTSTP:
    case SIGCONT:
      break;
    case SIGUSR1:
      /* this is strictly a debugging feature, unless someone can confirm
         that save will always get a consistent database */
      sigaction(i,&sa_save,NULL);
      break;
    case SIGUSR2:
      sigaction(i,&sa_debug,NULL);
      break;
    default:
      sigaction(i,&sa,NULL);
      break;
    }
  }
  


  if(!foreground){
    /* literally copied from the linux klogd code, go to background */
    if ((pid=fork()) == 0){
      int fl;
      int num_fds = getdtablesize();
      
      fflush(stdout);

      /* This is the child closing its file descriptors. */
      for (fl= 0; fl <= num_fds; ++fl)
	close(fl);
      setsid();
    } else{
      printf("%i:%i\n",msg_key,pid);

      exit(0);
    }
  } else{
    printf("%i:%i\n",msg_key,getpid());
    fflush(stdout);
  }
  get_msg();    /* we shouldn't return from this function */

  cleanup(-1);  /* if we do return, try to clean up and exit with a nonzero
		   return status */
  return 1;
}
