dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([libfakeroot.c])
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(fakeroot, 0.5.1)
AM_MAINTAINER_MODE
AC_CONFIG_AUX_DIR(..)

dnl AM_CONFIG_HEADER(config.h)

AM_PROG_LIBTOOL
AC_PROG_CC
dnl Checks for programs.

dnl Checks for libraries.
dnl Replace `main' with a function in -ldl:
AC_CHECK_LIB(dl, dlopen)


AC_CONFIG_FILES([Makefile])
AC_OUTPUT

dnl Local variables:
dnl mode: m4
dnl End:
