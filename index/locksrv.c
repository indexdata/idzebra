/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: locksrv.c,v $
 * Revision 1.3  1995-12-11 11:43:29  adam
 * Locking based on fcntl instead of flock.
 * Setting commitEnable removed. Command line option -n can be used to
 * prevent commit if commit setting is defined in the configuration file.
 *
 * Revision 1.2  1995/12/08  16:22:55  adam
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
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <alexutil.h>
#include "zserver.h"

static int server_lock_cmt = -1;
static int server_lock_org = -1;

int zebraServerLock (int commitPhase)
{
    char pathPrefix[1024];
    char path[1024];
    
    zebraLockPrefix (pathPrefix);

    if (server_lock_cmt == -1)
    {
        sprintf (path, "%s%s", FNAME_COMMIT_LOCK, pathPrefix);
        if ((server_lock_cmt = open (path, O_CREAT|O_RDWR, 0666))
            == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
        assert (server_lock_org == -1);

        sprintf (path, "%s%s", FNAME_ORG_LOCK, pathPrefix);
        if ((server_lock_org = open (path, O_CREAT|O_RDWR, 0666))
            == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
    }
    if (commitPhase)
    {
        logf (LOG_LOG, "Server locks org");
        zebraLock (server_lock_org, 0);
    }
    else
    {
        logf (LOG_LOG, "Server locks cmt");
        zebraLock (server_lock_cmt, 0);
    }
    return 0;
}

void zebraServerUnlock (int commitPhase)
{
    if (server_lock_org == -1)
        return;
    if (commitPhase)
    {
        logf (LOG_LOG, "Server unlocks org");
        zebraUnlock (server_lock_org);
    }
    else
    {
        logf (LOG_LOG, "Server unlocks cmt");
        zebraUnlock (server_lock_cmt);
    }
}

int zebraServerLockGetState (time_t *timep)
{
    char pathPrefix[1024];
    char path[1024];
    char buf[256];
    int fd;
    struct stat xstat;
    
    zebraLockPrefix (pathPrefix);

    sprintf (path, "%s%s", pathPrefix, FNAME_TOUCH_TIME);
    if (stat (path, &xstat) == -1)
        *timep = 1;
    else
        *timep = xstat.st_ctime;
    
    sprintf (path, "%s%s", pathPrefix, FNAME_MAIN_LOCK);
    fd = open (path, O_RDONLY);
    if (fd == -1)
    {
        *buf = 0;
        return 0;
    }
    if (read (fd, buf, 2) == 0)
    {
        *buf = 0;
        return 0;
    }
    close (fd);
    return *buf;
}
