/* $Id: tstisamb.c,v 1.5 2004-06-02 12:30:32 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

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

#include <string.h>
#include <yaz/xmalloc.h>
#include <yaz/log.h>
#include <isamb.h>
#include <assert.h>

static void log_item(int level, const void *b, const char *txt)
{
    int x;
    memcpy(&x, b, sizeof(int));
    yaz_log(LOG_DEBUG, "%s %d", txt, x);
}

static void log_pr(const char *txt)
{
    yaz_log(LOG_DEBUG, "%s", txt);
}

int compare_item(const void *a, const void *b)
{
    int ia, ib;

    memcpy(&ia, a, sizeof(int));
    memcpy(&ib, b, sizeof(int));
    return ia - ib;
}

void *code_start(int mode)
{
    return 0;
}

void code_item(int mode, void *p, char **dst, char **src)
{
    memcpy (*dst, *src, sizeof(int));
    (*dst) += sizeof(int);
    (*src) += sizeof(int);
}

void code_reset(void *p)
{
}
void code_stop(int mode, void *p)
{
}

struct read_info {
    int no;
    int max;
};

int code_read(void *vp, char **dst, int *insertMode)
{
    struct read_info *ri = (struct read_info *)vp;
    int x;

    if (ri->no > ri->max)
	exit(3);
    if (ri->no == ri->max)
	return 0;
    x = ri->no;
    memcpy (*dst, &x, sizeof(int));
    (*dst)+=sizeof(int);

    (ri->no)++;
    *insertMode = 1;
    return 1;
}

void tst_forward(ISAMB isb, int n)
{
    ISAMC_I isamc_i;
    ISAMC_P isamc_p;
    struct read_info ri;
    int i;
    ISAMB_PP pp;
    char key_buf[10];

    /* insert a number of entries */
    ri.no = 0;
    ri.max = n;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    
    isamc_p = isamb_merge (isb, 0 /* new list */ , &isamc_i);

    /* read the entries */
    pp = isamb_pp_open (isb, isamc_p);
    
    for (i = 0; i<ri.max; i++)
    {
	int x = -1;
	int xu = i;
	isamb_pp_forward(pp, &x, &xu);
	if (x != xu)
	{
	    yaz_log(LOG_WARN, "isamb_pp_forward (1). Got %d (expected %d)",
		    x, xu);
	    exit(4);
	}
	ri.no++;
    }
    isamb_pp_close(pp);
    
    pp = isamb_pp_open (isb, isamc_p);
    for (i = 0; i<ri.max; i += 100)
    {
	int x = -1;
	int xuntil = i;
	isamb_pp_forward(pp, &x, &xuntil);
	if (x > xuntil)
	{
	    yaz_log(LOG_WARN, "isamb_pp_forward (2). Got %d (expected %d)",
		    x, xuntil);
	    exit(4);
	}
	ri.no++;
    }
    isamb_pp_close(pp);

    isamb_unlink(isb, isamc_p);
}

void tst_insert(ISAMB isb, int n)
{
    ISAMC_I isamc_i;
    ISAMC_P isamc_p;
    struct read_info ri;
    ISAMB_PP pp;
    char key_buf[10];

    /* insert a number of entries */
    ri.no = 0;
    ri.max = n;

    isamc_i.clientData = &ri;
    isamc_i.read_item = code_read;
    
    isamc_p = isamb_merge (isb, 0 /* new list */ , &isamc_i);

    /* read the entries */
    pp = isamb_pp_open (isb, isamc_p);
    
    ri.no = 0;
    while(isamb_pp_read (pp, key_buf))
    {
	int x;
	memcpy (&x, key_buf, sizeof(int));
	if (x != ri.no)
	{
	    yaz_log(LOG_WARN, "isamb_pp_read. Got %d (expected %d)",
		    x, ri.no);
	    exit(3);
	}
	ri.no++;
    }
    if (ri.no != ri.max)
    {
	yaz_log(LOG_WARN, "ri.max != ri.max (%d != %d)", ri.no, ri.max);
	exit(3);
    }
    isamb_pp_close(pp);

    isamb_dump(isb, isamc_p, log_pr);
    isamb_unlink(isb, isamc_p);
}

int main(int argc, char **argv)
{
    BFiles bfs;
    ISAMB isb;
    ISAMC_M method;
    
    if (argc == 2)
	yaz_log_init_level(LOG_ALL);
	
    /* setup method (attributes) */
    method.compare_item = compare_item;
    method.log_item = log_item;
    method.code_start = code_start;
    method.code_item = code_item;
    method.code_reset = code_reset;
    method.code_stop = code_stop;

    /* create block system */
    bfs = bfs_create(0, 0);
    if (!bfs)
    {
	yaz_log(LOG_WARN, "bfs_create failed");
	exit(1);
    }

    bf_reset(bfs);

    /* create isam handle */
    isb = isamb_open (bfs, "isamb", 1, &method, 0);
    if (!isb)
    {
	yaz_log(LOG_WARN, "isamb_open failed");
	exit(2);
    }
    tst_insert(isb, 1);
    tst_insert(isb, 2);
    tst_insert(isb, 20);
    tst_insert(isb, 100);
    tst_insert(isb, 500);
    tst_insert(isb, 10000);
    tst_forward(isb, 10000);
    /* close isam handle */
    isamb_close(isb);

    /* exit block system */
    bfs_destroy(bfs);
    exit(0);
    return 0;
}
