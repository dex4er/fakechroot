dnl  AC_FUNC_READLINKAT_ARGTYPES
dnl  ---------------------------
dnl  Determine the correct type to be passed to each of the `readlinkat'
dnl  function's arguments, and define those types in `READLINKAT_TYPE_RETURN',
dnl  `READLINKAT_TYPE_ARG1', `READLINKAT_TYPE_ARG2', and
dnl  `READLINKAT_TYPE_ARG3'.
AN_FUNCTION([readlinkat], [AC_FUNC_READLINKAT_ARGTYPES])
AC_DEFUN([AC_FUNC_READLINKAT_ARGTYPES],
[AC_CHECK_HEADERS(unistd.h)
AC_CACHE_CHECK([types of arguments for readlinkat],
[ac_cv_func_readlinkat_args],
[for ac_return in 'ssize_t' 'int'; do
 for ac_arg1 in 'int name'; do
  for ac_arg2 in 'const char *name'; do
   for ac_arg3 in 'char *name'; do
    for ac_arg4 in 'size_t name' 'int name'; do
   AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#ifdef HAVE_UNISTD_H
#define _ATFILE_SOURCE
#include <unistd.h>
#endif
],
            [
#define ac_arg1(name) $ac_arg1
#define ac_arg2(name) $ac_arg2
#define ac_arg3(name) $ac_arg3
#define ac_arg4(name) $ac_arg4
extern $ac_return readlinkat (ac_arg1(dirfd), ac_arg2(pathname), ac_arg3(buf), ac_arg4(bufsiz));])],
            [ac_cv_func_readlinkat_args="$ac_return;$ac_arg1;$ac_arg2;$ac_arg3;$ac_arg4"; break 5])
    done
   done
  done
 done
done
# Provide a safe default value.
: ${ac_cv_func_readlinkat_args='(default) ssize_t;int name;const char *name;char *name;size_t name'}
])
ac_save_IFS=$IFS; IFS=';'
set dummy `echo "$ac_cv_func_readlinkat_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
IFS=$ac_save_IFS
shift
AC_DEFINE_UNQUOTED(READLINKAT_TYPE_RETURN, $[1],
                   [Define to the type of return value for `readlinkat'.])
AC_DEFINE_UNQUOTED(READLINKAT_TYPE_ARG1(name), $[2],
                   [Define to the type of arg 1 for `readlinkat'.])
AC_DEFINE_UNQUOTED(READLINKAT_TYPE_ARG2(name), $[3],
                   [Define to the type of arg 2 for `readlinkat'.])
AC_DEFINE_UNQUOTED(READLINKAT_TYPE_ARG3(name), $[4],
                   [Define to the type of arg 3 for `readlinkat'.])
AC_DEFINE_UNQUOTED(READLINKAT_TYPE_ARG4(name), $[5],
                   [Define to the type of arg 4 for `readlinkat'.])
rm -f conftest*
])
