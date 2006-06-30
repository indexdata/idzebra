/* $Id: flock.c,v 1.13 2006-06-30 15:10:20 adam Exp $
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

/** whether lock file desciptors are shared within one process' threads */
#define ZEBRA_FLOCK_SHARE_FD_IN_THREADS 1

/** whether we always lock on the file - even if rd/wr locks should do */
#define ZEBRA_FLOCK_FILE_LOCK_ALWAYS 1

/** whether we should also use pthreads locking to do "extra" rd/wr locks */
#define ZEBRA_FLOCK_EXTRA_RDWR_LOCKS 1

/** whether this module should debug */
#define DEBUG_FLOCK 0

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
#if ZEBRA_FLOCK_EXTRA_RDWR_LOCKS
    Zebra_lock_rdwr rdwr_lock;
#endif
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
    /* see if we have the same filename in a global list of "lock files" */
#ifndef WIN32
#if ZEBRA_FLOCK_SHARE_FD_IN_THREADS
    for (p = lock_list; p ; p = p->next)
        if (!strcmp(p->fname, fname))
            break;
#endif
#endif
    if (!p)
    {   /* didn't match (or we didn't want it to match! */
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
#ifndef WIN32
#if ZEBRA_FLOCK_EXTRA_RDWR_LOCKS
            zebra_lock_rdwr_init(&p->rdwr_lock);
#endif
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
        /* we have lock info so we can make a handle pointing to that */
        p->ref_count++;
        h = (ZebraLockHandle) xmalloc(sizeof(*h));
        h->p = p;
#ifndef WIN32
        h->write_flag = 0;
#endif
        yaz_log(log_level, "zebra_lock_create fd=%d p=%p fname=%s",
                h->p->fd, h, p->fname);
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
    yaz_log(log_level, "zebra_lock_destroy fd=%d p=%p fname=%s refcount=%d",
            h->p->fd, h, h->p->fname, h->p->ref_count);
    assert(h->p->ref_count > 0);
    --(h->p->ref_count);
    if (h->p->ref_count == 0)
    {
        /* must remove shared info from lock_list */
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

        yaz_log(log_level, "zebra_lock_destroy fd=%d p=%p fname=%s remove",
                h->p->fd, h, h->p->fname);

#ifndef WIN32
#if ZEBRA_FLOCK_EXTRA_RDWR_LOCKS
        zebra_lock_rdwr_destroy(&h->p->rdwr_lock);
#endif
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
    int r;
    area.l_type = type;
    area.l_whence = SEEK_SET;
    area.l_len = area.l_start = 0L;

    yaz_log(log_level, "fcntl begin type=%d fd=%d", type, fd);
    r = fcntl(fd, cmd, &area);
    if (r == -1)
        yaz_log(YLOG_WARN|YLOG_ERRNO, "fcntl FAIL type=%d fd=%d", type, fd);
    else
        yaz_log(log_level, "fcntl type=%d OK fd=%d", type, fd);
    
    return r;
}
#endif

int zebra_lock_w(ZebraLockHandle h)
{
    int r;
    int do_lock = ZEBRA_FLOCK_FILE_LOCK_ALWAYS;
    yaz_log(log_level, "zebra_lock_w fd=%d p=%p fname=%s begin", 
            h->p->fd, h, h->p->fname);

#ifdef WIN32
    while ((r = _locking(h->p->fd, _LK_LOCK, 1)))
        ;
#else
#if ZEBRA_FLOCK_EXTRA_RDWR_LOCKS
    zebra_lock_rdwr_wlock(&h->p->rdwr_lock);
#endif

    zebra_mutex_lock(&h->p->file_mutex);
    if (h->p->no_file_write_lock == 0)
        do_lock = 1;
    h->p->no_file_write_lock++;
    if (do_lock)
    {
        /* if there is already a read lock.. upgrade to write lock */
        r = unixLock(h->p->fd, F_WRLCK, F_SETLKW);
    }
    zebra_mutex_unlock(&h->p->file_mutex);

    h->write_flag = 1;
    yaz_log(log_level, "zebra_lock_w fd=%d p=%p fname=%s end", 
            h->p->fd, h, h->p->fname);
#endif

    return r;
}

int zebra_lock_r(ZebraLockHandle h)
{
    int r;
    int do_lock = ZEBRA_FLOCK_FILE_LOCK_ALWAYS;

    yaz_log(log_level, "zebra_lock_r fd=%d p=%p fname=%s", 
            h->p->fd, h, h->p->fname);
#ifdef WIN32
    while ((r = _locking(h->p->fd, _LK_LOCK, 1)))
        ;
#else
#if ZEBRA_FLOCK_EXTRA_RDWR_LOCKS
    zebra_lock_rdwr_rlock(&h->p->rdwr_lock);
#endif

    zebra_mutex_lock(&h->p->file_mutex);
    if (h->p->no_file_read_lock == 0 && h->p->no_file_write_lock == 0)
        do_lock = 1;
    h->p->no_file_read_lock++;
    if (do_lock)
    {
        /* only read lock if no write locks already */
        r = unixLock(h->p->fd, F_RDLCK, F_SETLKW);
    }
    zebra_mutex_unlock(&h->p->file_mutex);
    
    h->write_flag = 0;
#endif
    return r;
}

int zebra_unlock(ZebraLockHandle h)
{
    int r = 0;
    int do_unlock = ZEBRA_FLOCK_FILE_LOCK_ALWAYS;
    yaz_log(log_level, "zebra_unlock fd=%d p=%p fname=%s begin",
            h->p->fd, h, h->p->fname);
#ifdef WIN32
    r = _locking(h->p->fd, _LK_UNLCK, 1);
#else
    zebra_mutex_lock(&h->p->file_mutex);
    if (h->write_flag)
        h->p->no_file_write_lock--;
    else
        h->p->no_file_read_lock--;
    if (h->p->no_file_read_lock == 0 && h->p->no_file_write_lock == 0)
        do_unlock = 1;
    if (do_unlock)
        r = unixLock(h->p->fd, F_UNLCK, F_SETLKW);
    zebra_mutex_unlock(&h->p->file_mutex);

#if ZEBRA_FLOCK_EXTRA_RDWR_LOCKS
    if (h->write_flag)
        zebra_lock_rdwr_wunlock(&h->p->rdwr_lock);
    else
        zebra_lock_rdwr_runlock(&h->p->rdwr_lock);
#endif

    yaz_log(log_level, "zebra_unlock fd=%d p=%p fname=%s end",
            h->p->fd, h, h->p->fname);
#endif
    return r;
}

void zebra_flock_init()
{
    if (!initialized)
    {
        log_level = yaz_log_module_level("flock");
#if DEBUG_FLOCK
        log_level = YLOG_LOG|YLOG_FLUSH;
#endif
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

