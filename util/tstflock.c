/* $Id: tstflock.c,v 1.16.2.1 2006-10-23 11:37:11 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <assert.h>
#include <stdlib.h>
#include <yaz/test.h>
#include <yaz/log.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <fcntl.h>

#ifdef WIN32
#include <io.h>
#endif

#if YAZ_POSIX_THREADS
#include <pthread.h>
#endif
#ifdef WIN32
#include <windows.h>
#include <process.h>
#endif

#include <zebra-flock.h>
#include <string.h>

static char seq[1000];
static char *seqp = 0;

#define NUM_THREADS 100

#if YAZ_POSIX_THREADS
pthread_cond_t sleep_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

int test_fd = 0;

static void small_sleep()
{
#ifdef WIN32
    Sleep(2);
#else
#if YAZ_POSIX_THREADS
    struct timespec abstime;
    struct timeval now;

    gettimeofday(&now, 0);
    abstime.tv_sec = now.tv_sec;
    abstime.tv_nsec = 1000000 + now.tv_usec * 1000;
    if (abstime.tv_nsec > 1000000000) /* 1s = 1e9 ns */
    {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }
    pthread_mutex_lock(&sleep_mutex);
    pthread_cond_timedwait(&sleep_cond, &sleep_mutex, &abstime);
    pthread_mutex_unlock(&sleep_mutex);
#endif
#endif
}

void *run_func(void *arg)
{
    int i;
    int *pdata = (int*) arg;
    int use_write_lock = *pdata;
    ZebraLockHandle lh = zebra_lock_create(0, "my.LCK");
    for (i = 0; i<2; i++)
    {
        int write_lock = use_write_lock;

        if (use_write_lock == 2) /* random lock */
            write_lock = (rand() & 3) == 3 ? 1 : 0;
            
        if (write_lock)
        {
            zebra_lock_w(lh);

            write(test_fd, "L", 1);
            *seqp++ = 'L';
            small_sleep();
            *seqp++ = 'U';  
            write(test_fd, "U", 1);
          
            zebra_unlock(lh);
        }
        else
        {
            zebra_lock_r(lh);
            
            write(test_fd, "l", 1);
            *seqp++ = 'l';
            small_sleep();
            *seqp++ = 'u';
            write(test_fd, "u", 1);
            
            zebra_unlock(lh);
        }
    }
    zebra_lock_destroy(lh);
    *pdata = 123;
    return 0;
}

#ifdef WIN32
DWORD WINAPI ThreadProc(void *p)
{
    run_func(p);
    return 0;
}
#endif

static void tst_thread(int num, int write_flag)
{
#ifdef WIN32
    HANDLE handles[NUM_THREADS];
    DWORD dwThreadId[NUM_THREADS];
#endif
#if YAZ_POSIX_THREADS
    pthread_t child_thread[NUM_THREADS];
#endif
    int i, id[NUM_THREADS];

    seqp = seq;
    assert (num <= NUM_THREADS);
    for (i = 0; i < num; i++)
    {
        id[i] = write_flag;
#if YAZ_POSIX_THREADS
        pthread_create(&child_thread[i], 0 /* attr */, run_func, &id[i]);
#endif
#ifdef WIN32
        if (1)
        {
            void *pData = &id[i];
            handles[i] = CreateThread(
                NULL,              /* default security attributes */
                0,                 /* use default stack size */
                ThreadProc,        /* thread function */
                pData,             /* argument to thread function */
                0,                 /* use default creation flags */
                &dwThreadId[i]);   /* returns the thread identifier */
        }

#endif
    }
#if YAZ_POSIX_THREADS
    for (i = 0; i<num; i++)
        pthread_join(child_thread[i], 0);
#endif
#ifdef WIN32
    WaitForMultipleObjects(num, handles, TRUE, INFINITE);
#endif
    for (i = 0; i < num; i++)
        YAZ_CHECK(id[i] == 123);
    *seqp++ = '\0';
    logf(LOG_LOG, "tst_thread(%d,%d) returns seq=%s", 
         num, write_flag, seq);
}

static void tst()
{
    tst_thread(4, 1); /* write locks */
    if (1)
    {
        int i = 0;
        while (seq[i])
        {
            YAZ_CHECK_EQ(seq[i], 'L');
            YAZ_CHECK_EQ(seq[i+1], 'U');
            i = i + 2;
        }
    }

    tst_thread(6, 0);  /* read locks */

    tst_thread(20, 2); /* random locks */
}

void fork_tst()
{
#if HAVE_SYS_WAIT_H
    pid_t pid[2];
    int i;

    for (i = 0; i<2; i++)
    {
        pid[i] = fork();
        if (!pid[i])
        {
            tst();
            exit(0);
        }
    }
    for (i = 0; i<2; i++)
    {
        int status;
        waitpid(pid[i], &status, 0);
        YAZ_CHECK(status == 0);
    }
#else
    tst();
#endif
}

int main(int argc, char **argv)
{
    char logname[220];
    YAZ_CHECK_INIT(argc, argv);

    sprintf(logname, "%.200s.log", argv[0]);
    yaz_log_init_file(logname);

    /* ensure the flock system logs in our test */
    yaz_log_init_level(yaz_log_mask_str("flock"));

    zebra_flock_init();

    test_fd = open("tstflock.out", (O_BINARY|O_CREAT|O_RDWR), 0666);
    YAZ_CHECK(test_fd != -1);
    if (test_fd != -1)
    {
        fork_tst();
    }
    YAZ_CHECK_TERM;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

