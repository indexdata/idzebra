/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: locksrv.c,v $
 * Revision 1.15  2000-12-01 17:59:08  adam
 * Fixed bug regarding online updates on WIN32.
 * When zebra.cfg is not available the server will not abort.
 *
 * Revision 1.14  2000/03/15 15:00:30  adam
 * First work on threaded version.
 *
 * Revision 1.13  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.12  1999/02/02 14:50:58  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.11  1998/03/05 08:45:12  adam
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
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "zserver.h"

int zebra_server_lock_init (ZebraService zi)
{
    char path_prefix[1024];

    zi->server_lock_cmt = NULL;
    zi->server_lock_org = NULL;

    zebra_lock_prefix (zi->res, path_prefix);
    zi->server_path_prefix = (char *) xmalloc (strlen(path_prefix)+1);
    strcpy (zi->server_path_prefix, path_prefix);

    logf (LOG_DEBUG, "Locking system initialized");
    return 0;
}

int zebra_server_lock_destroy (ZebraService zi)
{
    xfree (zi->server_path_prefix);
    zebra_lock_destroy (zi->server_lock_cmt);
    zebra_lock_destroy (zi->server_lock_org);
    logf (LOG_DEBUG, "Locking system destroyed");
    return 0;
}

int zebra_server_lock (ZebraService zi, int commitPhase)
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

void zebra_server_unlock (ZebraService zi, int commitPhase)
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

int zebra_server_lock_get_state (ZebraService zi, time_t *timep)
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
        *timep = xstat.st_mtime;

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
