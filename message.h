#ifndef FAKEROOT_MESSAGE_H
#define FAKEROOT_MESSAGE_H

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
# ifdef HAVE_INTTYPES_H
# include <inttypes.h>
# else
#  error Problem
# endif
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
# define htonll(n)  (n)
# define ntohll(n)  (n)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
# define htonll(n)  ((((uint64_t) htonl(n)) << 32LL) | htonl((n) >> 32LL))
# define ntohll(n)  ((((uint64_t) ntohl(n)) << 32LL) | ntohl((n) >> 32LL))
#endif

#define FAKEROOTKEY_ENV "FAKEROOTKEY"

typedef uint32_t func_id_t;

typedef uint64_t fake_ino_t;
typedef uint64_t fake_dev_t;
typedef uint32_t fake_uid_t;
typedef uint32_t fake_gid_t;
typedef uint32_t fake_mode_t;
typedef uint32_t fake_nlink_t;

struct fakestat {
	fake_uid_t   uid;
	fake_gid_t   gid;
	fake_ino_t   ino;
	fake_dev_t   dev;
	fake_dev_t   rdev;
	fake_mode_t  mode;
	fake_nlink_t nlink;
} FAKEROOT_ATTR(packed);

struct fake_msg {
#ifndef FAKEROOT_FAKENET
	long mtype; /* message type in SYSV message sending */
#endif
	func_id_t       id; /* the requested function */
#ifndef FAKEROOT_FAKENET
	pid_t pid;
	int serial;
#endif
	struct fakestat st;
	uint32_t        remote;
} FAKEROOT_ATTR(packed);

#endif
