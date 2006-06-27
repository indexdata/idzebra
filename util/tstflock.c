/*
 * Copyright (C) 1995-2006, Index Data ApS
 * See the file LICENSE for details.
 *
 * $Id: tstflock.c,v 1.8 2006-06-27 12:24:14 adam Exp $
 */

#include <assert.h>
#include <yaz/test.h>
#if YAZ_POSIX_THREADS
#include <pthread.h>
#endif
#ifdef WIN32
#include <windows.h>
#include <process.h>
#endif

#include <idzebra/flock.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

static char seq[40];
static char *seqp = seq;

#define NUM_THREADS 5

static void small_sleep()
{
#ifdef WIN32
    Sleep(50);
#else
    sleep(1);
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
        if (use_write_lock)
        {
            zebra_lock_w(lh);
            
            *seqp++ = 'L';
            small_sleep();
            *seqp++ = 'U';
            
            zebra_unlock(lh);
        }
        else
        {
            zebra_lock_r(lh);
            
            *seqp++ = 'l';
            small_sleep();
            *seqp++ = 'u';
            
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

static void tst_win32(int num)
{
    HANDLE handles[NUM_THREADS];
    DWORD dwThreadId[NUM_THREADS];
    int i, id[NUM_THREADS];
    
    assert (num <= NUM_THREADS);
    for (i = 0; i<num; i++)
    {
        void *pData = &id[i];
        id[i] = i >= 2 ? 0 : 1; /* first two are writing.. rest is reading */
        handles[i] = CreateThread(
            NULL,              /* default security attributes */
            0,                 /* use default stack size */
            ThreadProc,        /* thread function */
            pData,             /* argument to thread function */
            0,                 /* use default creation flags */
            &dwThreadId[i]);   /* returns the thread identifier */
    }
    /* join */
    WaitForMultipleObjects(num, handles, TRUE, INFINITE);
}
#endif

#if YAZ_POSIX_THREADS
static void tst_pthread(int num)
{
    pthread_t child_thread[NUM_THREADS];
    int i, id[NUM_THREADS];

    assert (num <= NUM_THREADS);
    for (i = 0; i<num; i++)
    {
        id[i] = i >= 2 ? 0 : 1; /* first two are writing.. rest is reading */
        pthread_create(&child_thread[i], 0 /* attr */, run_func, &id[i]);
    }

    for (i = 0; i<num; i++)
        pthread_join(child_thread[i], 0);

    for (i = 0; i<num; i++)
        YAZ_CHECK(id[i] == 123);
}
#endif

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);

    zebra_flock_init();
#ifdef WIN32
    tst_win32(2);
#endif
#if YAZ_POSIX_THREADS
    tst_pthread(2);
#endif

    *seqp++ = '\0';
#if 0
    printf("seq=%s\n", seq);
#endif
#if 1
    /* does not pass.. for bug 529 */
    YAZ_CHECK(strcmp(seq, "LULULULU") == 0); /* for tst_pthread when num=2 */
#endif
    YAZ_CHECK_TERM;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

