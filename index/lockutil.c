/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockutil.c,v $
 * Revision 1.10  1997-09-29 09:08:36  adam
 * Revised locking system to be thread safe for the server.
 *
 * Revision 1.9  1997/09/25 14:54:43  adam
 * WIN32 files lock support.
 *
 * Revision 1.8  1997/09/17 12:19:15  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.7  1997/09/09 13:38:08  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.6  1996/10/29 14:08:14  adam
 * Uses resource lockDir instead of lockPath.
 *
 * Revision 1.5  1996/03/26 16:01:13  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.4  1995/12/13  08:46:10  adam
 * Locking uses F_WRLCK and F_RDLCK again!
 *
 * Revision 1.3  1995/12/12  16:00:57  adam
 * System call sync(2) used after update/commit.
 * Locking (based on fcntl) uses F_EXLCK and F_SHLCK instead of F_WRLCK
 * and F_RDLCK.
 *
 * Revision 1.2  1995/12/11  11:43:29  adam
 * Locking based on fcntl instead of flock.
 * Setting commitEnable removed. Command line option -n can be used to
 * prevent commit if commit setting is defined in the configuration file.
 *
 * Revision 1.1  1995/12/07  17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef WINDOWS
#include <io.h>
#include <sys/locking.h>
#else
#include <unistd.h>
#endif

#include "index.h"

struct zebra_lock_info {
    int fd;
    int excl_flag;
};

ZebraLockHandle zebra_lock_create (const char *name, int excl_flag)
{
    ZebraLockHandle h = xmalloc (sizeof(*h));
    h->excl_flag = excl_flag;
    h->fd = -1;
#ifdef WINDOWS
    if (!h->excl_flag)
        h->fd = open (name, O_BINARY|O_RDONLY);
    if (h->fd == -1)
        h->fd = open (name, ((h->excl_flag > 1) ? O_EXCL : 0)|
	    (O_BINARY|O_CREAT|O_RDWR), 0666);
#else
    h->fd= open (name, ((h->excl_flag > 1) ? O_EXCL : 0)|
	    (O_BINARY|O_CREAT|O_RDWR|O_SYNC), 0666);
#endif
    if (h->fd == -1)
    {
	if (h->excl_flag <= 1)
            logf (LOG_WARN|LOG_ERRNO, "open %s", name);
	xfree (h);
	return NULL;
    }
    return h;
}

void zebra_lock_destroy (ZebraLockHandle h)
{
    if (!h)
	return;
    if (h->fd != -1)
	close (h->fd);
    xfree (h);
}

void zebra_lock_prefix (Res res, char *path)
{
    char *lock_dir = res_get_def (res, "lockDir", "");

    strcpy (path, lock_dir);
    if (*path && path[strlen(path)-1] != '/')
        strcat (path, "/");
}

#ifdef WINDOWS

#else
static int unixLock (int fd, int type, int cmd)
{
    struct flock area;
    area.l_type = type;
    area.l_whence = SEEK_SET;
    area.l_len = area.l_start = 0L;
    return fcntl (fd, cmd, &area);
}
#endif

int zebra_lock (ZebraLockHandle h)
{
#ifdef WINDOWS
    return _locking (h->fd, _LK_LOCK, 1);
#else
    return unixLock (h->fd, h->excl_flag ? F_WRLCK : F_RDLCK, F_SETLKW);
#endif
}

int zebra_lock_nb (ZebraLockHandle h)
{
#ifdef WINDOWS
    return _locking (h->fd, _LK_NBLCK, 1);
#else
    return unixLock (h->fd, h->excl_flag ? F_WRLCK : F_RDLCK, F_SETLK);
#endif
}

int zebra_unlock (ZebraLockHandle h)
{
#ifdef WINDOWS
    return _locking (h->fd, _LK_UNLCK, 1);
#else
    return unixLock (h->fd, F_UNLCK, F_SETLKW);
#endif
}

int zebra_lock_fd (ZebraLockHandle h)
{
    return h->fd;
}
