#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

static struct pid {
    struct pid *next;
    FILE *fp;
    pid_t pid;
} *pidlist;

/* popen reimplementation taken from bionic */
FILE *popen(const char *program, const char *type) {
    struct pid * volatile cur;
    FILE *iop;
    int pdes[2];
    pid_t pid;

    if ((*type != 'r' && *type != 'w') || type[1] != '\0') {
        errno = EINVAL;
        return (NULL);
    }

    if ((cur = malloc(sizeof(struct pid))) == NULL)
        return (NULL);

    if (pipe(pdes) < 0) {
        free(cur);
        return (NULL);
    }

    switch (pid = vfork()) {
    case -1: /* Error. */
        (void) close(pdes[0]);
        (void) close(pdes[1]);
        free(cur);
        return (NULL);
        /* NOTREACHED */
    case 0: /* Child. */
    {
        struct pid *pcur;
        /*
         * because vfork() instead of fork(), must leak FILE *,
         * but luckily we are terminally headed for an execl()
         */
        for (pcur = pidlist; pcur; pcur = pcur->next)
            close(fileno(pcur->fp));

        if (*type == 'r') {
            int tpdes1 = pdes[1];

            (void) close(pdes[0]);
            /*
             * We must NOT modify pdes, due to the
             * semantics of vfork.
             */
            if (tpdes1 != STDOUT_FILENO) {
                (void) dup2(tpdes1, STDOUT_FILENO);
                (void) close(tpdes1);
                tpdes1 = STDOUT_FILENO;
            }
        } else {
            (void) close(pdes[1]);
            if (pdes[0] != STDIN_FILENO) {
                (void) dup2(pdes[0], STDIN_FILENO);
                (void) close(pdes[0]);
            }
        }
        execl("/bin/sh", "sh", "-c", program, (char *) NULL);
        _exit(127);
        /* NOTREACHED */
    }
    }

    /* Parent; assume fdopen can't fail. */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        (void) close(pdes[1]);
    } else {
        iop = fdopen(pdes[1], type);
        (void) close(pdes[0]);
    }

    /* Link into list of file descriptors. */
    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

    return (iop);
}


int main (int argc, char *argv[]) {
    FILE *fp;
    int status;
    char path[PATH_MAX];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s cmd\n", argv[0]);
        exit(2);
    }

    fp = popen(argv[1], "r");
    if (fp == NULL) {
        perror("popen");
        exit(1);
    }

    while (fgets(path, PATH_MAX, fp) != NULL) {
        printf("%s", path);
    }

    status = pclose(fp);
    if (status == -1) {
        perror("pclose");
        exit(1);
    }
}
