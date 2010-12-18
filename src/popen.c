/*
    libfakechroot -- fake chroot environment
    Copyright (c) 2010 Piotr Roszatycki <dexter@debian.org>

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

#include <stdio.h>
#include <unistd.h>
#include "libfakechroot.h"


/*
   popen function taken from uClibc.
   Copyright (C) 2004       Manuel Novoa III    <mjn3@codepoet.org>
   Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 */

struct popen_list_item {
    struct popen_list_item *next;
    FILE *f;
    pid_t pid;
};

static struct popen_list_item *popen_list /* = NULL (bss initialized) */;

wrapper(popen, FILE *, (const char * command, const char * modes))
{
    FILE *fp;
    struct popen_list_item *pi;
    struct popen_list_item *po;
    int pipe_fd[2];
    int parent_fd;
    int child_fd;
    volatile int child_writing;         /* Doubles as the desired child fildes. */
    pid_t pid;

    debug("popen(\"%s\", \"%s\")", command, modes);
    child_writing = 0;                  /* Assume child is writing. */
    if (modes[0] != 'w') {              /* Parent not writing... */
        ++child_writing;                /* so child must be writing. */
        if (modes[0] != 'r') {  /* Oops!  Parent not reading either! */
            __set_errno(EINVAL);
            goto RET_NULL;
        }
    }

    if (!(pi = malloc(sizeof(struct popen_list_item)))) {
        goto RET_NULL;
    }

    if (pipe(pipe_fd)) {
        goto FREE_PI;
    }

    child_fd = pipe_fd[child_writing];
    parent_fd = pipe_fd[1-child_writing];

    if (!(fp = fdopen(parent_fd, modes))) {
        close(parent_fd);
        close(child_fd);
        goto FREE_PI;
    }

    if ((pid = vfork()) == 0) { /* Child of vfork... */
        close(parent_fd);
        if (child_fd != child_writing) {
            dup2(child_fd, child_writing);
            close(child_fd);
        }

        /* SUSv3 requires that any previously popen()'d streams in the
         * parent shall be closed in the child. */
        for (po = popen_list ; po ; po = po->next) {
            fclose(po->f);
        }

        execl("/bin/sh", "sh", "-c", command, (char *)0);

        /* SUSv3 mandates an exit code of 127 for the child if the
         * command interpreter can not be invoked. */
        _exit(127);
    }

    /* We need to close the child filedes whether vfork failed or
     * it succeeded and we're in the parent. */
    close(child_fd);

    if (pid > 0) {                              /* Parent of vfork... */
        pi->pid = pid;
        pi->f = fp;
        pi->next = popen_list;
        popen_list = pi;

        return fp;
    }

    /* If we get here, vfork failed. */
    fclose(fp);                                 /* Will close parent_fd. */

FREE_PI:
    free(pi);

RET_NULL:
    return NULL;
}

#endif
