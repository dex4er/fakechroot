dnl  AC_FUNC_SCANDIR_ARGTYPES
dnl  -------------------------
dnl  Determine the correct type to be passed to each of the `scandir'
dnl  function's arguments, and define those types in `SCANDIR_TYPE_ARG1',
dnl  `SCANDIR_TYPE_ARG2', `SCANDIR_TYPE_ARG3' and `SCANDIR_TYPE_ARG4'.
AN_FUNCTION([scandir], [AC_FUNC_SCANDIR_ARGTYPES])
AC_DEFUN([AC_FUNC_SCANDIR_ARGTYPES],
[AC_CHECK_HEADERS(dirent.h)
AC_CACHE_CHECK([types of arguments for scandir],
[ac_cv_func_scandir_args],
[for ac_return in 'int'; do
 for ac_arg1 in 'const char *name'; do
  for ac_arg2 in 'struct dirent ***name'; do
   for ac_arg3 in 'int(*name)(const struct dirent *)' 'int(*name)(struct dirent *)'; do
    for ac_arg4 in 'int(*name)(const void *,const void *)' 'int(*name)(const struct dirent **, const struct dirent **)'; do
     AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#include <dirent.h>
],
            [
#define ac_arg1(name) $ac_arg1
#define ac_arg2(name) $ac_arg2
#define ac_arg3(name) $ac_arg3
#define ac_arg4(name) $ac_arg4
extern $ac_return scandir (ac_arg1(dirp), ac_arg2(namelist), ac_arg3(filter), ac_arg4(compar));])],
            [ac_cv_func_scandir_args="$ac_return;$ac_arg1;$ac_arg2;$ac_arg3;$ac_arg4"; break 5])
    done
   done
  done
 done
done
# Provide a safe default value.
: ${ac_cv_func_scandir_args='(default) int;const char *name;struct dirent ***name;int(*name)(const struct dirent *);int(*name)(const void *,const void *)'}
])
ac_save_IFS=$IFS; IFS=';'
set dummy `echo "$ac_cv_func_scandir_args" | sed 's/^(default) //' | sed 's/\*/\*/g'`
IFS=$ac_save_IFS
shift
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_RETURN, $[1],
                   [Define to the type of return value for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG1(name), $[2],
                   [Define to the type of arg 1 for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG2(name), $[3],
                   [Define to the type of arg 2 for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG3(name), $[4],
                   [Define to the type of arg 3 for `scandir'.])
AC_DEFINE_UNQUOTED(SCANDIR_TYPE_ARG4(name), $[5],
                   [Define to the type of arg 4 for `scandir'.])
rm -f conftest*
])
