/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockidx.c,v $
 * Revision 1.5  1995-12-13 08:46:09  adam
 * Locking uses F_WRLCK and F_RDLCK again!
 *
 * Revision 1.4  1995/12/12  16:00:57  adam
 * System call sync(2) used after update/commit.
 * Locking (based on fcntl) uses F_EXLCK and F_SHLCK instead of F_WRLCK
 * and F_RDLCK.
 *
 * Revision 1.3  1995/12/11  11:43:29  adam
 * Locking based on fcntl instead of flock.
 * Setting commitEnable removed. Command line option -n can be used to
 * prevent commit if commit setting is defined in the configuration file.
 *
 * Revision 1.2  1995/12/08  16:22:54  adam
 * Work on update while servers are running. Three lock files introduced.
 * The servers reload their registers when necessary, but they don't
 * reestablish result sets yet.
 *
 * Revision 1.1  1995/12/07  17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <alexutil.h>
#include "index.h"

static int lock_fd = -1;
static int server_lock_cmt = -1;
static int server_lock_org = -1;

int zebraIndexWait (int commitPhase)
{
    char pathPrefix[1024];
    char path[1024];
    int fd;
    
    zebraLockPrefix (pathPrefix);

    if (server_lock_cmt == -1)
    {
        sprintf (path, "%s%s", FNAME_COMMIT_LOCK, pathPrefix);
        if ((server_lock_cmt = open (path, O_CREAT|O_RDWR|O_SYNC, 0666))
            == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
    }
    else
        zebraUnlock (server_lock_cmt);
    if (server_lock_org == -1)
    {
        sprintf (path, "%s%s", FNAME_ORG_LOCK, pathPrefix);
        if ((server_lock_org = open (path, O_CREAT|O_RDWR|O_SYNC, 0666))
            == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
    }
    else
        zebraUnlock (server_lock_org);
    if (commitPhase)
        fd = server_lock_cmt;
    else
        fd = server_lock_org;
    if (zebraLockNB (fd, 1) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            logf (LOG_FATAL|LOG_ERRNO, "flock");
            exit (1);
        }
        if (commitPhase)
            logf (LOG_LOG, "Waiting for lock cmt");
        else
            logf (LOG_LOG, "Waiting for lock org");
        if (zebraLock (fd, 1) == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "flock");
            exit (1);
        }
    }
    zebraUnlock (fd);
    return 0;
}


void zebraIndexLockMsg (const char *str)
{
    char path[1024];
    char pathPrefix[1024];
    int l, r, fd;

    assert (lock_fd != -1);
    lseek (lock_fd, 0L, SEEK_SET);
    l = strlen(str);
    r = write (lock_fd, str, l);
    if (r != l)
    {
        logf (LOG_FATAL|LOG_ERRNO, "write lock file");
        exit (1);
    }
    zebraLockPrefix (pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_TOUCH_TIME);
    fd = creat (path, 0666);
    close (fd);
}

void zebraIndexUnlock (void)
{
    char path[1024];
    char pathPrefix[1024];

    zebraLockPrefix (pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    unlink (path);
}

void zebraIndexLock (int commitNow)
{
    char path[1024];
    char pathPrefix[1024];
    char buf[256];
    int r;

    if (lock_fd != -1)
        return ;
    zebraLockPrefix (pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    while (1)
    {
        lock_fd = open (path, O_CREAT|O_RDWR|O_EXCL, 0666);
        if (lock_fd == -1)
        {
            lock_fd = open (path, O_RDWR);
            if (lock_fd == -1) 
            {
                if (errno == ENOENT)
                    continue;
                logf (LOG_FATAL|LOG_ERRNO, "open %s", path);
                exit (1);
            }
            logf (LOG_LOG, "zebraLockNB");
            if (zebraLockNB (lock_fd, 1) == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    logf (LOG_LOG, "Waiting for other index process");
                    zebraLock (lock_fd, 1);
                    zebraUnlock (lock_fd);
                    close (lock_fd);
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
                    close (lock_fd);
                    unlink (path);
                    continue;
                }
                else if (r == -1)
                {
                    logf (LOG_FATAL|LOG_ERRNO, "read %s", path);
                    exit (1);
                }
                if (*buf == 'r')
                {
                    logf (LOG_WARN, "Previous transaction didn't"
                          " reach commit");
                    close (lock_fd);
                    bf_commitClean ();
                    unlink (path);
                    continue;
                }
                else if (*buf == 'd')
                {
                    logf (LOG_WARN, "Commit file wan't deleted after commit");
                    close (lock_fd);
                    bf_commitClean ();
                    unlink (path);
                    continue;
                }                    
                else if (*buf == 'w')
                {
                    logf (LOG_WARN, "Your index may be inconsistent");
                    exit (1);
                }
                else if (*buf == 'c')
                {
                    if (commitNow)
                    {
                        unlink (path);
                        close (lock_fd);
                        continue;
                    }
                    logf (LOG_FATAL, "Previous transaction didn't"
                          " finish commit. Commit now!");
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
    zebraLock (lock_fd, 1);
}

