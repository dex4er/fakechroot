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


/* a few functions that probe environment variables once, and remember
   the results
*/

static uid_t faked_uid(){
  static int inited=0;
  static int uid;
  const char *s; 

  if(!inited){
    if((s=env_var_set(FAKEROOTUID_ENV)))
      uid=atoi(s);
    else
      uid=0;
    inited=1;
  }
  return uid;    
}
static uid_t faked_gid(){
  static int inited=0;
  static int gid;
  const char *s;

  if(!inited){
    if((s=env_var_set(FAKEROOTGID_ENV)))
      gid=atoi(s);
    else
      gid=0;
    inited=1;
  }
  return gid;    
}

static uid_t faked_euid(){
  static int inited=0;
  static int uid;
  const char *s; 

  if(!inited){
    if((s=env_var_set(FAKEROOTEUID_ENV)))
      uid=atoi(s);
    else
      uid=0;
    inited=1;
  }
  return uid;    
}
static uid_t faked_egid(){
  static int inited=0;
  static int gid;
  const char *s;

  if(!inited){
    if((s=env_var_set(FAKEROOTEGID_ENV)))
      gid=atoi(s);
    else
      gid=0;
    inited=1;
  }
  return gid;    
}

static int dont_try_chown(){
  static int inited=0;
  static int donttry;

  if(!inited){
    donttry=(env_var_set(FAKEROOTDONTTRYCHOWN_ENV)!=NULL);
    inited=1;
  }
  return donttry;
}


/* The wrapped functions */


int WRAP_LSTAT LSTAT_ARG(int ver, 
		       const char *file_name, 
		       struct stat *statbuf){

  int r;

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
  struct stat st32;

  r=NEXT_LSTAT64(ver, file_name, st);

  if(r)
    return -1;

  stat32from64(&st32,st);
  send_get_stat(&st32);
  stat64from32(st,&st32);

  return 0;
}


int WRAP_STAT64 STAT64_ARG(int ver, 
			   const char *file_name, 
			   struct stat64 *st){
  int r;
  struct stat st32;

  r=NEXT_STAT64(ver,file_name,st);
  if(r)
    return -1;
  stat32from64(&st32,st);
  send_get_stat(&st32);
  stat64from32(st,&st32);
  return 0;
}


int WRAP_FSTAT64 FSTAT64_ARG(int ver, 
			     int fd, 
			     struct stat64 *st){
  int r;
  struct stat st32;

  r=NEXT_FSTAT64(ver, fd, st);
  if(r)
    return -1;
  stat32from64(&st32,st);
  send_get_stat(&st32);
  stat64from32(st,&st32);

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
  struct stat st;

  r=NEXT_LSTAT(_STAT_VER, pathname, &st);
  if(r)
    return -1;

  r=next_unlink(pathname);  

  if(r)
    return -1;
  
  send_stat(&st, unlink_func);
  
  return 0;
}

/*
  See the `remove funtions:' comments above for more info on
  these remove function wrappers.
*/
int rmdir(const char *pathname){
  int r;
  struct stat st;

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

  /* If newpath points to an existing file, that file will be 
     unlinked.   Make sure we tell the faked daemon about this! */

  /* we need the st_new struct in order to inform faked about the
     (possible) unlink of the file */

  r=NEXT_LSTAT(_STAT_VER, newpath, &st);

  s=next_rename(oldpath, newpath);
  if(s)
    return -1;
  if(!r)
    send_stat(&st,unlink_func);

  return 0;
}

uid_t getuid(void){
  return faked_uid();
}

uid_t geteuid(void){
  return faked_euid();
}

uid_t getgid(void){
  return faked_gid();
}

uid_t getegid(void){
  return faked_egid();
}

int setuid(uid_t id){
  return 0;
}

int setgid(uid_t id){
  return 0;
}

int seteuid(uid_t id){
  return 0;
}

int setegid(uid_t id){
  return 0;
}

int setreuid(SETREUID_ARG ruid, SETREUID_ARG euid){
  return 0;
}

int setregid(SETREGID_ARG rgid, SETREGID_ARG egid){
  return 0;
}

int setresuid(uid_t ruid, uid_t euid, uid_t suid){
  return 0;
}

int setresgid(gid_t rgid, gid_t egid, gid_t sgid){
  return 0;
}

int initgroups(const char* user, INITGROUPS_SECOND_ARG group){
  return 0;
}
