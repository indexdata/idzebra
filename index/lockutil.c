/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: lockutil.c,v 1.15 2002-04-04 14:14:13 adam Exp $
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef WIN32
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

char *zebra_mk_fname (const char *dir, const char *name)
{
    int dlen = dir ? strlen(dir) : 0;
    char *fname = xmalloc (dlen + strlen(name) + 3);

#ifdef WIN32
    if (dlen)
    {
        int last_one = dir[dlen-1];

        if (!strchr ("/\\:", last_one))
            sprintf (fname, "%s\\%s", dir, name);
        else
            sprintf (fname, "%s%s", dir, name);
    }
    else
        sprintf (fname, "%s", name);
#else
    if (dlen)
    {
        int last_one = dir[dlen-1];

        if (!strchr ("/", last_one))
            sprintf (fname, "%s/%s", dir, name);
        else
            sprintf (fname, "%s%s", dir, name);
    }
    else
        sprintf (fname, "%s", name);
#endif
    return fname;
}

ZebraLockHandle zebra_lock_create (const char *dir,
                                   const char *name, int excl_flag)
{
    char *fname = zebra_mk_fname(dir, name);
    ZebraLockHandle h = (ZebraLockHandle) xmalloc (sizeof(*h));

    h->excl_flag = excl_flag;
    h->fd = -1;

    
#ifdef WIN32
    if (!h->excl_flag)
        h->fd = open (name, O_BINARY|O_RDONLY);
    if (h->fd == -1)
        h->fd = open (fname, ((h->excl_flag > 1) ? O_EXCL : 0)|
	    (O_BINARY|O_CREAT|O_RDWR), 0666);
#else
    h->fd= open (fname, ((h->excl_flag > 1) ? O_EXCL : 0)|
	    (O_BINARY|O_CREAT|O_RDWR|O_SYNC), 0666);
#endif
    if (h->fd == -1)
    {
	if (h->excl_flag <= 1)
            logf (LOG_WARN|LOG_ERRNO, "open %s", fname);
	xfree (h);
        h = 0;
    }
    xfree (fname);
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

#ifndef WIN32
static int unixLock (int fd, int type, int cmd)
{
    struct flock area;
    area.l_type = type;
    area.l_whence = SEEK_SET;
    area.l_len = area.l_start = 0L;
    return fcntl (fd, cmd, &area);
}
#endif

int zebra_lock_w (ZebraLockHandle h)
{
#ifdef WIN32
    return _locking (h->fd, _LK_LOCK, 1);
#else
    return unixLock (h->fd, F_WRLCK, F_SETLKW);
#endif
}

int zebra_lock_r (ZebraLockHandle h)
{
#ifdef WIN32
    return _locking (h->fd, _LK_LOCK, 1);
#else
    return unixLock (h->fd, F_RDLCK, F_SETLKW);
#endif
}

int zebra_lock (ZebraLockHandle h)
{
#ifdef WIN32
    return _locking (h->fd, _LK_LOCK, 1);
#else
    return unixLock (h->fd, h->excl_flag ? F_WRLCK : F_RDLCK, F_SETLKW);
#endif
}

int zebra_lock_nb (ZebraLockHandle h)
{
#ifdef WIN32
    return _locking (h->fd, _LK_NBLCK, 1);
#else
    return unixLock (h->fd, h->excl_flag ? F_WRLCK : F_RDLCK, F_SETLK);
#endif
}

int zebra_unlock (ZebraLockHandle h)
{
#ifdef WIN32
    return _locking (h->fd, _LK_UNLCK, 1);
#else
    return unixLock (h->fd, F_UNLCK, F_SETLKW);
#endif
}

int zebra_lock_fd (ZebraLockHandle h)
{
    return h->fd;
}
