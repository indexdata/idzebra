/* $Id: locksrv.c,v 1.19 2005-01-15 19:38:26 adam Exp $
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

    yaz_log (YLOG_DEBUG, "Locking system initialized");
    return 0;
}

int zebra_server_lock_destroy (ZebraService zi)
{
    xfree (zi->server_path_prefix);
    zebra_lock_destroy (zi->server_lock_cmt);
    zebra_lock_destroy (zi->server_lock_org);
    yaz_log (YLOG_DEBUG, "Locking system destroyed");
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
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "create %s", path);
            return -1;
        }
        assert (zi->server_lock_org == NULL);

	strcpy (path, zi->server_path_prefix);
	strcat (path, FNAME_ORG_LOCK);
        if (!(zi->server_lock_org = zebra_lock_create (path, 0)))
        {
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "create %s", path);
            return -1;
        }
    }
    if (commitPhase)
    {
        yaz_log (YLOG_DEBUG, "Server locks org");
        zebra_lock (zi->server_lock_org);
    }
    else
    {
        yaz_log (YLOG_DEBUG, "Server locks cmt");
        zebra_lock (zi->server_lock_cmt);
    }
    return 0;
}

void zebra_server_unlock (ZebraService zi, int commitPhase)
{
    if (zi->server_lock_org == NULL)
        return;
    yaz_log (YLOG_DEBUG, "Server unlocks org");
    zebra_unlock (zi->server_lock_org);
    yaz_log (YLOG_DEBUG, "Server unlocks cmt");
    zebra_unlock (zi->server_lock_cmt);
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
