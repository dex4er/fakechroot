# include_headers.m4 - macro which generates prolog for C programs
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# AC_INCLUDES_HEADERS(HEADER-FILES, [PROLOG])
# -------------------------------------------
# Generates a prolog which can replace standard AC_INCLUDES_DEFAULT

AC_DEFUN([AC_INCLUDES_HEADERS],
    [m4_ifval([$2], [
$2
        ], [])
        m4_foreach_w([myheader], [$1],
            [m4_define(myhave, AS_TR_CPP(m4_join([], [HAVE_], myheader)))
m4_echo(m4_join([], [@%:@ifdef ], myhave))
m4_echo(m4_join([], [@%:@include <], myheader, [>]))
m4_echo([@%:@endif])
                ])])
