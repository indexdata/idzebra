/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockidx.c,v $
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

static int lock_fd = -1;

void zebraIndexLockMsg (const char *str)
{
    int l, r;

    assert (lock_fd != -1);
    lseek (lock_fd, 0L, SEEK_SET);
    l = strlen(str);
    r = write (lock_fd, str, l);
    if (r != l)
    {
        logf (LOG_FATAL|LOG_ERRNO, "write lock file");
        exit (1);
    }
}

void zebraIndexUnlock (int rw)
{
    char path[1024];
    char pathPrefix[1024];

    if (lock_fd == -1)
        return;

    zebraLockPrefix (pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    unlink (path);
}

void zebraIndexLock (int rw)
{
    char path[1024];
    char pathPrefix[1024];
    char buf[256];
    int r;

    zebraLockPrefix (pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    if (rw)
    {
        while (1)
        {
            lock_fd = open (path, O_CREAT|O_RDWR|O_EXCL|O_SYNC, 0666);
            if (lock_fd == -1)
            {
                lock_fd = open (path, O_RDONLY);
                assert (lock_fd != -1);
                if (flock (lock_fd, LOCK_EX|LOCK_NB) == -1)
                {
                    if (errno == EWOULDBLOCK)
                    {
                        logf (LOG_LOG, "Waiting for other index process");
                        flock (lock_fd, LOCK_EX);
                        continue;
                    }
                    else
                    {
                        logf (LOG_FATAL|LOG_ERRNO, "flock %s", path);
                        exit (1);
                    }
                }
                else
                {
                    logf (LOG_WARN, "Unlocked %s", path);
                    r = read (lock_fd, buf, 256);
                    if (r == 0)
                    {
                        logf (LOG_WARN, "Zero length %s", path);
                        unlink (path);
                        close (lock_fd);
                        continue;
                    }
                    else if (r == -1)
                    {
                        logf (LOG_FATAL|LOG_ERRNO, "read  %s", path);
                        exit (1);
                    }
                    if (*buf == 'r')
                    {
                        logf (LOG_WARN, "Previous transaction didn't"
                              " reach commit");
                        unlink (path);
                        close (lock_fd);
                        continue;
                    }
                    else if (*buf == 'w')
                    {
                        logf (LOG_WARN, "Your index may be inconsistent");
                        exit (1);
                    }
                    else if (*buf == 'c')
                    {
                        logf (LOG_FATAL, "Previous transaction didn't"
                              " finish commit. Restore now!");
                        exit (1);
                    }
                    else 
                    {
                        logf (LOG_FATAL, "Unknown id 0x%02x in %s", *buf,
                              path);
                        exit (1);
                    }
                }
            }
            else
                break;
        }
        flock (lock_fd, LOCK_EX);
    }
}

void zebraIndexLockCommit (void)
{

}

