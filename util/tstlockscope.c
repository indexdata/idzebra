/* $Id: tstlockscope.c,v 1.1 2006-07-02 21:22:17 adam Exp $
   Copyright (C) 2006
   Index Data ApS
*/

/** \file tstlockscope.c
    \brief tests scope of fcntl locks.. Either "process"  or "thread"
*/

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#if YAZ_POSIX_THREADS
#include <fcntl.h>
#endif

int fd;

const char *scope = "unknown";

static int file_lock(int fd, int type, int cmd)
{
    struct flock area;
    area.l_type = type;
    area.l_whence = SEEK_SET;
    area.l_len = area.l_start = 0L;
    
    return fcntl(fd, cmd, &area);
}

void *run_func(void *arg)
{
    if (file_lock(fd, F_WRLCK, F_SETLK) == -1)
        scope = "thread";
    else
        scope = "process";
    return 0;
}

int main(int argc, char **argv)
{
    pthread_t child_thread;
    fd = open("my.LCK", (O_CREAT|O_RDWR), 0666);
    if (fd == -1)
    {
        fprintf(stderr, "open: %s\n", strerror(errno));
        exit(1);
    }

    if (file_lock(fd, F_WRLCK, F_SETLKW) == -1)
    {
        fprintf(stderr, "fcntl: %s\n", strerror(errno));
        exit(1);
    }

#if YAZ_POSIX_THREADS
    pthread_create(&child_thread, 0 /* attr */, run_func, 0);
    pthread_join(child_thread, 0);
#endif
    printf("fcntl lock scope: %s\n", scope);
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

