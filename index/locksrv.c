/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: locksrv.c,v $
 * Revision 1.11  1998-03-05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.10  1997/09/29 09:08:36  adam
 * Revised locking system to be thread safe for the server.
 *
 * Revision 1.9  1997/09/25 14:54:43  adam
 * WIN32 files lock support.
 *
 * Revision 1.8  1997/09/17 12:19:15  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.7  1997/09/04 13:58:04  adam
 * Added O_BINARY for open calls.
 *
 * Revision 1.6  1996/10/29 14:06:52  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.5  1996/05/15 11:58:18  adam
 * Changed some log messages.
 *
 * Revision 1.4  1996/04/10  16:01:27  quinn
 * Fixed order of path/filename.
 *
 * Revision 1.3  1995/12/11  11:43:29  adam
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
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "zserver.h"

int zebra_server_lock_init (ZebraHandle zi)
{
    char path_prefix[1024];

    assert (zi->res);
    zi->server_lock_cmt = NULL;
    zi->server_lock_org = NULL;

    zebra_lock_prefix (zi->res, path_prefix);
    zi->server_path_prefix = xmalloc (strlen(path_prefix)+1);
    strcpy (zi->server_path_prefix, path_prefix);

    logf (LOG_DEBUG, "Locking system initialized");
    return 0;
}

int zebra_server_lock_destroy (ZebraHandle zi)
{
    xfree (zi->server_path_prefix);
    zebra_lock_destroy (zi->server_lock_cmt);
    zebra_lock_destroy (zi->server_lock_org);
    logf (LOG_DEBUG, "Locking system destroyed");
    return 0;
}

int zebra_server_lock (ZebraHandle zi, int commitPhase)
{
    if (!zi->server_lock_cmt)
    {
	char path[1024];

	strcpy (path, zi->server_path_prefix);
	strcat (path, FNAME_COMMIT_LOCK);
        if (!(zi->server_lock_cmt = zebra_lock_create (path, 0)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
        assert (zi->server_lock_org == NULL);

	strcpy (path, zi->server_path_prefix);
	strcat (path, FNAME_ORG_LOCK);
        if (!(zi->server_lock_org = zebra_lock_create (path, 0)))
        {
            logf (LOG_FATAL|LOG_ERRNO, "create %s", path);
            return -1;
        }
    }
    if (commitPhase)
    {
        logf (LOG_DEBUG, "Server locks org");
        zebra_lock (zi->server_lock_org);
    }
    else
    {
        logf (LOG_DEBUG, "Server locks cmt");
        zebra_lock (zi->server_lock_cmt);
    }
    return 0;
}

void zebra_server_unlock (ZebraHandle zi, int commitPhase)
{
    if (zi->server_lock_org == NULL)
        return;
    if (commitPhase)
    {
        logf (LOG_DEBUG, "Server unlocks org");
        zebra_unlock (zi->server_lock_org);
    }
    else
    {
        logf (LOG_DEBUG, "Server unlocks cmt");
        zebra_unlock (zi->server_lock_cmt);
    }
}

int zebra_server_lock_get_state (ZebraHandle zi, time_t *timep)
{
    char path[1024];
    char buf[256];
    int fd;
    struct stat xstat;
    
    strcpy (path, zi->server_path_prefix);
    strcat (path, FNAME_TOUCH_TIME);
    if (stat (path, &xstat) == -1)
        *timep = 1;
    else
        *timep = xstat.st_ctime;

    strcpy (path, zi->server_path_prefix);
    strcat (path, FNAME_MAIN_LOCK);
    fd = open (path, O_BINARY|O_RDONLY);
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
