/* $Id: lockidx.c,v 1.22 2002-08-02 19:26:55 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/


#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "index.h"
#include "zserver.h"

static ZebraLockHandle server_lock_main = NULL;
static ZebraLockHandle server_lock_cmt = NULL;
static ZebraLockHandle server_lock_org = NULL;

int zebraIndexWait (ZebraHandle zh, int commitPhase)
{
    ZebraLockHandle h;

    if (server_lock_cmt)
	zebra_unlock (server_lock_cmt);
    else
    {
	char path[1024];

	zebra_lock_prefix (zh->service->res, path);
	strcat (path, FNAME_COMMIT_LOCK);
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
	char path[1024];

	zebra_lock_prefix (zh->service->res, path);
	strcat (path, FNAME_ORG_LOCK);
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
#ifndef WIN32
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


void zebraIndexLockMsg (ZebraHandle zh, const char *str)
{
    char path[1024];
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
    zebra_lock_prefix (zh->service->res, path);
    strcat (path, FNAME_TOUCH_TIME);
    fd = creat (path, 0666);
    close (fd);
}

void zebraIndexUnlock (ZebraHandle zh)
{
    char path[1024];
    
    zebra_lock_prefix (zh->service->res, path);
    strcat (path, FNAME_MAIN_LOCK);
#ifdef WIN32
    zebra_lock_destroy (server_lock_main);
    if (unlink (path) && errno != ENOENT)
        logf (LOG_WARN|LOG_ERRNO, "unlink %s failed", path);
#else
    if (unlink (path) && errno != ENOENT)
        logf (LOG_WARN|LOG_ERRNO, "unlink %s failed", path);
    zebra_lock_destroy (server_lock_main);
#endif
    server_lock_main = 0;
}

int zebraIndexLock (BFiles bfs, ZebraHandle zh, int commitNow,
                    const char *rval)
{
    char path[1024];
    char buf[256];
    int r;

    if (server_lock_main)
        return 0;

    zebra_lock_prefix (zh->service->res, path);
    strcat (path, FNAME_MAIN_LOCK);
    while (1)
    {
        server_lock_main = zebra_lock_create (path, 2);
        if (!server_lock_main)
        {
            server_lock_main = zebra_lock_create (path, 1);
	    if (!server_lock_main)
            {
                logf (LOG_FATAL, "couldn't obtain indexer lock");
		exit (1);
            }
            if (zebra_lock_nb (server_lock_main) == -1)
            {
#ifdef WIN32
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
		    logf (LOG_WARN,
			  "The lock file indicates that your index is");
		    logf (LOG_WARN, "inconsistent. Perhaps the indexer");
		    logf (LOG_WARN, "terminated abnormally in the previous");
		    logf (LOG_WARN, "run. You can try to proceed by");
		    logf (LOG_WARN, "deleting the file %s", path);
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
    return 0;
}

