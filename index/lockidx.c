/* $Id: lockidx.c,v 1.24 2005-01-15 19:38:26 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
	    yaz_log (YLOG_WARN|YLOG_ERRNO, "cannot create lock %s", path);
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
	    yaz_log (YLOG_WARN|YLOG_ERRNO, "cannot create lock %s", path);
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
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "flock");
            exit (1);
        }
#endif
        if (commitPhase)
            yaz_log (YLOG_LOG, "Waiting for lock cmt");
        else
            yaz_log (YLOG_LOG, "Waiting for lock org");
        if (zebra_lock (h) == -1)
        {
            yaz_log (YLOG_FATAL, "flock");
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
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "write lock file");
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
        yaz_log (YLOG_WARN|YLOG_ERRNO, "unlink %s failed", path);
#else
    if (unlink (path) && errno != ENOENT)
        yaz_log (YLOG_WARN|YLOG_ERRNO, "unlink %s failed", path);
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
                yaz_log (YLOG_FATAL, "couldn't obtain indexer lock");
		exit (1);
            }
            if (zebra_lock_nb (server_lock_main) == -1)
            {
#ifdef WIN32
                yaz_log (YLOG_LOG, "waiting for other index process");
                zebra_lock (server_lock_main);
                zebra_unlock (server_lock_main);
                zebra_lock_destroy (server_lock_main);
                continue;
#else
                if (errno == EWOULDBLOCK)
                {
                    yaz_log (YLOG_LOG, "waiting for other index process");
                    zebra_lock (server_lock_main);
                    zebra_unlock (server_lock_main);
		    zebra_lock_destroy (server_lock_main);
                    continue;
                }
                else
                {
                    yaz_log (YLOG_FATAL|YLOG_ERRNO, "flock %s", path);
                    exit (1);
                }
#endif
            }
            else
            {
		int fd = zebra_lock_fd (server_lock_main);

                yaz_log (YLOG_WARN, "unlocked %s", path);
                r = read (fd, buf, 256);
                if (r == 0)
                {
                    yaz_log (YLOG_WARN, "zero length %s", path);
		    zebra_lock_destroy (server_lock_main);
		    unlink (path);
                    continue;
                }
                else if (r == -1)
                {
                    yaz_log (YLOG_FATAL|YLOG_ERRNO, "read %s", path);
                    exit (1);
                }
                if (*buf == 'r')
                {
                    yaz_log (YLOG_WARN, "previous transaction didn't"
                          " reach commit");
		    zebra_lock_destroy (server_lock_main);
                    bf_commitClean (bfs, rval);
                    unlink (path);
                    continue;
                }
                else if (*buf == 'd')
                {
                    yaz_log (YLOG_WARN, "commit file wan't deleted after commit");
                    zebra_lock_destroy (server_lock_main);
                    bf_commitClean (bfs, rval);
                    unlink (path);
                    continue;
                }                    
                else if (*buf == 'w')
                {
		    yaz_log (YLOG_WARN,
			  "The lock file indicates that your index is");
		    yaz_log (YLOG_WARN, "inconsistent. Perhaps the indexer");
		    yaz_log (YLOG_WARN, "terminated abnormally in the previous");
		    yaz_log (YLOG_WARN, "run. You can try to proceed by");
		    yaz_log (YLOG_WARN, "deleting the file %s", path);
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
                    yaz_log (YLOG_FATAL, "previous transaction didn't"
                          " finish commit. Commit now!");
                    exit (1);
                }
                else 
                {
                    yaz_log (YLOG_FATAL, "unknown id 0x%02x in %s", *buf,
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

