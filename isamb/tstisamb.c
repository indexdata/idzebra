/* $Id: tstisamb.c,v 1.28 2007-01-15 15:10:17 adam Exp $
   Copyright (C) 1995-2007
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
#include <idzebra/isamb.h>
#include <assert.h>

static int log_level = 0;

static void log_item(int level, const void *b, const char *txt)
{
    int x;
    memcpy(&x, b, sizeof(int));
    yaz_log(log_level, "%s %d", txt, x);
}

static void log_pr(const char *txt)
{
    yaz_log(log_level, "%s", txt);
}

int compare_item(const void *a, const void *b)
{
    int ia, ib;

    memcpy(&ia, a, sizeof(int));
    memcpy(&ib, b, sizeof(int));
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
    memcpy (*dst, *src, sizeof(int));
    (*dst) += sizeof(int);
    (*src) += sizeof(int);
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
};

int code_read(void *vp, char **dst, int *insertMode)
{
    struct read_info *ri = (struct read_info *)vp;
    int x;

    if (ri->no >= ri->max)
	return 0;
    ri->no++;

    x = ri->val;
    memcpy (*dst, &x, sizeof(int));
    (*dst)+=sizeof(int);

    ri->val = ri->val + ri->step;
    *insertMode = ri->insertMode;

#if 0
    yaz_log(log_level, "%d %5d", ri->insertMode, x);
#endif
    return 1;
}

void tst_insert(ISAMB isb, int n)
{
    ISAMC_I isamc_i;
    ISAM_P isamc_p;
    struct read_info ri;
    ISAMB_PP pp;
    char key_buf[20];
    int nerrs = 0;

    /* insert a number of entries */
    ri.no = 0;
    ri.max = n;

    ri.val = 0;
    ri.step = 1;
    ri.insertMode = 1;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    
    isamc_p = 0; /* new list */
    isamb_merge (isb, &isamc_p , &isamc_i);

    /* read the entries */
    pp = isamb_pp_open (isb, isamc_p, 1);

    ri.val = 0;
    while(isamb_pp_read (pp, key_buf))
    {
	int x;
	memcpy (&x, key_buf, sizeof(int));
	if (x != ri.val)
	{
	    yaz_log(YLOG_WARN, "isamb_pp_read. n=%d Got %d (expected %d)",
		    n, x, ri.val);
	    nerrs++;
	}
	else if (nerrs)
	    yaz_log(log_level, "isamb_pp_read. n=%d Got %d",
		    n, x);

	ri.val++;
    }
    if (ri.val != ri.max)
    {
	yaz_log(YLOG_WARN, "ri.max != ri.max (%d != %d)", ri.val, ri.max);
	nerrs++;
    }
    isamb_dump(isb, isamc_p, log_pr);
    isamb_pp_close(pp);

    if (nerrs)
        exit(3);
    /* delete a number of entries (even ones) */
    ri.no = 0;
    ri.max = n - n/2;

    ri.val = 0;
    ri.step = 2;
    ri.insertMode = 0;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    
    isamb_merge (isb, &isamc_p , &isamc_i);

    /* delete a number of entries (odd ones) */
    ri.no = 0;
    ri.max = n/2;

    ri.val = 1;
    ri.step = 2;
    ri.insertMode = 0;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    
    isamb_merge (isb, &isamc_p, &isamc_i);

    if (isamc_p)
    {
	yaz_log(YLOG_WARN, "isamb_merge did not return empty list n=%d",
		n);
	exit(3);
    }
}

void tst_forward(ISAMB isb, int n)
{
    ISAMC_I isamc_i;
    ISAM_P isamc_p;
    struct read_info ri;
    int i;
    ISAMB_PP pp;

    /* insert a number of entries */
    ri.val = 0;
    ri.max = n;

    ri.no = 0;
    ri.step = 1;
    ri.insertMode = 1;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    
    isamc_p = 0;
    isamb_merge (isb, &isamc_p, &isamc_i);

    /* read the entries */
    pp = isamb_pp_open (isb, isamc_p, 1);
    
    for (i = 0; i<ri.max; i +=2 )
    {
	int x = -1;
	int xu = i;
	isamb_pp_forward(pp, &x, &xu);
	if (x != xu && xu != x+1)
	{
	    yaz_log(YLOG_WARN, "isamb_pp_forward (1). Got %d (expected %d)",
		    x, xu);
	    exit(4);
	}
	ri.no++;
    }
    isamb_pp_close(pp);
    
    pp = isamb_pp_open (isb, isamc_p, 1);
    for (i = 0; i<ri.max; i += 100)
    {
	int x = -1;
	int xu = i;
	isamb_pp_forward(pp, &x, &xu);
	if (x != xu && xu != x+1)
	{
	    yaz_log(YLOG_WARN, "isamb_pp_forward (2). Got %d (expected %d)",
		    x, xu);
	    exit(4);
	}
	ri.no++;
    }
    isamb_pp_close(pp);

    isamb_unlink(isb, isamc_p);
}

void tst_x(ISAMB isb)
{
    ISAMC_I isamc_i;
    ISAM_P isamb_p = 0;
    struct read_info ri;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    ri.no = 0;
    ri.max = 500;

    ri.val = 1000;
    ri.step = 1;
    ri.insertMode = 1;

    isamb_merge (isb, &isamb_p , &isamc_i);

    ri.no = 0;
    ri.max = 500;

    ri.val = 1;
    ri.step = 1;
    ri.insertMode = 1;

    isamb_merge (isb, &isamb_p , &isamc_i);
}

void tst_append(ISAMB isb, int n)
{
    ISAMC_I isamc_i;
    ISAM_P isamb_p = 0;
    struct read_info ri;
    int i;
    int chunk = 10;

    for (i = 0; i < n; i += chunk)
    {
	/* insert a number of entries */
	ri.no = 0;
	ri.max = i + chunk;

	ri.val = 0;
	ri.step = 1;
	ri.insertMode = 1;
	
	isamc_i.clientData = &ri;
	isamc_i.read_item = code_read;
	
	isamb_merge (isb, &isamb_p , &isamc_i);
    }
}


struct random_read_info {
    int max;
    int idx;
    int level;
    int *delta;
};

int tst_random_read(void *vp, char **dst, int *insertMode)
{
    struct random_read_info *ri = (struct random_read_info *)vp;
    int x;

    while(ri->idx < ri->max && ri->delta[ri->idx] == ri->level)
    {
	ri->idx++;
	ri->level = 0;
    }
    if (ri->idx >= ri->max)
	return 0;
    
    if (ri->delta[ri->idx] > 0)
    {
	ri->level++;
	*insertMode = 1;
    }
    else
    {
	ri->level--;
	*insertMode = 0;
    }
    x = ri->idx;
    memcpy (*dst, &x, sizeof(int));
    (*dst)+=sizeof(int);

    yaz_log(YLOG_DEBUG, "%d %5d", *insertMode, x);
    return 1;
}

void tst_random(ISAMB isb, int n, int rounds, int max_dups)
{
    ISAM_P isamb_p = 0;

    int *freq = malloc(sizeof(int) * n);
    int *delta = malloc(sizeof(int) * n);
    int i, j;
    for (i = 0; i<n; i++)
	freq[i] = 0;
    
    for (j = 0; j<rounds; j++)
    {
	yaz_log(YLOG_DEBUG, "round %d", j);
	for (i = 0; i<n; i++)
	{
	    if (rand() & 1)
		delta[i] = (rand() % (1+max_dups)) - freq[i];
	    else
		delta[i] = 0;
	}
	if (n)
	{
	    ISAMC_I isamc_i;
	    struct random_read_info ri;
	    
	    ri.delta = delta;
	    ri.idx = 0;
	    ri.max = n;
	    ri.level = 0;
	    
	    isamc_i.clientData = &ri;
	    isamc_i.read_item = tst_random_read;

	    isamb_merge (isb, &isamb_p , &isamc_i);
	}
	
	yaz_log(YLOG_DEBUG, "dump %d", j);
	isamb_dump(isb, isamb_p, log_pr);

	yaz_log(YLOG_DEBUG, "----------------------------");
	for (i = 0; i<n; i++)
	    freq[i] += delta[i];

	if (!isamb_p)
	{
	    for (i = 0; i<n; i++)
		if (freq[i])
		{
		    yaz_log(YLOG_WARN, "isamb_merge returned 0, but "
			    "freq is non-empty");
		    exit(1);
		}
	}
	else
	{
	    int level = 0;
	    int idx = 0;
	    char key_buf[20];
	    ISAMB_PP pp = isamb_pp_open (isb, isamb_p, 1);

	    yaz_log(YLOG_DEBUG, "test %d", j);

	    while(isamb_pp_read (pp, key_buf))
	    {
		int x;
		memcpy (&x, key_buf, sizeof(int));
		yaz_log(YLOG_DEBUG, "Got %d", x);
		while (idx < n && freq[idx] == level)
		{
		    idx++;
		    level = 0;
		}
		if (idx == n)
		{
		    yaz_log(YLOG_WARN, "tst_random: Extra item: %d", x);
		    exit(1);
		}
		if (idx != x)
		{
		    yaz_log(YLOG_WARN, "tst_random: Mismatch %d != %d",
			    x, idx);
		    exit(1);
		}
		level++;
	    }
	    while (idx < n && freq[idx] == level)
	    {
		idx++;
		level = 0;
	    }
	    if (idx != n)
	    {
		yaz_log(YLOG_WARN, "tst_random: Missing item: %d", idx);
		exit(1);
	    }
	    isamb_pp_close(pp);
	}
    }
    free(freq);
    free(delta);
}

/* \fn void tst_minsert(ISAMB isb, int n)
   \brief insert inserts n identical keys, removes n/2, then n-n/2 ..
   \param isb ISAMB handle
   \param n number of keys
*/
void tst_minsert(ISAMB isb, int n)
{
    ISAMC_I isamc_i;
    ISAM_P isamb_p = 0;
    struct read_info ri;

    isamc_i.clientData = &ri;

    /* all have same value = 1 */
    ri.val = 1;  
    ri.step = 0;

    isamc_i.read_item = code_read;

    ri.no = 0;
    ri.max = n;

    ri.insertMode = 1;

    isamb_merge (isb, &isamb_p , &isamc_i);

    isamb_dump(isb, isamb_p, log_pr);
    
    ri.no = 0;
    ri.max = n - n/2;

    ri.insertMode = 0;

    isamb_merge (isb, &isamb_p , &isamc_i);

    ri.no = 0;
    ri.max = n/2;

    ri.insertMode = 0;

    isamb_merge (isb, &isamb_p , &isamc_i);
    if (isamb_p)
    {
	yaz_log(YLOG_WARN, "tst_minsert: isamb_merge should be empty n=%d",
		n);
	exit(1);
    }
}

/* tests for identical keys.. ISAMB does not handle that, so some of the
   tests below fails
*/
static void identical_keys_tests(ISAMB isb)
{
#if 1
    tst_minsert(isb, 10);
#endif
#if 0
    tst_minsert(isb, 600);  /* still fails */
#endif
#if 1
    tst_random(isb, 20, 200, 1);
#endif
#if 1
    tst_random(isb, 5, 200, 2);
#endif

#if 1
    tst_random(isb, 250, 10, 4);
#endif
#if 1
    /* fails if both are executed */
    tst_random(isb, 20000, 10, 4);
    tst_random(isb, 20000, 10, 10);
#endif
#if 1
    tst_random(isb, 250, 100, 10);
#endif
}

int main(int argc, char **argv)
{
    BFiles bfs;
    ISAMB isb;
    ISAMC_M method;
    
    if (argc == 2)
	yaz_log_init_level(YLOG_ALL);
	
    /* setup method (attributes) */
    method.compare_item = compare_item;
    method.log_item = log_item;
    method.codec.start = code_start;
    method.codec.encode = code_item;
    method.codec.decode = code_item;
    method.codec.reset = code_reset;
    method.codec.stop = code_stop;

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
    tst_insert(isb, 1);
    tst_insert(isb, 2);
    tst_insert(isb, 20);
    tst_insert(isb, 100);
    tst_insert(isb, 500);
    tst_insert(isb, 10000);

    tst_forward(isb, 10000);

    tst_x(isb);

    tst_append(isb, 1000);

    if (0)
	identical_keys_tests(isb);
    
    isamb_close(isb);

    /* exit block system */
    bfs_destroy(bfs);
    exit(0);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

