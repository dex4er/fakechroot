/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010, 2013 Piotr Roszatycki <dexter@debian.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/


#include <config.h>

/*
 * Starting with glibc 2.32 the compat nss module for getpwnam calls
 * __nss_files_fopen (which is a GLIBC_PRIVATE symbol provided by glibc)
 * instead of fopen (see 299210c1fa67e2dfb564475986fce11cd33db9ad). This
 * leads to getpwnam calls accessing /etc/passwd from *outside* the chroot
 * and as a result programs like adduser do not work correctly anymore
 * under fakechroot.
 *
 * Starting with glibc 2.34 the __nss_files_fopen was moved from nss to
 * libc.so and thus wrapping it with LD_PRELOAD has no affect anymore
 * (see 6212bb67f4695962748a5981e1b9fea105af74f6).
 *
 * So now we also wrap all the functions accessing /etc/passwd, /etc/group
 * and /etc/shadow. This solution will ignore NIS, LDAP or other local files
 * as potentially configured in /etc/nsswitch.conf.
 */

#include <gnu/libc-version.h>
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 32)

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include "libfakechroot.h"

/* getpwent, setpwent, endpwent, getpwuid, getpwnam */

static FILE *pw_f;

wrapper(getpwent, struct passwd *, (void))
{
	if (!pw_f) pw_f = fopen("/etc/passwd", "rbe");
	if (!pw_f) return 0;
	return fgetpwent(pw_f);
}

wrapper (getpwent_r, int, (struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp))
{
	if (!pw_f) pw_f = fopen("/etc/passwd", "rbe");
	if (!pw_f) return 0;
	return fgetpwent_r(pw_f, pwbuf, buf, buflen, pwbufp);
}

wrapper(setpwent, void, (void))
{
	if (pw_f) fclose(pw_f);
	pw_f = 0;
}

wrapper(endpwent, void, (void))
{
	if (pw_f) fclose(pw_f);
	pw_f = 0;
}

wrapper(getpwuid, struct passwd *, (uid_t uid))
{
	debug("getpwuid(\"%ul\")", uid);
	FILE *f = fopen("/etc/passwd", "rbe");
	if (!f) {
		return NULL;
	}
	struct passwd *res = NULL;
	while ((res = fgetpwent(f))) {
		if (res->pw_uid == uid)
			break;
	}
	fclose(f);
	return res;
}

wrapper(getpwuid_r, int, (uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result))
{
	debug("getpwuid_r(\"%ul\")", uid);
	FILE *f = fopen("/etc/passwd", "rbe");
	if (!f) {
		return errno;
	}
	int res;
	while (!(res = fgetpwent_r(f, pwd, buf, buflen, result))) {
		if (pwd->pw_uid == uid)
			break;
	}
	fclose(f);
	return res;
}

wrapper(getpwnam, struct passwd *, (const char *name))
{
	debug("getpwnam(\"%s\")", name);
	FILE *f = fopen("/etc/passwd", "rbe");
	if (!f) {
		return NULL;
	}
	struct passwd *res = NULL;
	while ((res = fgetpwent(f))) {
		if (name && !strcmp(name, res->pw_name))
			break;
	}
	fclose(f);
	return res;
}

wrapper(getpwnam_r, int, (const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result))
{
	debug("getpwnam_r(\"%s\")", name);
	FILE *f = fopen("/etc/passwd", "rbe");
	if (!f) {
		return errno;
	}
	int res;
	while (!(res = fgetpwent_r(f, pwd, buf, buflen, result))) {
		if (name && !strcmp(name, pwd->pw_name))
			break;
	}
	fclose(f);
	return res;
}

/* getgrent, setgrent, endgrent, getgrgid, getgrnam */

static FILE *gr_f;

wrapper(getgrent, struct group *, (void))
{
	if (!gr_f) gr_f = fopen("/etc/group", "rbe");
	if (!gr_f) return 0;
	return fgetgrent(gr_f);
}

wrapper (getgrent_r, int, (struct group *gbuf, char *buf, size_t buflen, struct group **gbufp))
{
	if (!gr_f) gr_f = fopen("/etc/group", "rbe");
	if (!gr_f) return 0;
	return fgetgrent_r(gr_f, gbuf, buf, buflen, gbufp);
}

wrapper(setgrent, void, (void))
{
	if (gr_f) fclose(gr_f);
	gr_f = 0;
}

wrapper(endgrent, void, (void))
{
	if (gr_f) fclose(gr_f);
	gr_f = 0;
}

wrapper(getgrgid, struct group *, (gid_t gid))
{
	debug("getgrgid(\"%ul\")", gid);
	FILE *f = fopen("/etc/group", "rbe");
	if (!f) {
		return NULL;
	}
	struct group *res = NULL;
	while ((res = fgetgrent(f))) {
		if (res->gr_gid == gid)
			break;
	}
	fclose(f);
	return res;
}

wrapper(getgrgid_r, int, (gid_t gid, struct group *grp, char *buf, size_t buflen, struct group **result))
{
	debug("getgrgid_r(\"%ul\")", gid);
	FILE *f = fopen("/etc/group", "rbe");
	if (!f) {
		return errno;
	}
	int res;
	while (!(res = fgetgrent_r(f, grp, buf, buflen, result))) {
		if (grp->gr_gid == gid)
			break;
	}
	fclose(f);
	return res;
}

wrapper(getgrnam, struct group *, (const char *name))
{
	debug("getgrnam(\"%s\")", name);
	FILE *f = fopen("/etc/group", "rbe");
	if (!f) {
		return NULL;
	}
	struct group *res = NULL;
	while ((res = fgetgrent(f))) {
		if (name && !strcmp(name, res->gr_name))
			break;
	}
	fclose(f);
	return res;
}

wrapper(getgrnam_r, int, (const char *name, struct group *grp, char *buf, size_t buflen, struct group **result))
{
	debug("getgrnam_r(\"%s\")", name);
	FILE *f = fopen("/etc/group", "rbe");
	if (!f) {
		return errno;
	}
	int res;
	while (!(res = fgetgrent_r(f, grp, buf, buflen, result))) {
		if (name && !strcmp(name, grp->gr_name))
			break;
	}
	fclose(f);
	return res;
}

/* getspent, setspent, endspent, getspnam */

static FILE *sp_f;

wrapper(getspent, struct spwd *, (void))
{
	if (!sp_f) sp_f = fopen("/etc/shadow", "rbe");
	if (!sp_f) return 0;
	return fgetspent(sp_f);
}

wrapper(setspent, void, (void))
{
	if (sp_f) fclose(sp_f);
	sp_f = 0;
}

wrapper(endspent, void, (void))
{
	if (sp_f) fclose(sp_f);
	sp_f = 0;
}

wrapper(getspnam, struct spwd *, (const char *name))
{
	debug("getspnam(\"%s\")", name);
	FILE *f = fopen("/etc/shadow", "rbe");
	if (!f) {
		return NULL;
	}
	struct spwd *res = NULL;
	while ((res = fgetspent(f))) {
		if (name && !strcmp(name, res->sp_namp))
			break;
	}
	fclose(f);
	return res;
}

wrapper(getspnam_r, int, (const char *name, struct spwd *spbuf, char *buf, size_t buflen, struct spwd **spbufp))
{
	debug("getspnam_r(\"%s\")", name);
	FILE *f = fopen("/etc/shadow", "rbe");
	if (!f) {
		return errno;
	}
	int res;
	while (!(res = fgetspent_r(f, spbuf, buf, buflen, spbufp))) {
		if (name && !strcmp(name, spbuf->sp_namp))
			break;
	}
	fclose(f);
	return res;
}

#else
typedef int empty_translation_unit;
#endif
