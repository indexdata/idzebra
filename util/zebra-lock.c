/* This file is part of the Zebra server.
   Copyright (C) Index Data

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



#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <stdio.h>

#include <zebra-lock.h>


int zebra_mutex_init (Zebra_mutex *p)
{
    p->state = 1;
#if YAZ_POSIX_THREADS
    pthread_mutex_init (&p->mutex, 0);
#endif
#ifdef WIN32
    InitializeCriticalSection (&p->mutex);
#endif
    return 0;
}

int zebra_mutex_destroy (Zebra_mutex *p)
{
    --(p->state);
    if (p->state != 0)
    {
        fprintf (stderr, "zebra_mutex_destroy. state = %d\n", p->state);
    }
#if YAZ_POSIX_THREADS
    pthread_mutex_destroy (&p->mutex);
#endif
#ifdef WIN32
    DeleteCriticalSection (&p->mutex);
#endif
    return 0;
}

int zebra_mutex_lock (Zebra_mutex *p)
{
    if (p->state != 1)
    {
        fprintf (stderr, "zebra_mutex_lock. state = %d\n", p->state);
    }
#if YAZ_POSIX_THREADS
    pthread_mutex_lock (&p->mutex);
#endif
#ifdef WIN32
    EnterCriticalSection (&p->mutex);
#endif
    return 0;
}

int zebra_mutex_unlock (Zebra_mutex *p)
{
    if (p->state != 1)
    {
        fprintf (stderr, "zebra_mutex_unlock. state = %d\n", p->state);
    }
#if YAZ_POSIX_THREADS
    pthread_mutex_unlock (&p->mutex);
#endif
#ifdef WIN32
    LeaveCriticalSection (&p->mutex);
#endif
    return 0;
}

int zebra_lock_rdwr_init (Zebra_lock_rdwr *p)
{
    p->readers_reading = 0;
    p->writers_writing = 0;
#if YAZ_POSIX_THREADS
    pthread_mutex_init (&p->mutex, 0);
    pthread_cond_init (&p->lock_free, 0);
#endif
    return 0;
}

int zebra_lock_rdwr_destroy (Zebra_lock_rdwr *p)
{
    assert (p->readers_reading == 0);
    assert (p->writers_writing == 0);
#if YAZ_POSIX_THREADS
    pthread_mutex_destroy (&p->mutex);
    pthread_cond_destroy (&p->lock_free);
#endif
    return 0;
}

int zebra_lock_rdwr_rlock (Zebra_lock_rdwr *p)
{
#if YAZ_POSIX_THREADS
    pthread_mutex_lock (& p->mutex);
    while (p->writers_writing)
        pthread_cond_wait (&p->lock_free, &p->mutex);
    p->readers_reading++;
    pthread_mutex_unlock(&p->mutex);
#endif
    return 0;
}

int zebra_lock_rdwr_wlock (Zebra_lock_rdwr *p)
{
#if YAZ_POSIX_THREADS
    pthread_mutex_lock (&p->mutex);
    while (p->writers_writing || p->readers_reading)
        pthread_cond_wait (&p->lock_free, &p->mutex);
    p->writers_writing++;
    pthread_mutex_unlock (&p->mutex);
#endif
    return 0;
}

int zebra_lock_rdwr_runlock (Zebra_lock_rdwr *p)
{
#if YAZ_POSIX_THREADS
    pthread_mutex_lock (&p->mutex);
    if (p->readers_reading == 0)
    {
        pthread_mutex_unlock (&p->mutex);
        return -1;
    }
    else
    {
        p->readers_reading--;
        if (p->readers_reading == 0)
            pthread_cond_signal (&p->lock_free);
        pthread_mutex_unlock (&p->mutex);
    }
#endif
    return 0;
}

int zebra_lock_rdwr_wunlock (Zebra_lock_rdwr *p)
{
#if YAZ_POSIX_THREADS
    pthread_mutex_lock (&p->mutex);
    if (p->writers_writing == 0)
    {
        pthread_mutex_unlock (&p->mutex);
        return -1;
    }
    else
    {
        p->writers_writing--;
        pthread_cond_broadcast(&p->lock_free);
        pthread_mutex_unlock(&p->mutex);
    }
#endif
    return 0;
}

int zebra_mutex_cond_init (Zebra_mutex_cond *p)
{
#if YAZ_POSIX_THREADS
    pthread_cond_init (&p->cond, 0);
    pthread_mutex_init (&p->mutex, 0);
#endif
    return 0;
}

int zebra_mutex_cond_destroy (Zebra_mutex_cond *p)
{
#if YAZ_POSIX_THREADS
    pthread_cond_destroy (&p->cond);
    pthread_mutex_destroy (&p->mutex);
#endif
    return 0;
}

int zebra_mutex_cond_lock (Zebra_mutex_cond *p)
{
#if YAZ_POSIX_THREADS
    return pthread_mutex_lock (&p->mutex);
#else
    return 0;
#endif
}

int zebra_mutex_cond_unlock (Zebra_mutex_cond *p)
{
#if YAZ_POSIX_THREADS
    return pthread_mutex_unlock (&p->mutex);
#else
    return 0;
#endif
}

int zebra_mutex_cond_wait (Zebra_mutex_cond *p)
{
#if YAZ_POSIX_THREADS
    return pthread_cond_wait (&p->cond, &p->mutex);
#else
    return 0;
#endif
}

int zebra_mutex_cond_signal (Zebra_mutex_cond *p)
{
#if YAZ_POSIX_THREADS
    return pthread_cond_signal (&p->cond);
#else
    return 0;
#endif
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

