/*
  Copyright: GPL. 
  Author: joost witteveen  (joostje@debian.org)
*/
/* #define _POSIX_C_SOURCE 199309L whatever that may mean...*/ 
/* #define _BSD_SOURCE             I use strdup, S_IFDIR, etc */ 

/* Roderich Schupp writes (bug #79100):
   /usr/include/dlfcn.h from libc6 2.2-5 defines RTLD_NEXT only
   when compiled with _GNU_SOURCE defined. Hence libfakeroot.c doesn't pick
   it
   up and does a dlopen("/lib/libc.so.6",...) in get_libc().
   This works most of the time, but explodes if you have an arch-optimized
   libc installed: the program now has two versions of libc.so
   (/lib/libc.so.6 and, say, /lib/i586/libc.so.6) mapped. Again for
   some programs you might get away with this, but running bash under
   fakeroot
   always bombs. Simple fix:
*/
#define _GNU_SOURCE 

#include "config.h"
#include "communicate.h"
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h> 
#include <dirent.h>
#include <errno.h>
#include <utime.h>
#include <glob.h>

#define MAXPATH 2048
static char *fakechroot_ptr;
static char *fakechroot_path;
static char fakechroot_buf[MAXPATH];

#define narrow_chroot_path(path) \
    { \
	fakechroot_path = getenv("FAKECHROOT_BASE"); \
	if (fakechroot_path != NULL) { \
	    fakechroot_ptr = strstr((path), fakechroot_path); \
	    if (fakechroot_ptr != (path)) { \
		(path) = NULL; \
	    } else { \
	        (path) = ((path) + strlen(fakechroot_path)); \
	    } \
	} \
    }

#define expand_chroot_path(path) \
    { \
	if (path != NULL && *path == '/') { \
    	    fakechroot_path = getenv("FAKECHROOT_BASE"); \
	    if (fakechroot_path != NULL) { \
		fakechroot_ptr = strstr((path), fakechroot_path); \
		if (fakechroot_ptr != (path)) { \
		    strcpy(fakechroot_buf, fakechroot_path); \
		    strcat(fakechroot_buf, path); \
		    path = fakechroot_buf; \
		} \
	    } \
	} \
    }


/* 
   Where are those shared libraries? 
   If I knew of a configure/libtool way to find that out, I'd use it. Or
   any other way other than the method I'm using below. Does anybody know
   how I can get that location? (BTW, symply linking a programme, and running
   `ldd' on it isn't the option, as Digital Unix doesn't have ldd)
*/


/* 
   Note that LIBCPATH isn't actually used on Linux or Solaris, as RTLD_NEXT
   is defined and we use that to get the `next_*' functions

   Linux:
*/

/* OSF1 :*/
/*#define LIBCPATH "/usr/shlib/libc.so"*/

#undef __xstat
#undef __fxstat
#undef __lxstat
#undef __xstat64
#undef __fxstat64
#undef __lxstat64
#undef _FILE_OFFSET_BITS

/*
// next_wrap_st:
// this structure is used in next_wrap, which is defined in
// wrapstruct.h, included below
*/
 
struct next_wrap_st{
  void **doit;
  char *name;
};

void *get_libc(){
 
#ifndef RTLD_NEXT
 void *lib=0;
 if(!lib){ 
   lib= dlopen(LIBCPATH,RTLD_LAZY);
 }
 if (NULL==lib) {
   fprintf(stderr, "Couldn't find libc at: %s\n", LIBCPATH);
   abort();
 }
 return lib;
#else
  return RTLD_NEXT;
#endif
}
void load_library_symbols(void);

int fakeroot_disabled = 0;

#include "wrapped.h"
#include "wraptmpf.h"
#include "wrapdef.h"
#include "wrapstruct.h"


void load_library_symbols(void){
  /* this function loads all original functions from the C library.
     I ran into problems when  each function individually
     loaded it's original counterpart, as RTLD_NEXT seems to have
     a different meaning in files with different names than libtricks.c
     (I.E, dlsym(RTLD_NEXT, ...) called in vsearch.c returned funtions
     defined in libtricks */
  /* The calling of this function itself is somewhat tricky:
     the awk script wrapawk generates several .h files. In wraptmpf.h
     there are temporary definitions for tmp_*, that do the call
     to this function. The other generated .h files do even more tricky
     things :) */

  static int done=0;
  int i;
  char* msg;
  
  if(!done){
    for(i=0; next_wrap[i].doit; i++){
      *(next_wrap[i].doit)=dlsym(get_libc(), next_wrap[i].name);
      if ( (msg = dlerror()) != NULL){
	fprintf (stderr, "dlsym(%s): %s\n", next_wrap[i].name, msg);
/*	abort ();*/
      }
    }
  }
}


/*
 * Fake implementations for the setuid family of functions.
 * The fake IDs are inherited by child processes via environment variables.
 *
 * Issues:
 *   o Privileges are not checked, which results in incorrect behaviour.
 *     Example: process changes its (real, effective and saved) uid to 1000
 *     and then tries to regain root privileges.  This should normally result
 *     in an EPERM, but our implementation doesn't care...
 *   o If one of the setenv calls fails, the state may get corrupted.
 *   o Not thread-safe.
 */


/* Generic set/get ID functions */

static int env_get_id(const char *key) {
  char *str = getenv(key);
  if (str)
    return atoi(str);
  return 0;
}

static int env_set_id(const char *key, int id) {
  if (id == 0) {
    unsetenv(key);
    return 0;
  } else {
    char str[12];
    snprintf(str, sizeof (str), "%d", id);
    return setenv(key, str, 1);
  }
}

static void read_id(unsigned int *id, const char *key) {
  if (*id == (unsigned int)-1)
    *id = env_get_id(key);
}

static int write_id(const char *key, int id) {
  if (env_get_id(key) != id)
    return env_set_id(key, id);
  return 0;
}

/* Fake ID storage */

static uid_t faked_real_uid = -1;
static gid_t faked_real_gid = -1;
static uid_t faked_effective_uid = -1;
static gid_t faked_effective_gid = -1;
static uid_t faked_saved_uid = -1;
static gid_t faked_saved_gid = -1;
static uid_t faked_fs_uid = -1;
static gid_t faked_fs_gid = -1;

/* Read user ID */

static void read_real_uid() {
  read_id(&faked_real_uid, FAKEROOTUID_ENV);
}

static void read_effective_uid() {
  read_id(&faked_effective_uid, FAKEROOTEUID_ENV);
}

static void read_saved_uid() {
  read_id(&faked_saved_uid, FAKEROOTSUID_ENV);
}

static void read_fs_uid() {
  read_id(&faked_fs_uid, FAKEROOTFUID_ENV);
}

static void read_uids() {
  read_real_uid();
  read_effective_uid();
  read_saved_uid();
  read_fs_uid();
}

/* Read group ID */

static void read_real_gid() {
  read_id(&faked_real_gid, FAKEROOTGID_ENV);
}

static void read_effective_gid() {
  read_id(&faked_effective_gid, FAKEROOTEGID_ENV);
}

static void read_saved_gid() {
  read_id(&faked_saved_gid, FAKEROOTSGID_ENV);
}

static void read_fs_gid() {
  read_id(&faked_fs_gid, FAKEROOTFGID_ENV);
}

static void read_gids() {
  read_real_gid();
  read_effective_gid();
  read_saved_gid();
  read_fs_gid();
}

/* Write user ID */

static int write_real_uid() {
  return write_id(FAKEROOTUID_ENV, faked_real_uid);
}

static int write_effective_uid() {
  return write_id(FAKEROOTEUID_ENV, faked_effective_uid);
}

static int write_saved_uid() {
  return write_id(FAKEROOTSUID_ENV, faked_saved_uid);
}

static int write_fs_uid() {
  return write_id(FAKEROOTFUID_ENV, faked_fs_uid);
}

static int write_uids() {
  if (write_real_uid() < 0)
    return -1;
  if (write_effective_uid() < 0)
    return -1;
  if (write_saved_uid() < 0)
    return -1;
  if (write_fs_uid() < 0)
    return -1;
  if (write_fs_uid() < 0)
    return -1;
  return 0;
}

/* Write group ID */

static int write_real_gid() {
  return write_id(FAKEROOTGID_ENV, faked_real_gid);
}

static int write_effective_gid() {
  return write_id(FAKEROOTEGID_ENV, faked_effective_gid);
}

static int write_saved_gid() {
  return write_id(FAKEROOTSGID_ENV, faked_saved_gid);
}

static int write_fs_gid() {
  return write_id(FAKEROOTFGID_ENV, faked_fs_gid);
}

static int write_gids() {
  if (write_real_gid() < 0)
    return -1;
  if (write_effective_gid() < 0)
    return -1;
  if (write_saved_gid() < 0)
    return -1;
  if (write_fs_gid() < 0)
    return -1;
  if (write_fs_gid() < 0)
    return -1;
  return 0;
}

/* Faked get functions */

static uid_t get_faked_uid() {
  read_real_uid();
  return faked_real_uid;
}

static gid_t get_faked_gid() {
  read_real_gid();
  return faked_real_gid;
}

static uid_t get_faked_euid() {
  read_effective_uid();
  return faked_effective_uid;
}

static gid_t get_faked_egid() {
  read_effective_gid();
  return faked_effective_gid;
}

static uid_t get_faked_suid() {
  read_saved_uid();
  return faked_saved_uid;
}

static gid_t get_faked_sgid() {
  read_saved_gid();
  return faked_saved_gid;
}

static uid_t get_faked_fsuid() {
  read_fs_uid();
  return faked_fs_uid;
}

static gid_t get_faked_fsgid() {
  read_fs_gid();
  return faked_fs_gid;
}

/* Faked set functions */

static int set_faked_uid(uid_t uid) {
  read_uids();
  if (faked_effective_uid == 0) {
    faked_real_uid = uid;
    faked_effective_uid = uid;
    faked_saved_uid = uid;
  } else {
    faked_effective_uid = uid;
  }
  faked_fs_uid = uid;
  return write_uids();
}

static int set_faked_gid(gid_t gid) {
  read_gids();
  if (faked_effective_gid == 0) {
    faked_real_gid = gid;
    faked_effective_gid = gid;
    faked_saved_gid = gid;
  } else {
    faked_effective_gid = gid;
  }
  faked_fs_gid = gid;
  return write_gids();
}

static int set_faked_euid(uid_t euid) {
  read_effective_uid();
  faked_effective_uid = euid;
  read_fs_uid();
  faked_fs_uid = euid;
  if (write_effective_uid() < 0)
    return -1;
  if (write_fs_uid() < 0)
    return -1;
  return 0;
}

static int set_faked_egid(gid_t egid) {
  read_effective_gid();
  faked_effective_gid = egid;
  read_fs_gid();
  faked_fs_gid = egid;
  if (write_effective_gid() < 0)
    return -1;
  if (write_fs_gid() < 0)
    return -1;
  return 0;
}

static int set_faked_reuid(uid_t ruid, uid_t euid) {
  read_uids();
  if (ruid != (uid_t)-1 || euid != (uid_t)-1)
    faked_saved_uid = faked_effective_uid;
  if (ruid != (uid_t)-1)
    faked_real_uid = ruid;
  if (euid != (uid_t)-1)
    faked_effective_uid = euid;
  faked_fs_uid = faked_effective_uid;
  return write_uids();
}

static int set_faked_regid(gid_t rgid, gid_t egid) {
  read_gids();
  if (rgid != (gid_t)-1 || egid != (gid_t)-1)
    faked_saved_gid = faked_effective_gid;
  if (rgid != (gid_t)-1)
    faked_real_gid = rgid;
  if (egid != (gid_t)-1)
    faked_effective_gid = egid;
  faked_fs_gid = faked_effective_gid;
  return write_gids();
}

#ifdef HAVE_SETRESUID
static int set_faked_resuid(uid_t ruid, uid_t euid, uid_t suid) {
  read_uids();
  if (ruid != (uid_t)-1)
    faked_real_uid = ruid;
  if (euid != (uid_t)-1)
    faked_effective_uid = euid;
  if (suid != (uid_t)-1)
    faked_saved_uid = suid;
  faked_fs_uid = faked_effective_uid;
  return write_uids();
}
#endif

#ifdef HAVE_SETRESGID
static int set_faked_resgid(gid_t rgid, gid_t egid, gid_t sgid) {
  read_gids();
  if (rgid != (gid_t)-1)
    faked_real_gid = rgid;
  if (egid != (gid_t)-1)
    faked_effective_gid = egid;
  if (sgid != (gid_t)-1)
    faked_saved_gid = sgid;
  faked_fs_gid = faked_effective_gid;
  return write_gids();
}
#endif

#ifdef HAVE_SETFSUID
static uid_t set_faked_fsuid(uid_t fsuid) {
  uid_t prev_fsuid = get_faked_fsuid();
  faked_fs_uid = fsuid;
  return prev_fsuid;
}
#endif

#ifdef HAVE_SETFSGID
static gid_t set_faked_fsgid(gid_t fsgid) {
  gid_t prev_fsgid = get_faked_fsgid();
  faked_fs_gid = fsgid;
  return prev_fsgid;
}
#endif


static int dont_try_chown(){
  static int inited=0;
  static int donttry;

  if(!inited){
    donttry=(env_var_set(FAKEROOTDONTTRYCHOWN_ENV)!=NULL);
    inited=1;
  }
  return 1;
}


/* The wrapped functions */


int WRAP_LSTAT LSTAT_ARG(int ver, 
		       const char *file_name, 
		       struct stat *statbuf){

  int r;

  expand_chroot_path(file_name);
  r=NEXT_LSTAT(ver, file_name, statbuf);
  if(r)
    return -1;
  send_get_stat(statbuf);
  return 0;
}


int WRAP_STAT STAT_ARG(int ver, 
		       const char *file_name, 
		       struct stat *st){
  int r;
  expand_chroot_path(file_name);
  r=NEXT_STAT(ver, file_name, st);
  if(r)
    return -1;
  send_get_stat(st);
  return 0;
}


int WRAP_FSTAT FSTAT_ARG(int ver, 
			int fd, 
			struct stat *st){


  int r;

  r=NEXT_FSTAT(ver, fd, st);
  if(r)
    return -1;
  send_get_stat(st);
  return 0;
}


#ifdef STAT64_SUPPORT

int WRAP_LSTAT64 LSTAT64_ARG (int ver, 
			   const char *file_name, 
			   struct stat64 *st){

  int r;
  expand_chroot_path(file_name);

  r=NEXT_LSTAT64(ver, file_name, st);

  if(r)
    return -1;

  send_get_stat64(st);

  return 0;
}


int WRAP_STAT64 STAT64_ARG(int ver, 
			   const char *file_name, 
			   struct stat64 *st){
  int r;
  expand_chroot_path(file_name);

  r=NEXT_STAT64(ver,file_name,st);
  if(r)
    return -1;
  send_get_stat64(st);
  return 0;
}


int WRAP_FSTAT64 FSTAT64_ARG(int ver, 
			     int fd, 
			     struct stat64 *st){
  int r;

  r=NEXT_FSTAT64(ver, fd, st);
  if(r)
    return -1;
  send_get_stat64(st);

  return 0;
}

#endif




/*************************************************************/
/*
  Wrapped functions general info:
  
  In general, the structure is as follows:
    - Then, if the function does things that (possibly) fail by
      other users than root, allow for `fake' root privileges.
      Do this by obtaining the inode the function works on, and then
      informing faked (the deamon that remembers all `fake' file
      permissions e.d.) about the intentions of the user.
      Faked maintains a list of inodes and their respective
      `fake' ownerships/filemodes.
    - Or, if the function requests information that we should
      fake, again get the inode of the file, and ask faked about the
      ownership/filemode data it maintains for that inode.

*/
/*************************************************************/



/* chown, lchown, fchown, chmod, fchmod, mknod functions
   
   quite general. See the `Wrapped functions general info:' above
   for more info.
 */

int chown(const char *path, uid_t owner, gid_t group){
  struct stat st;
  int r=0;

  expand_chroot_path(path);

#ifdef LCHOWN_SUPPORT
  /*chown(sym-link) works on the target of the symlink if lchown is
    present and enabled.*/
  r=NEXT_STAT(_STAT_VER, path, &st);
#else
  /*chown(sym-link) works on the symlink itself, use lstat: */
  r=NEXT_LSTAT(_STAT_VER, path, &st);
#endif
  
  if(r)
    return r;
  st.st_uid=owner;
  st.st_gid=group;
  send_stat(&st,chown_func);
  if(!dont_try_chown())
    r=next_lchown(path,owner,group);
  else
    r=0;
  if(r&&(errno==EPERM))
    r=0;
  
  return r;
}


#ifdef LCHOWN_SUPPORT
int lchown(const char *path, uid_t owner, gid_t group){
  struct stat st;
  int r=0;
  expand_chroot_path(path);

  r=NEXT_LSTAT(_STAT_VER, path, &st);
  if(r)
    return r;
  st.st_uid=owner;
  st.st_gid=group;
  send_stat(&st,chown_func);
  if(!dont_try_chown())
    r=next_lchown(path,owner,group);
  else
    r=0;
  if(r&&(errno==EPERM))
    r=0;

  return r;
}
#endif

int fchown(int fd, uid_t owner, gid_t group){
  struct stat st;
  int r;

  r=NEXT_FSTAT(_STAT_VER, fd, &st);
  if(r)
    return r;
  
  st.st_uid=owner;
  st.st_gid=group;
  send_stat(&st, chown_func);
  
  if(!dont_try_chown())
    r=next_fchown(fd,owner,group);
  else
    r=0;
  
  if(r&&(errno==EPERM))
    r=0;
  
  return r;
}

int chmod(const char *path, mode_t mode){
  struct stat st;
  int r;

  expand_chroot_path(path);

  r=NEXT_STAT(_STAT_VER, path, &st);
  if(r)
    return r;
  
  st.st_mode=(mode&ALLPERMS)|(st.st_mode&~ALLPERMS);
    
  send_stat(&st, chmod_func);
  
  /* if a file is unwritable, then root can still write to it
     (no matter who owns the file). If we are fakeroot, the only
     way to fake this is to always make the file writable, readable
     etc for the real user (who started fakeroot). Also holds for
     the exec bit of directories.
     Yes, packages requering that are broken. But we have lintian
     to get rid of broken packages, not fakeroot.
  */
  mode |= 0600;
  if(S_ISDIR(st.st_mode))
    mode |= 0100;
  
  r=next_chmod(path, mode);
  if(r&&(errno==EPERM))
    r=0;
  return r;
}

int fchmod(int fd, mode_t mode){
  int r;
  struct stat st;


  r=NEXT_FSTAT(_STAT_VER, fd, &st);
  
  if(r)
    return(r);
  
  st.st_mode=(mode&ALLPERMS)|(st.st_mode&~ALLPERMS);
  send_stat(&st,chmod_func);  
  
  /* see chmod() for comment */
  mode |= 0600;
  if(S_ISDIR(st.st_mode))
    mode |= 0100;
  
  r=next_fchmod(fd, mode);
  if(r&&(errno==EPERM))
    r=0;
  return r;
}

int WRAP_MKNOD MKNOD_ARG(int ver UNUSED,
			 const char *pathname, 
			 mode_t mode, dev_t XMKNOD_FRTH_ARG dev)
{
  struct stat st;
  mode_t old_mask=umask(022);
  int fd,r;

  umask(old_mask);
  
  /*Don't bother to mknod the file, that probably doesn't work.
    just create it as normal file, and leave the premissions
    to the fakemode.*/
  expand_chroot_path(pathname);
  
  fd=open(pathname, O_WRONLY|O_CREAT|O_TRUNC, 00644);

  if(fd==-1)
    return -1;
  
  close(fd);
  /* get the inode, to communicate with faked */

  r=NEXT_LSTAT(_STAT_VER, pathname, &st);

  if(r)
    return -1;
  
  st.st_mode= mode & ~old_mask;
  st.st_rdev= XMKNOD_FRTH_ARG dev;
  
  send_stat(&st,mknod_func);
    
  return 0;
}

int mkdir(const char *path, mode_t mode){
  struct stat st;
  int r;
  mode_t old_mask=umask(022);

  umask(old_mask);


  /* we need to tell the fake deamon the real mode. In order
     to communicate with faked we need a struct stat, so we now
     do a stat of the new directory (just for the inode/dev) */
  expand_chroot_path(path);

  r=next_mkdir(path, mode|0700); 
  /* mode|0700: see comment in the chown() function above */
  if(r)
    return -1;
  r=NEXT_STAT(_STAT_VER, path, &st);
  
  if(r)
    return -1;
  
  st.st_mode=(mode&~old_mask&ALLPERMS)|(st.st_mode&~ALLPERMS)|S_IFDIR;

  send_stat(&st, chmod_func);

  return 0;
}

/* 
   The remove funtions: unlink, rmdir, rename.
   These functions can all remove inodes from the system.
   I need to inform faked about the removal of these inodes because
   of the following:
    # rm -f file
    # touch file 
    # chown admin file
    # rm file
    # touch file
   In the above example, assuming that for both touch-es, the same
   inode is generated, faked will still report the owner of `file'
   as `admin', unless it's informed about the removal of the inode.   
*/

int unlink(const char *pathname){
  int r;
#ifdef STAT64_SUPPORT
  struct stat64 st;
  expand_chroot_path(pathname);

  r=NEXT_LSTAT64(_STAT_VER, pathname, &st);
#else
  struct stat st;
  expand_chroot_path(pathname);

  r=NEXT_LSTAT(_STAT_VER, pathname, &st);
#endif
  if(r)
    return -1;

  r=next_unlink(pathname);  

  if(r)
    return -1;
  
#ifdef STAT64_SUPPORT
  send_stat64(&st, unlink_func);
#else
  send_stat(&st, unlink_func);
#endif
  
  return 0;
}

/*
  See the `remove funtions:' comments above for more info on
  these remove function wrappers.
*/
int rmdir(const char *pathname){
  int r;
  struct stat st;
  expand_chroot_path(pathname);

  r=NEXT_LSTAT(_STAT_VER, pathname, &st);
  if(r)
    return -1;
  r=next_rmdir(pathname);  
  if(r)
    return -1;

  send_stat(&st,unlink_func);  

  return 0;
}

/*
  See the `remove funtions:' comments above for more info on
  these remove function wrappers.
*/
int remove(const char *pathname){
  int r;
  struct stat st;
  expand_chroot_path(pathname);

  r=NEXT_LSTAT(_STAT_VER, pathname, &st);
  if(r)
    return -1;
  r=next_remove(pathname);  
  if(r)
    return -1;
  send_stat(&st,unlink_func);  
  
  return r;
}

/*
  if the second argument to the rename() call points to an
  existing file, then that file will be removed. So, we have
  to treat this function as one of the `remove functions'.

  See the `remove funtions:' comments above for more info on
  these remove function wrappers.
*/

int rename(const char *oldpath, const char *newpath){
  int r,s;
  struct stat st;     
  char tmp[MAXPATH];

  /* If newpath points to an existing file, that file will be 
     unlinked.   Make sure we tell the faked daemon about this! */

  /* we need the st_new struct in order to inform faked about the
     (possible) unlink of the file */


  expand_chroot_path(oldpath);
  strcpy(tmp, oldpath); oldpath=tmp;
  expand_chroot_path(newpath);

  r=NEXT_LSTAT(_STAT_VER, newpath, &st);


  s=next_rename(oldpath, newpath);
  if(s)
    return s;
  if(!r)
    send_stat(&st,unlink_func);

  return 0;
}

#ifdef FAKEROOT_FAKENET
pid_t fork(void)
{
  pid_t pid;

  pid = next_fork();

  if (pid == 0)
    close_comm_sd();

  return pid;
}

pid_t vfork(void)
{
  /* We can't wrap vfork(2) without breaking everything... */
  return fork();
}

#endif /* FAKEROOT_FAKENET */
uid_t getuid(void){
  if (fakeroot_disabled)
    return next_getuid();
  return get_faked_uid();
}

uid_t geteuid(void){
  if (fakeroot_disabled)
    return next_geteuid();
  return get_faked_euid();
}

#ifdef HAVE_GETRESUID
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid){
  if (fakeroot_disabled)
    return next_getresuid(ruid, euid, suid);
  *ruid = get_faked_uid();
  *euid = get_faked_euid();
  *suid = get_faked_suid();
  return 0;
}
#endif /* HAVE_GETRESUID */

uid_t getgid(void){
  if (fakeroot_disabled)
    return next_getgid();
  return get_faked_gid();
}

uid_t getegid(void){
  if (fakeroot_disabled)
    return next_getegid();
  return get_faked_egid();
}

#ifdef HAVE_GETRESGID
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid){
  if (fakeroot_disabled)
    return next_getresgid(rgid, egid, sgid);
  *rgid = get_faked_gid();
  *egid = get_faked_egid();
  *sgid = get_faked_sgid();
  return 0;
}
#endif /* HAVE_GETRESGID */

int setuid(uid_t id){
  if (fakeroot_disabled)
    return next_setuid(id);
  return set_faked_uid(id);
}

int setgid(uid_t id){
  if (fakeroot_disabled)
    return next_setgid(id);
  return set_faked_gid(id);
}

int seteuid(uid_t id){
  if (fakeroot_disabled)
    return next_seteuid(id);
  return set_faked_euid(id);
}

int setegid(uid_t id){
  if (fakeroot_disabled)
    return next_setegid(id);
  return set_faked_egid(id);
}

int setreuid(SETREUID_ARG ruid, SETREUID_ARG euid){
  if (fakeroot_disabled)
    return next_setreuid(ruid, euid);
  return set_faked_reuid(ruid, euid);
}

int setregid(SETREGID_ARG rgid, SETREGID_ARG egid){
  if (fakeroot_disabled)
    return next_setregid(rgid, egid);
  return set_faked_regid(rgid, egid);
}

#ifdef HAVE_SETRESUID
int setresuid(uid_t ruid, uid_t euid, uid_t suid){
  if (fakeroot_disabled)
    return next_setresuid(ruid, euid, suid);
  return set_faked_resuid(ruid, euid, suid);
}
#endif /* HAVE_SETRESUID */

#ifdef HAVE_SETRESGID
int setresgid(gid_t rgid, gid_t egid, gid_t sgid){
  if (fakeroot_disabled)
    return next_setresgid(rgid, egid, sgid);
  return set_faked_resgid(rgid, egid, sgid);
}
#endif /* HAVE_SETRESGID */

#ifdef HAVE_SETFSUID
uid_t setfsuid(uid_t fsuid){
  if (fakeroot_disabled)
    return next_setfsuid(fsuid);
  return set_faked_fsuid(fsuid);
}
#endif /* HAVE_SETFSUID */

#ifdef HAVE_SETFSGID
gid_t setfsgid(gid_t fsgid){
  if (fakeroot_disabled)
    return next_setfsgid(fsgid);
  return set_faked_fsgid(fsgid);
}
#endif /* HAVE_SETFSGID */

int initgroups(const char* user, INITGROUPS_SECOND_ARG group){
  if (fakeroot_disabled)
    return next_initgroups(user, group);
  else
    return 0;
}

int setgroups(SETGROUPS_SIZE_TYPE size, const gid_t *list){
  if (fakeroot_disabled)
    return next_setgroups(size, list);
  else
    return 0;
}

int chroot(const char *path) {
    char *ptr;
    int status;

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) {
	return EACCES;
    }
    
    if ((status = chdir(path)) != 0) {
	return status;
    }

    ptr = rindex(path, 0);
    if (ptr > path) {
	ptr--;
	while (*ptr == '/') {
	    *ptr-- = 0;
	}
    }

    setenv("FAKECHROOT_BASE", path, 1);
    fakechroot_path = getenv("FAKECHROOT_BASE");
    chdir("/");
    return 0;
}

int chdir(const char *path) {
    expand_chroot_path(path);
    return next_chdir(path);
}

char *getcwd(char *buf, size_t size) {
    char *cwd;

    if ((cwd = next_getcwd(buf, size)) == NULL) {
	return NULL;
    }
    narrow_chroot_path(cwd);
    return cwd;
}

char *getwd(char *buf) {
    char *cwd;

    if ((cwd = next_getwd(buf)) == NULL) {
	return NULL;
    }
    narrow_chroot_path(cwd);
    return cwd;
}

char *get_current_dir_name(void) {
    char *cwd, *oldptr, *newptr;

    if ((cwd = next_get_current_dir_name()) == NULL) {
	return NULL;
    }
    oldptr = cwd;
    narrow_chroot_path(cwd);
    if (cwd == NULL) {
        return NULL;
    }
    if ((newptr = malloc(strlen(cwd)+1)) == NULL) {
	free(oldptr);
	return NULL;
    }
    strcpy(newptr, cwd);
    free(oldptr);
    return newptr;
}

int open(const char *pathname, int flags, ...) {
    struct stat st;
    int mode = 0;
    int r;
    int ret = -1;
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    ret =  next_open(pathname, flags, mode);
    r=NEXT_LSTAT(_STAT_VER, pathname, &st);
    if(!r)
        send_stat(&st,chmod_func);
    return ret;
}

int creat(const char *pathname, mode_t mode) {
    expand_chroot_path(pathname);
    return next_creat(pathname, mode);
}

int open64(const char *pathname, int flags, ...) {
    struct stat st;
    int mode = 0;
    int r;
    int ret = -1;
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    ret =  next_open64(pathname, flags, mode);
    r=NEXT_LSTAT(_STAT_VER, pathname, &st);
    if(!r)
        send_stat(&st,chmod_func);
    return ret;
}

int creat64(const char *pathname, mode_t mode) {
    struct stat st;
    int ret,r;
    expand_chroot_path(pathname);
    ret = next_creat64(pathname, mode);
    r=NEXT_LSTAT(_STAT_VER, pathname, &st);
    if(!r)
        send_stat(&st,chmod_func);
    return ret;

}

DIR *opendir(const char *name) {
    expand_chroot_path(name);
    return next_opendir(name);
}

int utime(const char *filename, const struct utimbuf *buf) {
    expand_chroot_path(filename);
    return next_utime(filename, buf);
}

int utimes(char *filename, struct timeval *tvp) {
    expand_chroot_path(filename);
    return next_utimes(filename, tvp);
}

int symlink(const char *oldpath, const char *newpath) {
    struct stat st;
    char tmp[MAXPATH];
    int ret,r;
    expand_chroot_path(oldpath);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath);
    ret = next_symlink(oldpath, newpath);
    r=NEXT_LSTAT(_STAT_VER, newpath, &st);
    if(!r)
        send_stat(&st,chmod_func);
    return ret;
}

int link(const char *oldpath, const char *newpath) {
    struct stat st;
    char tmp[MAXPATH];
    int ret,r;
    expand_chroot_path(oldpath);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath);
    ret = next_link(oldpath, newpath);
    r=NEXT_LSTAT(_STAT_VER, newpath, &st);
    if(!r)
        send_stat(&st,chmod_func);
    return ret;
}

int readlink(const char *path, char *buf, size_t bufsiz) {
    int status;
    char tmp[MAXPATH], *tmpptr;

    expand_chroot_path(path);

    if ((status = next_readlink(path, tmp, bufsiz)) == -1) {
	return status;
    }
    /* TODO: shouldn't end with \000 */
    tmp[status] = 0;

    fakechroot_path = getenv("FAKECHROOT_BASE");
    if (fakechroot_path != NULL) { 
	fakechroot_ptr = strstr(tmp, fakechroot_path);
        if (fakechroot_ptr != tmp) {
	    tmpptr = tmp;
	} else {
	    tmpptr = tmp + strlen(fakechroot_path);
	}
	strcpy(buf, tmpptr);
    } else {
	strcpy(buf, tmp);
    }
    return strlen(buf);
}

FILE *fopen(const char *path, const char *mode) {
    expand_chroot_path(path);
    return next_fopen(path, mode);
}

FILE *freopen(const char *path, const char *mode, FILE *stream) {
    expand_chroot_path(path);
    return next_freopen(path, mode, stream);
}

char *mktemp(char *template) {
    expand_chroot_path(template);
    return next_mktemp(template);
}

int mkstemp(char *template) {
    struct stat st;
    char tmp[MAXPATH], *oldtemplate, *ptr;
    int fd,r;

    oldtemplate = template;
    
    expand_chroot_path(template);
    if ((fd = next_mkstemp(template)) == -1) {
	return -1;
    }
    r=NEXT_FSTAT(_STAT_VER, fd, &st);
    if(!r)
        send_stat(&st,chmod_func);
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr);
    if (ptr != NULL) {
        strcpy(oldtemplate, ptr);
    }
    return fd;
}

int mkstemp64(char *template) {
    struct stat st;
    char tmp[MAXPATH], *oldtemplate, *ptr;
    int fd,r;

    oldtemplate = template;
    
    expand_chroot_path(template);
    if ((fd = next_mkstemp64(template)) == -1) {
	return -1;
    }
    r=NEXT_FSTAT(_STAT_VER, fd, &st);
    if(!r)
        send_stat(&st,chmod_func);
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr);
    if (ptr != NULL) {
        strcpy(oldtemplate, ptr);
    }
    return fd;
}

char *mkdtemp(char *template) {
    struct stat st;
    char tmp[MAXPATH], *oldtemplate, *ptr;
    int r;

    oldtemplate = template;
    
    expand_chroot_path(template);
    if (next_mkdtemp(template) == NULL) {
	return NULL;
    }
    r=NEXT_STAT(_STAT_VER, template, &st);
    if(!r)
        send_stat(&st,chmod_func);
    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr);
    if (ptr == NULL) {
	return NULL;
    }
    strcpy(oldtemplate, ptr);
    return oldtemplate;
}

char *tempnam(const char *dir, const char *pfx) {
    expand_chroot_path(dir);
    return next_tempnam(dir, pfx);
}

char *tmpnam(char *s) {
    static char buf[MAXPATH];
    char *ptr;
    
    if (s != NULL) {
	return next_tmpnam(s);
    }
    
    ptr = next_tmpnam(NULL);
    expand_chroot_path(ptr);
    strcpy(buf, ptr);
    return buf;
}

int mkfifo(const char *pathname, mode_t mode) {
    expand_chroot_path(pathname);
    return next_mkfifo(pathname, mode);
}

int execv(const char *path, char *const argv []) {
    return execve (path, argv, environ);
}

int execvp(const char *file, char *const argv []) {
  if (*file == '\0')
    {
      /* We check the simple case first. */
      errno = ENOENT;
      return -1;
    }

  if (strchr (file, '/') != NULL)
    {
      /* Don't search when it contains a slash.  */
      return execve (file, argv, environ);
    }
  else
    {
      int got_eacces = 0;
      char *path, *p, *name;
      size_t len;
      size_t pathlen;

      path = getenv ("PATH");
      if (path == NULL)
	{
	  /* There is no `PATH' in the environment.
	     The default search path is the current directory
	     followed by the path `confstr' returns for `_CS_PATH'.  */
	  len = confstr (_CS_PATH, (char *) NULL, 0);
	  path = (char *) alloca (1 + len);
	  path[0] = ':';
	  (void) confstr (_CS_PATH, path + 1, len);
	}

      len = strlen (file) + 1;
      pathlen = strlen (path);
      name = alloca (pathlen + len + 1);
      /* Copy the file name at the top.  */
      name = (char *) memcpy (name + pathlen + 1, file, len);
      /* And add the slash.  */
      *--name = '/';

      p = path;
      do
	{
	  char *startp;

	  path = p;
	  p = strchrnul (path, ':');

	  if (p == path)
	    /* Two adjacent colons, or a colon at the beginning or the end
	       of `PATH' means to search the current directory.  */
	    startp = name + 1;
	  else
	    startp = (char *) memcpy (name - (p - path), path, p - path);

	  /* Try to execute this name.  If it works, execv will not return.  */
	  execve (startp, argv, environ);

	  switch (errno)
	    {
	    case EACCES:
	      /* Record the we got a `Permission denied' error.  If we end
		 up finding no executable we can use, we want to diagnose
		 that we did find one but were denied access.  */
	      got_eacces = 1;
	    case ENOENT:
	    case ESTALE:
	    case ENOTDIR:
	      /* Those errors indicate the file is missing or not executable
		 by us, in which case we want to just try the next path
		 directory.  */
	      break;

	    default:
	      /* Some other error means we found an executable file, but
		 something went wrong executing it; return the error to our
		 caller.  */
	      return -1;
	    }
	}
      while (*p++ != '\0');

      /* We tried every element and none of them worked.  */
      if (got_eacces)
	/* At least one failure was due to permissions, so report that
           error.  */
	errno = EACCES;
    }

  /* Return the error from the last attempt (probably ENOENT).  */
  return -1;
}

int execve(const char *filename, char *const argv [], char *const envp[]) {
    int file;
    char hashbang[2048];
    size_t argv_max = 1024;
    const char **newargv = alloca (argv_max * sizeof (const char *));
    char tmp[2048], newfilename[2048], argv0[2048], *ptr;
    unsigned int i, j, n;
    char c;
    
    expand_chroot_path(filename);
    strcpy(tmp, filename);
    filename = tmp;

    if ((file = open(filename, O_RDONLY)) == -1) {
	errno = ENOENT;
	return -1;
    }
    
    i = read(file, hashbang, 2048-2);
    close(file);
    if (i == -1) {
	errno = ENOENT;
	return -1;
    }
    
    if (hashbang[0] != '#' || hashbang[1] != '!')
	return next_execve(filename, argv, envp);

    hashbang[i] = hashbang[i+1] = 0;
    for (i = j = 2; (hashbang[i] == ' ' || hashbang[i] == '\t') && i < 2048; i++, j++);
    for (n = 0; i < 2048; i++) {
	c = hashbang[i];
	if (hashbang[i] == 0 || hashbang[i] == ' ' || hashbang[i] == '\t' || hashbang[i] == '\n') {
	    hashbang[i] = 0;
	    if (i > j) {
		if (n == 0) {
		    ptr = &hashbang[j];
		    expand_chroot_path(ptr);
		    strcpy(newfilename, ptr);
		    strcpy(argv0, &hashbang[j]);
		    newargv[n++] = argv0;
		} else {
	    	    newargv[n++] = &hashbang[j];
		}
	    }
	    j = i + 1;
	}
	if (c == '\n' || c == 0)
	    break;
    }

    expand_chroot_path(filename);
    newargv[n++] = filename;

    for (i = 1; argv[i] != NULL && i<argv_max; ) {
        newargv[n++] = argv[i++];
    }
    
    newargv[n] = 0;
    
    return next_execve(newfilename, (char *const *)newargv, envp);
}

int execl (const char *path, const char *arg, ...) {
  size_t argv_max = 1024;
  const char **argv = alloca (argv_max * sizeof (const char *));
  unsigned int i;
  va_list args;

  argv[0] = arg;

  va_start (args, arg);
  i = 0;
  while (argv[i++] != NULL)
    {
      if (i == argv_max)
	{
	  const char **nptr = alloca ((argv_max *= 2) * sizeof (const char *));

	    if ((char *) argv + i == (char *) nptr)
	    /* Stack grows up.  */
	    argv_max += i;
	  else
	    /* We have a hole in the stack.  */
	    argv = (const char **) memcpy (nptr, argv,
					   i * sizeof (const char *));
	}

      argv[i] = va_arg (args, const char *);
    }
  va_end (args);

  return execve (path, (char *const *) argv, environ);
}

int execlp (const char *file, const char *arg, ...) {
  size_t argv_max = 1024;
  const char **argv = alloca (argv_max * sizeof (const char *));
  unsigned int i;
  va_list args;

  argv[0] = arg;

  va_start (args, arg);
  i = 0;
  while (argv[i++] != NULL)
    {
      if (i == argv_max)
	{
	  const char **nptr = alloca ((argv_max *= 2) * sizeof (const char *));

	    if ((char *) argv + i == (char *) nptr)
	    /* Stack grows up.  */
	    argv_max += i;
	  else
	    /* We have a hole in the stack.  */
	    argv = (const char **) memcpy (nptr, argv,
					   i * sizeof (const char *));
	}

      argv[i] = va_arg (args, const char *);
    }
  va_end (args);

  expand_chroot_path(file);
  return next_execvp (file, (char *const *) argv);
}

int execle (const char *path, const char *arg, ...) {
  size_t argv_max = 1024;
  const char **argv = alloca (argv_max * sizeof (const char *));
  const char *const *envp;
  unsigned int i;
  va_list args;
  argv[0] = arg;

  va_start (args, arg);
  i = 0;
  while (argv[i++] != NULL)
    {
      if (i == argv_max)
	{
	  const char **nptr = alloca ((argv_max *= 2) * sizeof (const char *));

	    if ((char *) argv + i == (char *) nptr)
	    /* Stack grows up.  */
	    argv_max += i;
	  else
	    /* We have a hole in the stack.  */
	    argv = (const char **) memcpy (nptr, argv,
					   i * sizeof (const char *));
	}

      argv[i] = va_arg (args, const char *);
    }

  envp = va_arg (args, const char *const *);
  va_end (args);

  return execve (path, (char *const *) argv, (char *const *) envp);
}

int access(const char *pathname, int mode) {
    expand_chroot_path(pathname);
    return next_access(pathname, mode);
}

FILE *fopen64(const char *path, const char *mode) {
    expand_chroot_path(path);
    return next_fopen64(path, mode);
}

FILE *freopen64(const char *path, const char *mode, FILE *stream) {
    expand_chroot_path(path);
    return next_freopen64(path, mode, stream);
}

int truncate(const char *path, off_t length) {
    expand_chroot_path(path);
    return next_truncate(path, length);
}

int truncate64(const char *path, off64_t length) {
    expand_chroot_path(path);
    return next_truncate(path, length);
}

void *dlopen(const char *filename, int flag) {
    expand_chroot_path(filename);
    return next_dlopen(filename, flag);
}

int __open(const char *pathname, int flags, ...) {
    struct stat st;
    int mode = 0;
    expand_chroot_path(pathname);

return -1;
    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }
    if (flags & O_CREAT) {
	int r;
	r=NEXT_LSTAT(_STAT_VER, pathname, &st);
	if(!r)
	    send_stat(&st,chmod_func);
    }

    return next___open(pathname, flags, mode);
}

int __open64(const char *pathname, int flags, ...) {
    struct stat st;
    int mode = 0;
    int ret;
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }
    ret = next___open64(pathname, flags, mode);
    if (flags & O_CREAT) {
	int r;
	r=NEXT_LSTAT(_STAT_VER, pathname, &st);
	if(!r)
	    send_stat(&st,chmod_func);
    }
    return ret; 
}

int lckpwdf() {
    return 0;
}

int ulckpwdf() {
    return 0;
}

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    expand_chroot_path(path);
    return next_setxattr(path, name, value, size, flags);
}

int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags) {
    expand_chroot_path(path);
    return next_lsetxattr(path, name, value, size, flags);
}

ssize_t getxattr(const char *path, const char *name, void *value, size_t size) {
    expand_chroot_path(path);
    return next_getxattr(path, name, value, size);
}

ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size) {
    expand_chroot_path(path);
    return next_lgetxattr(path, name, value, size);
}

ssize_t listxattr(const char *path, char *list, size_t size) {
    expand_chroot_path(path);
    return next_listxattr(path, list, size);
}

ssize_t llistxattr(const char *path, char *list, size_t size) {
    expand_chroot_path(path);
    return next_llistxattr(path, list, size);
}

int removexattr(const char *path, const char *name) {
    expand_chroot_path(path);
    return next_removexattr(path, name);
}

int lremovexattr(const char *path, const char *name) {
    expand_chroot_path(path);
    return next_lremovexattr(path, name);
}

int glob(const char *pattern, int flags, int (*errfunc) (const char *, int), glob_t *pglob)
{
    int rc,i;
    char tmp[MAXPATH], *tmpptr;

    expand_chroot_path(pattern);
    rc = next_glob(pattern,flags,errfunc,pglob);
    if (rc < 0)
	return rc;
    for(i = 0;i < pglob->gl_pathc;i++)
     {
       strcpy(tmp,pglob->gl_pathv[i]);
       fakechroot_path = getenv("FAKECHROOT_BASE");
       if (fakechroot_path != NULL) { 
	 fakechroot_ptr = strstr(tmp, fakechroot_path);
         if (fakechroot_ptr != tmp) {
	    tmpptr = tmp;
	 } else {
	    tmpptr = tmp + strlen(fakechroot_path);
	 }
	 strcpy(pglob->gl_pathv[i], tmpptr);
       }
     }
    return rc;
}

int glob64(const char *pattern, int flags, int (*errfunc) (const char *, int), glob64_t *pglob)
{
    int rc,i;
    char tmp[MAXPATH], *tmpptr;

    expand_chroot_path(pattern);
    rc = next_glob64(pattern,flags,errfunc,pglob);
    if (rc < 0)
	return rc;
    for(i = 0;i < pglob->gl_pathc;i++)
     {
       strcpy(tmp,pglob->gl_pathv[i]);
       fakechroot_path = getenv("FAKECHROOT_BASE");
       if (fakechroot_path != NULL) { 
	 fakechroot_ptr = strstr(tmp, fakechroot_path);
         if (fakechroot_ptr != tmp) {
	    tmpptr = tmp;
	 } else {
	    tmpptr = tmp + strlen(fakechroot_path);
	 }
	 strcpy(pglob->gl_pathv[i], tmpptr);
       }
     }
    return rc;
}

int glob_pattern_p(const char *pattern, int quote)
{
    expand_chroot_path(pattern);
    return next_glob_pattern_p(pattern, quote);
}

int scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *))
{
    expand_chroot_path(dir);
    return next_scandir(dir, namelist, filter, compar);
}

int scandir64(const char *dir, struct dirent64 ***namelist, int(*filter)(const struct dirent64 *), int(*compar)(const void *, const void *))
{
    expand_chroot_path(dir);
    return next_scandir(dir, namelist, filter, compar);
}

int fakeroot_disable(int new)
{
  int old = fakeroot_disabled;
  fakeroot_disabled = new ? 1 : 0;
  return old;
}

int fakeroot_isdisabled(void)
{
  return fakeroot_disabled;
}
