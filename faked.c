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
    
    fakeroot script
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
       LD_PRELOAD=libtricks.so.0.0, and FAKEROOT_DBKEY=ipc-key,
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

#include "config.h"
#include "communicate.h"
#ifndef FAKEROOT_FAKENET
# include <sys/ipc.h>
# include <sys/msg.h>
# include <sys/sem.h>
#else /* FAKEROOT_FAKENET */
# include <sys/socket.h>
# include <sys/param.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif /* FAKEROOT_FAKENET */
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_SYS_SYSMACROS_H
# include <sys/sysmacros.h>
#endif
#ifdef FAKEROOT_DB_PATH
# include <dirent.h>
#endif

#ifndef FAKEROOT_FAKENET
# define FAKE_KEY msg_key
#else /* FAKEROOT_FAKENET */
# define FAKE_KEY port
#endif /* FAKEROOT_FAKENET */

#ifndef SOL_TCP
# define SOL_TCP 6 /* this should probably be done with getprotoent */
#endif

#define fakestat_equal(a, b)  ((a)->dev == (b)->dev && (a)->ino == (b)->ino)

#ifndef FAKEROOT_FAKENET
# if HAVE_SEMUN_DEF == 0
  union semun {
    int val;
    struct semid_ds *buf;
    u_short *array;
  };
# endif
#endif /* ! FAKEROOT_FAKENET */

void process_chown(struct fake_msg *buf);
void process_chmod(struct fake_msg *buf);
void process_mknod(struct fake_msg *buf);
void process_stat(struct fake_msg *buf);
void process_unlink(struct fake_msg *buf);

#ifdef FAKEROOT_FAKENET
static int get_fakem(struct fake_msg *buf);
#endif

typedef void (*process_func)(struct fake_msg *);

process_func func_arr[]={process_chown,
			 process_chmod,
			 process_mknod,
			 process_stat,
			 process_unlink,
			 };

unsigned int highest_funcid = sizeof(func_arr)/sizeof(func_arr[0]);

#ifndef FAKEROOT_FAKENET
key_t msg_key=0;
#else /* FAKEROOT_FAKENET */
static int comm_sd = -1;
#endif /* FAKEROOT_FAKENET */

int debug = 0, unknown_is_real = 0;
char *save_file = NULL;

#ifdef FAKEROOT_FAKENET
static void fail(const char *msg)
{
  if (errno > 0)
    fprintf(stderr, "fakeroot daemon: %s (%s)\n", msg, strerror(errno));
  else
    fprintf(stderr, "fakeroot daemon: %s\n", msg);

  exit(1);
}
#endif

struct data_node_s;
typedef struct data_node_s {
  struct data_node_s *next;
  struct fakestat     buf;
  uint32_t            remote;
} data_node_t;

#define data_node_get(n)   ((struct fakestat *) &(n)->buf)

#define HASH_TABLE_SIZE 10009
#define HASH_DEV_MULTIPLIER 8328 /* = 2^64 % HASH_TABLE_SIZE */

static int data_hash_val(const struct fakestat *key) {
  return (key->dev * HASH_DEV_MULTIPLIER + key->ino) % HASH_TABLE_SIZE;
}

static data_node_t *data_hash_table[HASH_TABLE_SIZE];

static void init_hash_table() {
  int table_pos;

  for (table_pos = 0; table_pos < HASH_TABLE_SIZE; table_pos++)
    data_hash_table[table_pos] = NULL;
}

static data_node_t *data_find(const struct fakestat *key,
			      const uint32_t remote)
{
  data_node_t *n;

  for (n = data_hash_table[data_hash_val(key)]; n; n = n->next) {
    if (fakestat_equal(&n->buf, key) && n->remote == remote)
      break;
  }

  return n;
}

static void data_insert(const struct fakestat *buf,
			const uint32_t remote)
{
  data_node_t *n, *last = NULL;
  
  for (n = data_hash_table[data_hash_val(buf)]; n; last = n, n = n->next)
    if (fakestat_equal(&n->buf, buf) && n->remote == remote)
      break;

  if (n == NULL) {
    n = calloc(1, sizeof (data_node_t));

    if (last)
      last->next = n;
    else
      data_hash_table[data_hash_val(buf)] = n;
  }

  memcpy(&n->buf, buf, sizeof (struct fakestat));
  n->remote = (uint32_t) remote;
}

static data_node_t *data_erase(data_node_t *pos)
{
  data_node_t *n, *prev = NULL, *next;

  for (n = data_hash_table[data_hash_val(&pos->buf)]; n;
       prev = n, n = n->next)
    if (n == pos)
      break;

  next = n->next;

  if (n == data_hash_table[data_hash_val(&pos->buf)])
    data_hash_table[data_hash_val(&pos->buf)] = next;
  else
    prev->next = next;

  free(n);

  return next;
}

static data_node_t *data_node_next(data_node_t *n) {
  int table_pos;

  if (n != NULL && n->next != NULL)
    return n->next;

  if (n == NULL)
    table_pos = 0;
  else
    table_pos = data_hash_val(&n->buf) + 1;
  while (table_pos < HASH_TABLE_SIZE && data_hash_table[table_pos] == NULL)
    table_pos++;
  if (table_pos < HASH_TABLE_SIZE)
    return data_hash_table[table_pos];
  else
    return NULL;
}

static unsigned int data_size(void)
{
  unsigned int size = 0;
  int table_pos;
  data_node_t *n;

  for (table_pos = 0; table_pos < HASH_TABLE_SIZE; table_pos++)
    for (n = data_hash_table[table_pos]; n; n = n->next)
      size++;

  return size;

}

#define data_begin()  (data_node_next(NULL))
#define data_end()    (NULL)


#ifdef FAKEROOT_FAKENET
static struct {
  unsigned int capacity;
  unsigned int size;
  int *array;
} sd_list = {
  0, 0, NULL
};

static void sd_list_add(int sd)
{
  if (sd_list.capacity == sd_list.size) {
    sd_list.capacity += 16;

    if (sd_list.array == NULL) {

      sd_list.array = malloc(sd_list.capacity * sizeof (int));
      if (!sd_list.array)
	fail("malloc");
    } else {
      sd_list.array = realloc(sd_list.array, sd_list.capacity * sizeof (int));
      if (!sd_list.array)
	fail("realloc");

    }
  }

  sd_list.array[sd_list.size] = sd;
  sd_list.size++;
}

static void sd_list_remove(unsigned int i)
{
  for (i++; i < sd_list.size; i++)
    sd_list.array[i - 1] = sd_list.array[i];
  sd_list.size--;
}

#define sd_list_size()    (sd_list.size)
#define sd_list_index(i)  (sd_list.array[(i)])

static void faked_send_fakem(const struct fake_msg *buf)
{
  struct fake_msg fm;

  fm.id = htonl(buf->id);
  fm.st.uid = htonl(buf->st.uid);
  fm.st.gid = htonl(buf->st.gid);
  fm.st.ino = htonll(buf->st.ino);
  fm.st.dev = htonll(buf->st.dev);
  fm.st.rdev = htonll(buf->st.rdev);
  fm.st.mode = htonl(buf->st.mode);
  fm.st.nlink = htonl(buf->st.nlink);
  fm.remote = htonl(buf->remote);

  while (1) {
    ssize_t len;

    len = write(comm_sd, &fm, sizeof (fm));
    if (len > 0)
      break;

    if (errno == EINTR)
      continue;

    fail("write");
  }
}
#else

# define faked_send_fakem send_fakem

#endif /* FAKEROOT_FAKENET */

#ifdef FAKEROOT_DB_PATH
# define DB_PATH_LEN    4095
# define DB_PATH_SCAN "%4095s"

/*
 * IN:  'path' contains the dir to scan recursively
 * OUT: 'path' contains the matching file if 1 is returned
 */
static int scan_dir(const fake_dev_t dev, const fake_ino_t ino,
                    char *const path)
{
  const size_t pathlen = strlen(path) + strlen("/");
  if (pathlen >= DB_PATH_LEN)
    return 0;
  strcat(path, "/");

  DIR *const dir = opendir(path);
  if (!dir)
    return 0;

  struct dirent *ent;
  while ((ent = readdir(dir))) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    if (ent->d_ino == ino) {
      struct stat buf;
      strncpy(path + pathlen, ent->d_name, DB_PATH_LEN - pathlen);
      if (lstat(path, &buf) == 0 && buf.st_dev == dev)
        break;
    } else if (ent->d_type == DT_DIR) {
      strncpy(path + pathlen, ent->d_name, DB_PATH_LEN - pathlen);
      if (scan_dir(dev, ino, path))
        break;
    }
  }

  closedir(dir);
  return ent != 0;
}

/*
 * Finds a path for inode/device pair--there can be several if bind mounts
 * are used.  This should not be a problem if the bind mount configuration
 * is the same when loading the database.
 *
 * IN:  'roots' contains the dirs to scan recursively (separated by colons)
 * OUT: 'path' contains the matching file if 1 is returned
 */
static int find_path(const fake_dev_t dev, const fake_ino_t ino,
                     const char *const roots, char *const path)
{
  unsigned int end = 0;

  do {
    unsigned int len, start = end;

    while (roots[end] != '\0' && roots[end] != ':')
      end++;

    len = end - start;
    if (len == 0)
      continue;

    if (roots[end - 1] == '/')
      len--;

    if (len > DB_PATH_LEN)
      len = DB_PATH_LEN;

    strncpy(path, roots + start, len);
    path[len] = '\0';

    if (scan_dir(dev, ino, path))
      return 1;
  } while (roots[end++] != '\0');

  return 0;
}

#endif

int save_database(const uint32_t remote)
{
#ifdef FAKEROOT_DB_PATH
  char path[DB_PATH_LEN + 1];
  const char *roots;
#endif
  data_node_t *i;
  FILE *f;

  if(!save_file)
    return 1;

#ifdef FAKEROOT_DB_PATH
  path[DB_PATH_LEN] = '\0';

  roots = getenv(DB_SEARCH_PATHS_ENV);
  if (!roots)
    roots = "/";
#endif

  f=fopen(save_file, "w");
  if(!f)
    return 0;

  for (i = data_begin(); i != data_end(); i = data_node_next(i)) {
    if (i->remote != remote)
      continue;

#ifdef FAKEROOT_DB_PATH
    if (find_path(i->buf.dev, i->buf.ino, roots, path))
      fprintf(f,"mode=%llo,uid=%llu,gid=%llu,nlink=%llu,rdev=%llu %s\n",
              (uint64_t) i->buf.mode,(uint64_t) i->buf.uid,(uint64_t) i->buf.gid,
              (uint64_t) i->buf.nlink,(uint64_t) i->buf.rdev,path);
#else
    fprintf(f,"dev=%llx,ino=%llu,mode=%llo,uid=%llu,gid=%llu,nlink=%llu,rdev=%llu\n",
            (uint64_t) i->buf.dev,(uint64_t) i->buf.ino,(uint64_t) i->buf.mode,
            (uint64_t) i->buf.uid,(uint64_t) i->buf.gid,(uint64_t) i->buf.nlink,
            (uint64_t) i->buf.rdev);
#endif
  }

  return fclose(f);
}

int load_database(const uint32_t remote)
{
  int r;

  uint64_t stdev, stino, stmode, stuid, stgid, stnlink, strdev;
  struct fakestat st;

#ifdef FAKEROOT_DB_PATH
  char path[DB_PATH_LEN + 1];
  struct stat path_st;

  path[DB_PATH_LEN] = '\0';
#endif

  while(1){
#ifdef FAKEROOT_DB_PATH
    r=scanf("mode=%llo,uid=%llu,gid=%llu,nlink=%llu,rdev=%llu "DB_PATH_SCAN"\n",
            &stmode, &stuid, &stgid, &stnlink, &strdev, &path);
    if (r != 6)
      break;

    if (stat(path, &path_st) < 0) {
      fprintf(stderr, "%s: %s\n", path, strerror(errno));
      if (errno == ENOENT || errno == EACCES)
        continue;
      else
        break;
    }
    stdev = path_st.st_dev;
    stino = path_st.st_ino;
#else
    r=scanf("dev=%llx,ino=%llu,mode=%llo,uid=%llu,gid=%llu,nlink=%llu,rdev=%llu\n",
            &stdev, &stino, &stmode, &stuid, &stgid, &stnlink, &strdev);
    if (r != 7)
      break;
#endif

    st.dev = stdev;
    st.ino = stino;
    st.mode = stmode;
    st.uid = stuid;
    st.gid = stgid;
    st.nlink = stnlink;
    st.rdev = strdev;
    data_insert(&st, remote);
  }
  if(!r||r==EOF)
    return 1;
  else
    return 0;
}

/*********************************/
/*                               */
/* data base maintainance        */
/*                               */
/*********************************/
void debug_stat(const struct fakestat *st){
  fprintf(stderr,"dev:ino=(%llx:%lli), mode=0%lo, own=(%li,%li), nlink=%li, rdev=%lli\n",
	  st->dev,
	  st->ino,
	  st->mode,
	  st->uid,
	  st->gid,
	  st->nlink,
	  st->rdev);
}

void insert_or_overwrite(struct fakestat *st,
			 const uint32_t remote){
  data_node_t *i;
  
  i = data_find(st, remote);
  if (i == data_end()) {
    if(debug){
      fprintf(stderr,"FAKEROOT: insert_or_overwrite unknown stat:\n");
      debug_stat(st);
    }
    data_insert(st, remote);
  }
  else
	memcpy(data_node_get(i), st, sizeof (struct fakestat));
}

/*******************************************/
/*                                         */
/* process requests from wrapper functions */
/*                                         */
/*******************************************/


void process_chown(struct fake_msg *buf){
  struct fakestat *stptr;
  struct fakestat st;
  data_node_t *i;
  
  if(debug){
    fprintf(stderr,"FAKEROOT: chown ");
    debug_stat(&buf->st);
  }
  i = data_find(&buf->st, buf->remote);
  if (i != data_end()) {
    stptr = data_node_get(i);
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
    insert_or_overwrite(&st, buf->remote);
  }
}

void process_chmod(struct fake_msg *buf){
  struct fakestat *st;
  data_node_t *i;
  
  if(debug)
    fprintf(stderr,"FAKEROOT: chmod, mode=%lo\n",
	    buf->st.mode);
  
  i = data_find(&buf->st, buf->remote);
  if (i != data_end()) {
    st = data_node_get(i);
    st->mode = (buf->st.mode&~S_IFMT) | (st->mode&S_IFMT);
  }
  else{
    st=&buf->st;
    st->uid=0;
    st->gid=0;
  }
  insert_or_overwrite(st, buf->remote);
}

void process_mknod(struct fake_msg *buf){
  struct fakestat *st;
  data_node_t *i;
  
  if(debug)
    fprintf(stderr,"FAKEROOT: chmod, mode=%lo\n",
	    buf->st.mode);
  
  i = data_find(&buf->st, buf->remote);
  if (i != data_end()) {
    st = data_node_get(i);
    st->mode = buf->st.mode;
    st->rdev = buf->st.rdev;
  }
  else{
    st=&buf->st;
    st->uid=0;
    st->gid=0;
  }
  insert_or_overwrite(st, buf->remote);
}

void process_stat(struct fake_msg *buf){
  data_node_t *i;

  i = data_find(&buf->st, buf->remote);
  if(debug){
    fprintf(stderr,"FAKEROOT: process stat oldstate=");
    debug_stat(&buf->st);
  }
  if (i == data_end()) {
    if (debug)
      fprintf(stderr,"FAKEROOT:    (previously unknown)\n");
    if (!unknown_is_real) {
      buf->st.uid=0;
      buf->st.gid=0;
    }
  }
  else{
    cpyfakefake(&buf->st, data_node_get(i));
    if(debug){
      fprintf(stderr,"FAKEROOT: (previously known): fake=");
      debug_stat(&buf->st);      
    }

  }
  faked_send_fakem(buf);
}
//void process_fstat(struct fake_msg *buf){
//  process_stat(buf);
//}

void process_unlink(struct fake_msg *buf){

  if((buf->st.nlink==1)||
     (S_ISDIR(buf->st.mode)&&(buf->st.nlink==2))){
    data_node_t *i;
    i = data_find(&buf->st, buf->remote);
    if (i != data_end()) {
      if(debug){
	fprintf(stderr,"FAKEROOT: unlink known file, old stat=");
	debug_stat(data_node_get(i));
      }
      data_erase(i);
    }
    if (data_find(&buf->st, buf->remote) != data_end()) {
      fprintf(stderr,"FAKEROOT************************************************* cannot remove stat (a \"cannot happen\")\n");
    }
  }
}

void debugdata(int dummy UNUSED){
  int stored_errno = errno;
  data_node_t *i;

  fprintf(stderr," FAKED keeps data of %i inodes:\n", data_size());
  for (i = data_begin(); i != data_end(); i = data_node_next(i))
    debug_stat(data_node_get(i));

  errno = stored_errno;
}


void process_msg(struct fake_msg *buf){

  func_id_t f;
  f= buf->id;
  if (f <= highest_funcid)
    func_arr[f]((struct fake_msg*)buf);
}

#ifndef FAKEROOT_FAKENET

void get_msg()
{
  struct fake_msg buf;
  int r = 0;

  if(debug)
    fprintf(stderr,"FAKEROOT: msg=%i, key=%i\n",msg_get,msg_key);
  do {
    r=msgrcv(msg_get,&buf,sizeof(struct fake_msg),0,0);
    if(debug)
      fprintf(stderr,"FAKEROOT: r=%i, received message type=%li, message=%i\n",r,buf.mtype,buf.id);
    if(r!=-1)
      buf.remote = 0;
      process_msg(&buf);
  }while ((r!=-1)||(errno==EINTR));
  if(debug){
    perror("FAKEROOT, get_msg");
    fprintf(stderr,"r=%i, EINTR=%i\n",errno,EINTR);
  }
}

#else /* FAKEROOT_FAKENET */

void get_msg(const int listen_sd)
{
  struct fake_msg buf;
  fd_set readfds;

  while (1) {
    int count, maxfd;
    unsigned int i;

    FD_ZERO(&readfds);

    FD_SET(listen_sd, &readfds);
    maxfd = listen_sd;

    for (i = 0; i < sd_list_size(); i++) {
      const int sd = sd_list_index(i);

      FD_SET(sd, &readfds);
      maxfd = MAX(sd, maxfd);
    }

    count = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (count < 0) {
      if (errno == EINTR)
	continue;

      fail("select");
    }

    for (i = 0; i < sd_list_size(); ) {
      const int sd = sd_list_index(i);

      if (FD_ISSET(sd, &readfds)) {
	if (debug)
	  fprintf(stderr, "fakeroot: message from fd=%d\n", sd);

	comm_sd = sd;

	if (get_fakem(&buf) < 0) {
	  if (debug)
	    fprintf(stderr, "fakeroot: closing fd=%d\n", sd);

	  close(sd);

	  sd_list_remove(i);
	  continue;
	}

	process_msg(&buf);
      }

      i++;
    }

    if (FD_ISSET(listen_sd, &readfds)) {
      struct sockaddr_in addr;
      socklen_t len = sizeof (addr);
      const int sd = accept(listen_sd, (struct sockaddr *) &addr, &len);
      if (sd < 0)
	fail("accept");

      if (debug) {
	char host[256];
	if (getnameinfo((struct sockaddr *) &addr, len, host, sizeof (host),
	    NULL, 0, 0) == 0)
	  fprintf(stderr, "fakeroot: connection from %s, fd=%d\n", host, sd);
      }

      comm_sd = sd;

      if (get_fakem(&buf) < 0) {
	if (debug)
	  fprintf(stderr, "fakeroot: closing fd=%d\n", sd);

	close(sd);
	continue;
      }

      process_msg(&buf);
      sd_list_add(sd);
    }
  }
}

#endif /* FAKEROOT_FAKENET */

/***********/
/*         */
/* misc    */
/*         */
/***********/

void save(int dummy){
  if(!save_database(0)) {
    if(debug)
      fprintf(stderr, "fakeroot: saved database in %s\n", save_file);
  } else
    if(debug)
      fprintf(stderr, "fakeroot: database save FAILED\n");
}

#ifndef FAKEROOT_FAKENET
# define FAKEROOT_CLEANUPMSG "fakeroot: clearing up message queues and semaphores, signal=%i\n"
#else /* FAKEROOT_FAKENET */
# define FAKEROOT_CLEANUPMSG "fakeroot: signal=%i\n"
#endif /* FAKEROOT_FAKENET */

void cleanup(int g)
{
#ifndef FAKEROOT_FAKENET
  union semun sem_union;
#endif /* ! FAKEROOT_FAKENET */

  if(debug)
    fprintf(stderr, FAKEROOT_CLEANUPMSG,  g);

#ifndef FAKEROOT_FAKENET
  msgctl (msg_get, IPC_RMID,NULL);
  msgctl (msg_snd, IPC_RMID,NULL);
  semctl (sem_id,0,IPC_RMID,sem_union);
#endif /* ! FAKEROOT_FAKENET */

  save(0);

  if(g!=-1)
    exit(0);
}

/*************/
/*           */
/*   main    */
/*           */
/*************/

static long int read_intarg(char **argv)
{
  if(!*argv){
    fprintf(stderr,"%s needs numeric argument\n",*(argv-1));
    exit(1);
  } else
  {
    return atoi(*argv);
  }
}

#ifdef FAKEROOT_FAKENET
static int get_fakem(struct fake_msg *buf)
{
  while (1) {
    ssize_t len;

    len = read(comm_sd, buf, sizeof (struct fake_msg));
    if (len > 0)
      break;

    if (len == 0)
      return -1;

    if (errno == EINTR)
      continue;

    fail("read");
  }

  buf->id = ntohl(buf->id);
  buf->st.uid = ntohl(buf->st.uid);
  buf->st.gid = ntohl(buf->st.gid);
  buf->st.ino = ntohll(buf->st.ino);
  buf->st.dev = ntohll(buf->st.dev);
  buf->st.rdev = ntohll(buf->st.rdev);
  buf->st.mode = ntohl(buf->st.mode);
  buf->st.nlink = ntohl(buf->st.nlink);
  buf->remote = ntohl(buf->remote);

  return 0;
}
#endif /* FAKEROOT_FAKENET */

int main(int argc, char **argv){
  struct sigaction sa,sa_debug,sa_save;
  int i;
  int foreground = 0;
  int load = 0;
  int pid;
#ifndef FAKEROOT_FAKENET
  union semun sem_union;
  int justcleanup = 0;
#else /* FAKEROOT_FAKENET */
  int sd, val;
  unsigned int port = 0;
  struct sockaddr_in addr;
  socklen_t addr_len;
#endif /* FAKEROOT_FAKENET */

  if(getenv(FAKEROOTKEY_ENV)) {
 /* I'm not sure -- maybe this can work?) */
    fprintf(stderr,"Please, don't run fakeroot from within fakeroot!\n");
    exit(1);
  }

  while(*(++argv)){
    if(!strcmp(*argv,"--key"))
#ifndef FAKEROOT_FAKENET
      msg_key=read_intarg(++argv);
#else /* FAKEROOT_FAKENET */
      fprintf(stderr,"This fakeroot has been compiled for TCP and does not support --key\n");
#endif /* FAKEROOT_FAKENET */
    else if(!strcmp(*argv,"--cleanup")) {
#ifndef FAKEROOT_FAKENET
      msg_key=read_intarg(++argv);
      justcleanup= 1;
#else /* FAKEROOT_FAKENET */
      fprintf(stderr,"This fakeroot has been compiled for TCP and does not support --cleanup\n");
#endif /* FAKEROOT_FAKENET */
    }
    else if(!strcmp(*argv,"--port"))
#ifndef FAKEROOT_FAKENET
      fprintf(stderr,"This fakeroot has been compiled for SYSV IPC and does not support --port\n");
#else /* FAKEROOT_FAKENET */
      port=read_intarg(++argv);
#endif /* FAKEROOT_FAKENET */
    else if(!strcmp(*argv,"--foreground"))
      foreground = 1;
    else if(!strcmp(*argv,"--debug"))
      debug=1;
    else if(!strcmp(*argv,"--save-file"))
      save_file=*(++argv);
    else if(!strcmp(*argv,"--load"))
      load=1;
    else if(!strcmp(*argv,"--unknown-is-real"))
      unknown_is_real = 1;
    else if(!strcmp(*argv,"--version")) {
      fprintf(stderr,"fakeroot version " VERSION "\n");
      exit(0);
    } else {
      fprintf(stderr,"faked, daemon for fake root environment\n");
      fprintf(stderr,"Best used from the shell script `fakeroot'\n");
#ifndef FAKEROOT_FAKENET
      fprintf(stderr,"options for fakeroot: --key, --cleanup, --foreground, --debug, --save-file, --load, --unknown-is-real\n");
#else /* FAKEROOT_FAKENET */
      fprintf(stderr,"options for fakeroot: --port, --foreground, --debug, --save-file, --load, --unknown-is-real\n");
#endif /* FAKEROOT_FAKENET */
      exit(1);
    }
  }

  init_hash_table();

  if(load)
    if(!load_database(0)) {
      fprintf(stderr,"Database load failed\n");
      exit(1);
    }

#ifndef FAKEROOT_FAKENET

  if(!msg_key) {
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

#else /* FAKEROOT_FAKENET */

  sd = socket(PF_INET, SOCK_STREAM, 0);
  if (sd < 0)
    fail("socket");

  val = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val)) < 0)
    fail("setsockopt(SO_REUSEADDR)");

  val = 1;
  if (setsockopt(sd, SOL_TCP, TCP_NODELAY, &val, sizeof (val)) < 0)
    fail("setsockopt(TCP_NODELAY)");

  if (port > 0) {
    memset((char *) &addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sd, (struct sockaddr *) &addr, sizeof (addr)) < 0)
      fail("bind");
  }

  if (listen(sd, SOMAXCONN) < 0)
    fail("listen");

  addr_len = sizeof (addr);
  if (getsockname(sd, (struct sockaddr *) &addr, &addr_len) < 0)
    fail("getsockname");

  port = ntohs(addr.sin_port);

#endif /* FAKEROOT_FAKENET */

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
#ifdef FAKEROOT_FAKENET
	if (fl != sd)
#endif /* FAKEROOT_FAKENET */
	  close(fl);
      setsid();
    } else {
      printf("%i:%i\n",FAKE_KEY,pid);

      exit(0);
    }
  } else {
    printf("%i:%i\n",FAKE_KEY,getpid());
    fflush(stdout);
  }

#ifndef FAKEROOT_FAKENET
  get_msg();    /* we shouldn't return from this function */
#else /* FAKEROOT_FAKENET */
  get_msg(sd);  /* we shouldn't return from this function */
#endif /* FAKEROOT_FAKENET */

  cleanup(-1);  /* if we do return, try to clean up and exit with a nonzero
		   return status */
  return 1;
}
