/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef ZEBRA_LOCK_H
#define ZEBRA_LOCK_H

#ifdef WIN32
#include <windows.h>
#endif
#if YAZ_POSIX_THREADS
#include <pthread.h>
#endif

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

typedef struct {
#ifdef WIN32
    CRITICAL_SECTION mutex;
#else
# if YAZ_POSIX_THREADS
    pthread_mutex_t mutex;
# else
    int dummy;
# endif
#endif
    int state;
} Zebra_mutex;

YAZ_EXPORT int zebra_mutex_init (Zebra_mutex *p);
YAZ_EXPORT int zebra_mutex_destroy (Zebra_mutex *p);
YAZ_EXPORT int zebra_mutex_lock (Zebra_mutex *p);
YAZ_EXPORT int zebra_mutex_unlock (Zebra_mutex *p);

typedef struct {
    int readers_reading;
    int writers_writing;
#if YAZ_POSIX_THREADS
    pthread_mutex_t mutex;
    pthread_cond_t lock_free;
#endif
} Zebra_lock_rdwr;

YAZ_EXPORT int zebra_lock_rdwr_init (Zebra_lock_rdwr *p);
YAZ_EXPORT int zebra_lock_rdwr_destroy (Zebra_lock_rdwr *p);
YAZ_EXPORT int zebra_lock_rdwr_rlock (Zebra_lock_rdwr *p);
YAZ_EXPORT int zebra_lock_rdwr_wlock (Zebra_lock_rdwr *p);
YAZ_EXPORT int zebra_lock_rdwr_runlock (Zebra_lock_rdwr *p);
YAZ_EXPORT int zebra_lock_rdwr_wunlock (Zebra_lock_rdwr *p);

typedef struct {
#if YAZ_POSIX_THREADS
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#else
    int dummy;
#endif
} Zebra_mutex_cond;

YAZ_EXPORT int zebra_mutex_cond_init (Zebra_mutex_cond *p);
YAZ_EXPORT int zebra_mutex_cond_destroy (Zebra_mutex_cond *p);
YAZ_EXPORT int zebra_mutex_cond_lock (Zebra_mutex_cond *p);
YAZ_EXPORT int zebra_mutex_cond_unlock (Zebra_mutex_cond *p);
YAZ_EXPORT int zebra_mutex_cond_wait (Zebra_mutex_cond *p);
YAZ_EXPORT int zebra_mutex_cond_signal (Zebra_mutex_cond *p);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

