/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockidx.c,v $
 * Revision 1.12  1997-09-25 14:54:43  adam
 * WIN32 files lock support.
 *
 * Revision 1.11  1997/09/17 12:19:15  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.10  1997/09/09 13:38:07  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.9  1997/09/04 13:58:04  adam
 * Added O_BINARY for open calls.
 *
 * Revision 1.8  1997/02/12 20:39:46  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.7  1996/10/29 14:08:13  adam
 * Uses resource lockDir instead of lockPath.
 *
 * Revision 1.6  1996/03/26 16:01:13  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.5  1995/12/13  08:46:09  adam
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
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "index.h"

static ZebraLockHandle server_lock_main = NULL;
static ZebraLockHandle server_lock_cmt = NULL;
static ZebraLockHandle server_lock_org = NULL;

int zebraIndexWait (int commitPhase)
{
    char pathPrefix[1024];
    char path[1024];
    ZebraLockHandle h;
    
    zebraLockPrefix (common_resource, pathPrefix);

    if (server_lock_cmt)
	zebra_unlock (server_lock_cmt);
    else
    {
	sprintf (path, "%s%s", pathPrefix, FNAME_COMMIT_LOCK);
	server_lock_cmt = zebra_lock_create (path, 1);
	if (!server_lock_cmt)
	{
	    logf (LOG_WARN|LOG_ERRNO, "cannot create lock %s", path);
	    return -1;
	}
    }
    if (server_lock_org)
	zebra_unlock (server_lock_org);
    else
    {
        sprintf (path, "%s%s", pathPrefix, FNAME_ORG_LOCK);
	server_lock_org = zebra_lock_create (path, 1);
	if (!server_lock_org)
	{
	    logf (LOG_WARN|LOG_ERRNO, "cannot create lock %s", path);
	    return -1;
	}
    }
    if (commitPhase)
        h = server_lock_cmt;
    else
        h = server_lock_org;
    if (zebra_lock_nb (h))
    {
#ifndef WINDOWS
        if (errno != EWOULDBLOCK)
        {
            logf (LOG_FATAL|LOG_ERRNO, "flock");
            exit (1);
        }
#endif
        if (commitPhase)
            logf (LOG_LOG, "Waiting for lock cmt");
        else
            logf (LOG_LOG, "Waiting for lock org");
        if (zebra_lock (h) == -1)
        {
            logf (LOG_FATAL, "flock");
            exit (1);
        }
    }
    zebra_unlock (h);
    return 0;
}


void zebraIndexLockMsg (const char *str)
{
    char path[1024];
    char pathPrefix[1024];
    int l, r, fd;

    assert (server_lock_main);
    fd = zebra_lock_fd (server_lock_main);
    lseek (fd, 0L, SEEK_SET);
    l = strlen(str);
    r = write (fd, str, l);
    if (r != l)
    {
        logf (LOG_FATAL|LOG_ERRNO, "write lock file");
        exit (1);
    }
    zebraLockPrefix (common_resource, pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_TOUCH_TIME);
    fd = creat (path, 0666);
    close (fd);
}

void zebraIndexUnlock (void)
{
    char path[1024];
    char pathPrefix[1024];

    zebraLockPrefix (common_resource, pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    unlink (path);
}

void zebraIndexLock (BFiles bfs, int commitNow, const char *rval)
{
    char path[1024];
    char pathPrefix[1024];
    char buf[256];
    int r;

    if (server_lock_main)
        return ;
    zebraLockPrefix (common_resource, pathPrefix);
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    while (1)
    {
        server_lock_main = zebra_lock_create (path, 2);
        if (!server_lock_main)
        {
            server_lock_main = zebra_lock_create (path, 1);
	    if (!server_lock_main)
            {
                if (errno == ENOENT)
                    continue;
                logf (LOG_FATAL|LOG_ERRNO, "open %s", path);
                exit (1);
            }
            if (zebra_lock_nb (server_lock_main) == -1)
            {
#ifdef WINDOWS
                logf (LOG_LOG, "waiting for other index process");
                zebra_lock (server_lock_main);
                zebra_unlock (server_lock_main);
                zebra_lock_destroy (server_lock_main);
                continue;
#else
                if (errno == EWOULDBLOCK)
                {
                    logf (LOG_LOG, "waiting for other index process");
                    zebra_lock (server_lock_main);
                    zebra_unlock (server_lock_main);
		    zebra_lock_destroy (server_lock_main);
                    continue;
                }
                else
                {
                    logf (LOG_FATAL|LOG_ERRNO, "flock %s", path);
                    exit (1);
                }
#endif
            }
            else
            {
		int fd = zebra_lock_fd (server_lock_main);

                logf (LOG_WARN, "unlocked %s", path);
                r = read (fd, buf, 256);
                if (r == 0)
                {
                    logf (LOG_WARN, "zero length %s", path);
		    zebra_lock_destroy (server_lock_main);
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
                    logf (LOG_WARN, "previous transaction didn't"
                          " reach commit");
		    zebra_lock_destroy (server_lock_main);
                    bf_commitClean (bfs, rval);
                    unlink (path);
                    continue;
                }
                else if (*buf == 'd')
                {
                    logf (LOG_WARN, "commit file wan't deleted after commit");
                    zebra_lock_destroy (server_lock_main);
                    bf_commitClean (bfs, rval);
                    unlink (path);
                    continue;
                }                    
                else if (*buf == 'w')
                {
                    logf (LOG_WARN, "your index may be inconsistent");
                    exit (1);
                }
                else if (*buf == 'c')
                {
                    if (commitNow)
                    {
                        unlink (path);
			zebra_lock_destroy (server_lock_main);
                        continue;
                    }
                    logf (LOG_FATAL, "previous transaction didn't"
                          " finish commit. Commit now!");
                    exit (1);
                }
                else 
                {
                    logf (LOG_FATAL, "unknown id 0x%02x in %s", *buf,
                          path);
                    exit (1);
                }
            }
        }
        else
            break;
    }
    zebra_lock (server_lock_main);
}

