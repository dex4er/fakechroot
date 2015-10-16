/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010-2015 Piotr Roszatycki <dexter@debian.org>

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

#ifdef __GNUC__

#define _BSD_SOURCE
#define _POSIX_SOURCE
#define _DEFAULT_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "libfakechroot.h"


/* #include <sys/types.h> */
/* #include <stdlib.h> */
/* #include <unistd.h> */
/* #include <sys/wait.h> */
wrapper(system, int, (const char * command))
{
    pid_t pid;
    int pstat;
    sigset_t mask, omask;
    struct sigaction new_action_ign, old_action_int, old_action_quit;

    debug("system(\"%s\")", command);
    if (command == 0)
        return 1;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &omask);

    if ((pid = vfork()) < 0) {
        sigprocmask(SIG_SETMASK, &omask, NULL);
        return -1;
    }
    if (pid == 0) {
        sigprocmask(SIG_SETMASK, &omask, NULL);

        execl("/bin/sh", "sh", "-c", command, (char *) 0);
        _exit(127);
    }

    new_action_ign.sa_handler = SIG_IGN;
    sigemptyset(&new_action_ign.sa_mask);
    new_action_ign.sa_flags = 0;

    sigaction(SIGINT, &new_action_ign, &old_action_int);
    sigaction(SIGQUIT, &new_action_ign, &old_action_quit);

    pid = waitpid(pid, (int *)&pstat, 0);

    sigprocmask(SIG_SETMASK, &omask, NULL);

    sigaction(SIGINT, &old_action_int, NULL);
    sigaction(SIGQUIT, &old_action_quit, NULL);

    return (pid == -1 ? -1 : pstat);
}

#else
typedef int empty_translation_unit;
#endif
