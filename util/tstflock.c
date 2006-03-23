/*
 * Copyright (C) 1995-2005, Index Data ApS
 * See the file LICENSE for details.
 *
 * $Id: tstflock.c,v 1.2 2006-03-23 12:07:12 adam Exp $
 */

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

static char seq[20];
static char *seqp = seq;

#define NUM_THREADS 2

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
    ZebraLockHandle lh = zebra_lock_create(0, "my.LCK");
    for (i = 0; i<2; i++)
    {
        zebra_lock_w(lh);
     
        *seqp++ = 'L';
        small_sleep();
        *seqp++ = 'U';
        
        zebra_unlock(lh);
    }
    zebra_lock_destroy(lh);
    return 0;
}

#ifdef WIN32
DWORD WINAPI ThreadProc(void *p)
{
    run_func(p);
    return 0;
}

static void tst1()
{
    HANDLE handles[NUM_THREADS];
    DWORD dwThreadId[NUM_THREADS];
    int i, id[NUM_THREADS];
    
    for (i = 0; i<NUM_THREADS; i++)
    {
        void *pData = &id[i];
        handles[i] = CreateThread(
            NULL,              // default security attributes
            0,                 // use default stack size  
            ThreadProc,        // thread function 
            pData,             // argument to thread function 
            0,                 // use default creation flags 
            &dwThreadId[i]);      // returns the thread identifier 
    }
    WaitForMultipleObjects(NUM_THREADS, handles, TRUE, INFINITE);
    /* join */
    *seqp++ = '\0';
}
#endif

#if YAZ_POSIX_THREADS
static void tst1()
{
    pthread_t child_thread[2];
    int id1 = 1;
    int id2 = 2;
    pthread_create(&child_thread[0], 0 /* attr */, run_func, &id1);
    pthread_create(&child_thread[1], 0 /* attr */, run_func, &id2);
    pthread_join(child_thread[0], 0);
    pthread_join(child_thread[1], 0);
    *seqp++ = '\0';
}
#endif

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    tst1();
    printf("seq=%s\n", seq);
#if 0
    /* does not pass.. for bug 529 */
    YAZ_CHECK(strcmp(seq, "LULULULU") == 0);
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

