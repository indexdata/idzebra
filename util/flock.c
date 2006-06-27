/* $Id: flock.c,v 1.9 2006-06-27 12:24:14 adam Exp $
   Copyright (C) 1995-2006
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
#include <zebra-lock.h>
#include <yaz/xmalloc.h>
#include <yaz/log.h>

/** have this module (mutex) been initialized? */
static int initialized = 0;

/** mutex for lock_list below */
Zebra_mutex lock_list_mutex;

/** our list of file locked files */
static struct zebra_lock_info *lock_list = 0;

/** the internal handle, with a pointer to each lock file info */
struct zebra_lock_handle {
#ifndef WIN32
    /** so we can call zebra_lock_rdwr_wunlock or zebra_lock_lock_runlock */
    int write_flag;
#endif
    struct zebra_lock_info *p;
};

struct zebra_lock_info {
    /** file descriptor */
    int fd;
    /** full path (xmalloc'ed) */
    char *fname;
    /** reference counter: number of zebra_lock_handles pointing to us */
    int ref_count;
#ifndef WIN32
    /** number of file write locks/read locks */
    int no_file_write_lock;
    int no_file_read_lock;
    Zebra_lock_rdwr rdwr_lock;
    Zebra_mutex file_mutex;
#endif
    /** next in lock list */
    struct zebra_lock_info *next;
};

static int log_level = 0;

char *zebra_mk_fname(const char *dir, const char *name)
{
    int dlen = dir ? strlen(dir) : 0;
    char *fname = xmalloc(dlen + strlen(name) + 3);
    
#ifdef WIN32
    if (dlen)
    {
        int last_one = dir[dlen-1];
        
        if (!strchr("/\\:", last_one))
            sprintf(fname, "%s\\%s", dir, name);
        else
            sprintf(fname, "%s%s", dir, name);
    }
    else
        sprintf(fname, "%s", name);
#else
    if (dlen)
    {
        int last_one = dir[dlen-1];

        if (!strchr("/", last_one))
            sprintf(fname, "%s/%s", dir, name);
        else
            sprintf(fname, "%s%s", dir, name);
    }
    else
        sprintf(fname, "%s", name);
#endif
    return fname;
}

ZebraLockHandle zebra_lock_create(const char *dir, const char *name)
{
    char *fname = zebra_mk_fname(dir, name);
    struct zebra_lock_info *p = 0;
    ZebraLockHandle h = 0;

    assert(initialized);

    zebra_mutex_lock(&lock_list_mutex);
#ifndef WIN32 
    for (p = lock_list; p ; p = p->next)
        if (!strcmp(p->fname, fname))
            break;
#endif
    if (!p)
    {
        p = (struct zebra_lock_info *) xmalloc(sizeof(*p));
        
        p->ref_count = 0;
#ifdef WIN32
        p->fd = open(name, O_BINARY|O_RDONLY);
        if (p->fd == -1)
            p->fd = open(fname, (O_BINARY|O_CREAT|O_RDWR), 0666);
#else
        p->fd = open(fname, (O_BINARY|O_CREAT|O_RDWR), 0666);
#endif
        if (p->fd == -1)
        {
            xfree(p);
            yaz_log(YLOG_WARN | YLOG_ERRNO, 
                    "zebra_lock_create fail fname=%s", fname);
            p = 0;
        }
        else
        {
            p->fname = fname;
            fname = 0;  /* fname buffer now owned by p->fname */
            yaz_log(log_level, "zebra_lock_create fd=%d p=%p fname=%s",
                    p->fd, p, p->fname);
#ifndef WIN32
            zebra_lock_rdwr_init(&p->rdwr_lock);
            zebra_mutex_init(&p->file_mutex);
            p->no_file_write_lock = 0;
            p->no_file_read_lock = 0;
#endif
            p->next = lock_list;
            lock_list = p;
        }
    }
    if (p)
    {
        p->ref_count++;
        h = (ZebraLockHandle) xmalloc(sizeof(*h));
        h->p = p;
#ifndef WIN32
        h->write_flag = 0;
#endif
    }
    zebra_mutex_unlock(&lock_list_mutex);
    xfree(fname); /* free it - if it's still there */

    return h;
}

void zebra_lock_destroy(ZebraLockHandle h)
{
    if (!h)
	return;
    yaz_log(log_level, "zebra_lock_destroy fd=%d p=%p fname=%s",
            h->p->fd, h, h->p->fname);
    zebra_mutex_lock(&lock_list_mutex);
    yaz_log(log_level, "zebra_lock_destroy fd=%d p=%p fname=%s 1",
            h->p->fd, h, h->p->fname);
    assert(h->p->ref_count > 0);
    --(h->p->ref_count);
    if (h->p->ref_count == 0)
    {
        struct zebra_lock_info **hp = &lock_list;
        while (*hp)
        {
            if (*hp == h->p)
            {
                *hp = h->p->next;
                break;
            }
            else
                hp = &(*hp)->next;
        }
#ifndef WIN32
        zebra_lock_rdwr_destroy(&h->p->rdwr_lock);
        zebra_mutex_destroy(&h->p->file_mutex);
#endif
        if (h->p->fd != -1)
            close(h->p->fd);
        xfree(h->p->fname);
        xfree(h->p);
    }
    xfree(h);
    zebra_mutex_unlock(&lock_list_mutex);
}

#ifndef WIN32
static int unixLock(int fd, int type, int cmd)
{
    struct flock area;
    area.l_type = type;
    area.l_whence = SEEK_SET;
    area.l_len = area.l_start = 0L;
    return fcntl(fd, cmd, &area);
}
#endif

int zebra_lock_w(ZebraLockHandle h)
{
    int r;
    yaz_log(log_level, "zebra_lock_w fd=%d p=%p fname=%s", 
            h->p->fd, h, h->p->fname);
    
#ifdef WIN32
    while ((r = _locking(h->p->fd, _LK_LOCK, 1)))
        ;
#else
    zebra_mutex_lock(&h->p->file_mutex);
    if (h->p->no_file_write_lock == 0)
    {
        /* if there is already a read lock.. upgrade to write lock */
        r = unixLock(h->p->fd, F_WRLCK, F_SETLKW);
    }
    h->p->no_file_write_lock++;
    zebra_mutex_unlock(&h->p->file_mutex);

    zebra_lock_rdwr_wlock(&h->p->rdwr_lock);
    h->write_flag = 1;
#endif
    return r;
}

int zebra_lock_r(ZebraLockHandle h)
{
    int r;
    yaz_log(log_level, "zebra_lock_r fd=%d p=%p fname=%s", 
            h->p->fd, h, h->p->fname);
#ifdef WIN32
    while ((r = _locking(h->p->fd, _LK_LOCK, 1)))
        ;
#else
    zebra_mutex_lock(&h->p->file_mutex);
    if (h->p->no_file_read_lock == 0 && h->p->no_file_write_lock == 0)
    {
        /* only read lock if no write locks already */
        r = unixLock(h->p->fd, F_RDLCK, F_SETLKW);
    }
    h->p->no_file_read_lock++;
    zebra_mutex_unlock(&h->p->file_mutex);

    zebra_lock_rdwr_rlock(&h->p->rdwr_lock);
    h->write_flag = 0;
#endif
    return r;
}

int zebra_unlock(ZebraLockHandle h)
{
    int r = 0;
    yaz_log(log_level, "zebra_unlock fd=%d p=%p fname=%s",
            h->p->fd, h, h->p->fname);
#ifdef WIN32
    r = _locking(h->p->fd, _LK_UNLCK, 1);
#else
    if (h->write_flag)
        zebra_lock_rdwr_wunlock(&h->p->rdwr_lock);
    else
        zebra_lock_rdwr_runlock(&h->p->rdwr_lock);

    zebra_mutex_lock(&h->p->file_mutex);
    if (h->write_flag)
        h->p->no_file_write_lock--;
    else
        h->p->no_file_read_lock--;
    if (h->p->no_file_read_lock == 0 && h->p->no_file_write_lock == 0)
    {
        r = unixLock(h->p->fd, F_UNLCK, F_SETLKW);
    }
    zebra_mutex_unlock(&h->p->file_mutex);
#endif
    return r;
}

void zebra_flock_init()
{
    if (!initialized)
    {
        log_level = yaz_log_module_level("flock");
        initialized = 1;
        zebra_mutex_init(&lock_list_mutex);
    }
    yaz_log(log_level, "zebra_flock_init");
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

