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

#define MAXPATH 2048
static char *fakechroot_ptr;
static char *fakechroot_path;
static char fakechroot_buf[MAXPATH];

#define narrow_chroot_path(path, retval, errval) \
    { \
	fakechroot_path = getenv("FAKECHROOT"); \
	if (fakechroot_path != NULL) { \
	    fakechroot_ptr = strstr((path), fakechroot_path); \
	    if (fakechroot_ptr != (path)) { \
		if (errval != 0) { \
		    errno = errval; \
		} \
		return (retval); \
	    } \
	    (path) = ((path) + strlen(fakechroot_path)); \
	} \
    }

#define expand_chroot_path(path) \
    { \
	if (path != NULL && *path == '/') { \
    	    fakechroot_path = getenv("FAKECHROOT"); \
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
  struct stat st32;

  expand_chroot_path(file_name);
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

  expand_chroot_path(file_name);
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
  fd=next_open(pathname, O_WRONLY|O_CREAT|O_TRUNC, 00644);

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
  struct stat st;

  expand_chroot_path(pathname);
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



int chroot(const char *path) {
    char *ptr;
    int status;

    fakechroot_path = getenv("FAKECHROOT");
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

    setenv("FAKECHROOT", path, 1);
    fakechroot_path = getenv("FAKECHROOT");
    
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
    narrow_chroot_path(cwd, NULL, 0);
    return cwd;
}

char *getwd(char *buf) {
    char *cwd;

    if ((cwd = next_getwd(buf)) == NULL) {
	return NULL;
    }
    narrow_chroot_path(cwd, NULL, 0);
    return cwd;
}

char *get_current_dir_name(void) {
    char *cwd, *oldptr, *newptr;

    if ((cwd = next_get_current_dir_name()) == NULL) {
	return NULL;
    }
    oldptr = cwd;
    narrow_chroot_path(cwd, NULL, 0);
    if ((newptr = malloc(strlen(cwd)+1)) == NULL) {
	free(oldptr);
	return NULL;
    }
    strcpy(newptr, cwd);
    free(oldptr);
    return newptr;
}

int open(const char *pathname, int flags, ...) {
    int mode = 0;
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next_open(pathname, flags, mode);
}

int creat(const char *pathname, mode_t mode) {
    expand_chroot_path(pathname);
    return next_creat(pathname, mode);
}

int open64(const char *pathname, int flags, ...) {
    int mode = 0;
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next_open64(pathname, flags, mode);
}

int creat64(const char *pathname, mode_t mode) {
    expand_chroot_path(pathname);
    return next_creat64(pathname, mode);
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
    char tmp[MAXPATH];
    expand_chroot_path(oldpath);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath);
    return next_symlink(oldpath, newpath);
}

int link(const char *oldpath, const char *newpath) {
    char tmp[MAXPATH];
    expand_chroot_path(oldpath);
    strcpy(tmp, oldpath); oldpath=tmp;
    expand_chroot_path(newpath);
    return next_link(oldpath, newpath);
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

    fakechroot_path = getenv("FAKECHROOT");
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
    char tmp[MAXPATH], *oldtemplate, *ptr;
    int fd;

    oldtemplate = template;
    
    expand_chroot_path(template);
    if ((fd = next_mkstemp(template)) == -1) {
	return -1;
    }

    ptr = tmp;
    strcpy(ptr, template);
    narrow_chroot_path(ptr, -1, EEXIST);
    strcpy(oldtemplate, ptr);
    return fd;
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
    int mode = 0;
    expand_chroot_path(pathname);

return -1;
    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next___open(pathname, flags, mode);
}

int __open64(const char *pathname, int flags, ...) {
    int mode = 0;
    expand_chroot_path(pathname);

    if (flags & O_CREAT) {
        va_list arg;
        va_start (arg, flags);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return next___open64(pathname, flags, mode);
}

int lckpwdf() {
    return 0;
}

int ulckpwdf() {
    return 0;
}

int setgroups(size_t size, const gid_t *list) {
    return 0;
}
