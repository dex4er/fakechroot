# include_headers.m4 - macro which generates prolog for C programs
#
# Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# ACX_INCLUDES_HEADERS(HEADER-FILES, [PROLOG])
# -------------------------------------------
# Generates a prolog which can replace standard AC_INCLUDES_DEFAULT

AC_DEFUN([ACX_INCLUDES_HEADERS],
    [
$2
        m4_foreach_w([myheader], [$1],
            [
[@%:@ifdef HAVE_]AS_TR_CPP(myheader)
[@%:@include <]myheader[>]
[@%:@endif]
                ])])
