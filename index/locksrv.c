/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: locksrv.c,v $
 * Revision 1.1  1995-12-07 17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <alexutil.h>
#include "index.h"

static int server_lock_fd = -1;

int zebraServerLock (void)
{
    char pathPrefix[1024];
    char path[1024];
    
    zebraLockPrefix (pathPrefix);

    assert (server_lock_fd == -1);
    sprintf (path, "%szebrasrv.%ld", pathPrefix, (long) getpid());
    if ((server_lock_fd = open (path, O_CREAT|O_RDWR|O_SYNC|O_EXCL, 0666))
        == -1)
    {
        logf (LOG_WARN|LOG_ERRNO, "remove stale %s", path);
        unlink (path);
        if ((server_lock_fd = open (path, O_CREAT|O_RDWR|O_SYNC|O_EXCL, 0666))
            == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
    }
    flock (server_lock_fd, LOCK_EX);

    return 0;
}

int zebraServerLockGetState (void)
{
    char pathPrefix[1024];
    char path[1024];
    char buf[256];
    int fd;
    
    zebraLockPrefix (pathPrefix);

    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    fd = open (path, O_RDONLY);
    if (fd == -1)
        return 0;
    if (read (fd, buf, 2) == 0)
        return 0;
    close (fd);
    return *buf;
}

void zebraServerLockMsg (const char *str)
{
    int l, r;

    assert (server_lock_fd != -1);
    lseek (server_lock_fd, 0L, SEEK_SET);
    l = strlen(str);
    r = write (server_lock_fd, str, l);
    if (r != l)
    {
        logf (LOG_FATAL|LOG_ERRNO, "write server lock file");
        exit (1);
    }
}

void zebraServerUnlock (void)
{
    char pathPrefix[1024];
    char path[1024];
    
    assert (server_lock_fd != -1);
    zebraLockPrefix (pathPrefix);
    flock (server_lock_fd, LOCK_UN);
    sprintf (path, "%szebrasrv.%ld", pathPrefix, (long) getpid());
    unlink (path);
}

