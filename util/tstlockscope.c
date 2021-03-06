/* This file is part of the Zebra server.
   Copyright (C) Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** \file tstlockscope.c
    \brief tests scope of fcntl locks.. Either "process"  or "thread"
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <yaz/log.h>
#include <yaz/test.h>
#include <fcntl.h>
#if YAZ_POSIX_THREADS
#include <pthread.h>
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

void tst(void)
{
    pthread_t child_thread;
    int r;
    fd = open("my.LCK", (O_CREAT|O_RDWR), 0666);

    YAZ_CHECK(fd != -1);
    if (fd == -1)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "open");
        return;
    }

    r = file_lock(fd, F_WRLCK, F_SETLKW);
    YAZ_CHECK(r != -1);
    if (r == -1)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "fcnt");
        return;
    }

#if YAZ_POSIX_THREADS
    pthread_create(&child_thread, 0 /* attr */, run_func, 0);
    pthread_join(child_thread, 0);
#endif
    yaz_log(YLOG_LOG, "fcntl lock scope: %s", scope);
}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    tst();
    YAZ_CHECK_TERM;
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

