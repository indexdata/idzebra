/*
 * Copyright (C) 1995-2005, Index Data ApS
 * See the file LICENSE for details.
 *
 * $Id: tstflock.c,v 1.1 2006-03-23 09:15:25 adam Exp $
 */

#include <yaz/test.h>
#if YAZ_POSIX_THREADS
#include <pthread.h>
#endif

#include <idzebra/flock.h>
#include <unistd.h>
#include <string.h>

static char seq[20];
static char *seqp = seq;

void *run_func(void *arg)
{
    int i;
    ZebraLockHandle lh = zebra_lock_create(0, "my.LCK");
    for (i = 0; i<2; i++)
    {
        zebra_lock_w(lh);
     
        *seqp++ = 'L';
        sleep(1);
        *seqp++ = 'U';
        
        zebra_unlock(lh);
    }
    zebra_lock_destroy(lh);
    return 0;
}

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

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    tst1();

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

