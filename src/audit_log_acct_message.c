/*
 * Copyright (C) 2015      JH Chatenet <jhcha54008@free.fr>
 *
 * Licensed under the LGPL v2.1.
*/


#include <config.h>

#include "libfakechroot.h"


wrapper(audit_log_acct_message, int, (int audit_fd, int type, const char *pgname, const char *op, const char *name, unsigned int id, const char *host, const char *addr, const char *tty, int result))
{
    return 0;
}
