/* $Id: flock.c,v 1.1 2006-03-23 09:15:25 adam Exp $
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#include <sys/locking.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <idzebra/flock.h>
#include <yaz/xmalloc.h>

struct zebra_lock_info {
    int fd;
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

ZebraLockHandle zebra_lock_create (const char *dir, const char *name)
{
    char *fname = zebra_mk_fname(dir, name);
    ZebraLockHandle h = (ZebraLockHandle) xmalloc (sizeof(*h));

    h->fd = -1;
#ifdef WIN32
    h->fd = open (name, O_BINARY|O_RDONLY);
    if (h->fd == -1)
        h->fd = open (fname, (O_BINARY|O_CREAT|O_RDWR), 0666);
#else
    h->fd= open (fname, (O_BINARY|O_CREAT|O_RDWR), 0666);
#endif
    if (h->fd == -1)
    {
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

int zebra_unlock (ZebraLockHandle h)
{
#ifdef WIN32
    return _locking (h->fd, _LK_UNLCK, 1);
#else
    return unixLock (h->fd, F_UNLCK, F_SETLKW);
#endif
}

