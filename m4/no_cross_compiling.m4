# no_cross_compiling.m4 - call macro with cross_compiling=no
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ASX_NO_CROSS_COMPILING(MACRO)
# -----------------------------
AC_DEFUN([ASX_NO_CROSS_COMPILING],
    [
AS_VAR_COPY([save_cross_compiling], [cross_compiling])
AS_VAR_SET([cross_compiling], [no])
$1
AS_VAR_COPY([cross_compiling], [save_cross_compiling])
AS_UNSET([save_cross_compiling])
])
