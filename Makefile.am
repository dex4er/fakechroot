AUTOMAKE_OPTIONS=foreign
SUBDIRS=scripts doc test

noinst_LTLIBRARIES = libcommunicate.la
libcommunicate_la_SOURCES = communicate.c

lib_LTLIBRARIES=libfakechroot.la
libfakechroot_la_SOURCES=libfakeroot.c wrapdef.h  wrapstruct.h communicate.h
libfakechroot_la_LDFLAGS=-version-info 0:1:0
#libfakechroot_la_LDFLAGS=-version-info 4:0:4
libfakechroot_la_DEPENDENCIES=wrapdef.h wrapstruct.h libcommunicate.la
libfakechroot_la_LIBADD = libcommunicate.la

bin_PROGRAMS=faked
faked_SOURCES = faked.cc
faked_LDADD = libcommunicate.la

simple_SOURCES=simple.c
noinst_PROGRAMS=simple

CLEANFILES= wrapdef.h wrapstruct.h wrapped.h wraptmpf.h

EXTRA_DIST=wrapawk wrapfunc.inp                        \
           debian/rules debian/changelog debian/control  \
	   DEBUG BUGS

CLEAN_FILES=fakerootconfig.h

wrapped.h wrapdef.h wrapstruct.h wraptmpf.h:wrapawk wrapfunc.inp
	awk -f $(srcdir)/wrapawk < $(srcdir)/wrapfunc.inp  

libfakeroot.lo:libfakeroot.c wrapdef.h wrapstruct.h wraptmpf.h 

fakerootconfig.h: ./config.status
	CONFIG_FILES= CONFIG_HEADERS= /bin/sh ./config.status


#Local variables:
#mode: makefile
#End:

