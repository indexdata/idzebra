/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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
#include <yaz/options.h>
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <yaz/log.h>
#include <yaz/xmalloc.h>
#include <yaz/timing.h>
#include <idzebra/isamb.h>
#include <assert.h>

static void log_item(int level, const void *b, const char *txt)
{
    int x;
    memcpy(&x, b, sizeof(int));
    yaz_log(YLOG_LOG, "%s %d", txt, x);
}

static void log_pr(const char *txt)
{
    yaz_log(YLOG_LOG, "%s", txt);
}

int compare_item(const void *a, const void *b)
{
    int ia, ib;

    memcpy(&ia, (const char *) a + 1, sizeof(int));
    memcpy(&ib, (const char *) b + 1, sizeof(int));
    if (ia > ib)
	return 1;
    if (ia < ib)
	return -1;
   return 0;
}

void *code_start(void)
{
    return 0;
}

void code_item(void *p, char **dst, const char **src)
{
    int sz = **src;
    memcpy (*dst, *src, sz);
    (*dst) += sz;
    (*src) += sz;
}

void code_reset(void *p)
{
}
void code_stop(void *p)
{
}

struct read_info {
    int val;
    int step;

    int no;
    int max;
    int insertMode;
    int sz;
};

int code_read(void *vp, char **dst, int *insertMode)
{
    struct read_info *ri = (struct read_info *)vp;
    int x;

    if (ri->no >= ri->max)
	return 0;
    ri->no++;

    x = ri->val;
    memset(*dst, 0, ri->sz);
    **dst = ri->sz;
    memcpy(*dst + 1, &x, sizeof(int));

    (*dst) += ri->sz;

    ri->val = ri->val + ri->step;
    *insertMode = ri->insertMode;

#if 0
    yaz_log(YLOG_LOG, "%d %5d", ri->insertMode, x);
#endif
    return 1;
}

void bench_insert(ISAMB isb, int number_of_trees,
                  int number_of_rounds, int number_of_elements,
                  int extra_size)
{
    ISAMC_I isamc_i;
    ISAM_P *isamc_p = xmalloc(sizeof(ISAM_P) * number_of_trees);
    struct read_info ri;
    int round, i;

    for (i = 0; i<number_of_trees; i++)
        isamc_p[i] = 0; /* initially, is empty */

    ri.val = 0;
    ri.step = 1;
    ri.insertMode = 1;
    ri.sz = sizeof(int) + 1 + extra_size;
    
    for (round = 0; round < number_of_rounds; round++)
    {
        yaz_timing_t t = yaz_timing_create();

        yaz_timing_start(t);
        for (i = 0; i<number_of_trees; i++)
        {

            /* insert a number of entries */
            ri.no = 0;
          
            ri.val = (rand());
            if (RAND_MAX < 65536)  
                ri.val = ri.val + 65536*rand();

            // ri.val = number_of_elements * round;
            ri.max = number_of_elements;
            
            isamc_i.clientData = &ri;
            isamc_i.read_item = code_read;
            
            isamb_merge (isb, &isamc_p[i] , &isamc_i);

            if (0)
                isamb_dump(isb, isamc_p[i], log_pr);
        }
        yaz_timing_stop(t);
        printf("%3d %8.6f %5.2f %5.2f\n",
               round+1,
               yaz_timing_get_real(t),
               yaz_timing_get_user(t),
               yaz_timing_get_sys(t));
        yaz_timing_destroy(&t);
    }
    xfree(isamc_p);
}

void exit_usage(void)
{
    fprintf(stderr, "benchisamb [-r rounds] [-n items] [-i isams]\n");
    exit(1);
}

int main(int argc, char **argv)
{
    BFiles bfs;
    ISAMB isb;
    ISAMC_M method;
    int ret;
    char *arg;
    int number_of_rounds = 10;
    int number_of_items = 1000;
    int number_of_isams = 1000;
    int extra_size = 0;
    yaz_timing_t t = 0;
    
    while ((ret = options("z:r:n:i:", argv, argc, &arg)) != -2)
    {
        switch(ret)
        {
        case 'r':
            number_of_rounds = atoi(arg);
            break;
        case 'n':
            number_of_items = atoi(arg);
            break;
        case 'i':
            number_of_isams = atoi(arg);
            break;
        case 'z':
            extra_size = atoi(arg);
            break;
        case 0:
            fprintf(stderr, "bad arg: %s\n", arg);
            exit_usage();
        default:
            fprintf(stderr, "bad option.\n");
            exit_usage();
        }
    }
	
    /* setup method (attributes) */
    method.compare_item = compare_item;
    method.log_item = log_item;
    method.codec.start = code_start;
    method.codec.encode = code_item;
    method.codec.decode = code_item;
    method.codec.reset = code_reset;
    method.codec.stop = code_stop;

    t = yaz_timing_create();
    
    yaz_timing_start(t);

    /* create block system */
    bfs = bfs_create(0, 0);
    if (!bfs)
    {
	yaz_log(YLOG_WARN, "bfs_create failed");
	exit(1);
    }

    bf_reset(bfs);

    /* create isam handle */
    isb = isamb_open (bfs, "isamb", 1, &method, 0);
    if (!isb)
    {
	yaz_log(YLOG_WARN, "isamb_open failed");
	exit(2);
    }
    bench_insert(isb, number_of_isams, number_of_rounds, number_of_items,
                 extra_size);
    
    isamb_close(isb);

    /* exit block system */
    bfs_destroy(bfs);

    yaz_timing_stop(t);
    printf("Total %8.6f %5.2f %5.2f\n",
           yaz_timing_get_real(t),
           yaz_timing_get_user(t),
           yaz_timing_get_sys(t));
    yaz_timing_destroy(&t);
    exit(0);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

