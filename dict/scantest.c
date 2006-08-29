/* $Id: scantest.c,v 1.7 2006-08-29 12:31:12 adam Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <yaz/log.h>
#include <yaz/test.h>
#include <yaz/options.h>
#include <idzebra/dict.h>

struct handle_info {
    int b;
    int a;
    char **ar;
};

static int handler(char *name, const char *info, int pos, void *client)
{
    struct handle_info *hi = (struct handle_info *) client;
    int idx;
    if (pos > 0)
	idx = hi->a - pos + hi->b;
    else
	idx = -pos - 1;

    yaz_log(YLOG_DEBUG, "pos=%d idx=%d name=%s", pos, idx, name);
    if (idx < 0)
	return 0;

    hi->ar[idx] = malloc(strlen(name)+1);
    strcpy(hi->ar[idx], name);
    return 0;
}

int do_scan(Dict dict, int before, int after, char *scan_term, char **cmp_strs,
            int verbose)
{
    struct handle_info hi;
    int i;
    int errors = 0;
    hi.a = after;
    hi.b = before;
    hi.ar = malloc(sizeof(char*) * (after+before+1));
    for (i = 0; i<after+before; i++)
	hi.ar[i] = 0;
    yaz_log(YLOG_DEBUG, "dict_scan before=%d after=%d term=%s",
            before, after, scan_term);
    dict_scan (dict, scan_term, &before, &after, &hi, handler);
    for (i = 0; i<hi.a+hi.b; i++)
    {
	if (cmp_strs)
	{
	    if (!cmp_strs[i])
	    {
		printf ("--> FAIL cmp_strs == NULL\n");
		errors++;
	    }
	    else if (!hi.ar[i])
	    {
		printf ("--> FAIL strs == NULL\n");
		errors++;
	    }
	    else if (strcmp(cmp_strs[i], hi.ar[i]))
	    {
		printf ("--> FAIL expected %s\n", cmp_strs[i]);
		errors++;
	    }
	}
	if (verbose || errors)
	{
	    if (i == hi.b)
		printf ("* ");
	    else
		printf ("  ");
	    if (hi.ar[i])
		printf ("%s", hi.ar[i]);
	    else
		printf ("NULL");
	    putchar('\n');
	}
	if (hi.ar[i])
	    free(hi.ar[i]);
    }
    free(hi.ar);
    return errors;
}

static void tst(Dict dict)
{
    char scan_term[1024];
    {
        char *cs[] = {
            "4497",
            "4498",
            "4499",
            "45"};
        strcpy(scan_term, "4499");
        YAZ_CHECK_EQ(do_scan(dict, 2, 2, scan_term, cs, 0), 0);
    }
    {
        char *cs[] = {
            "4498",
            "4499",
            "45",
            "450"};
        strcpy(scan_term, "45");
        YAZ_CHECK_EQ(do_scan(dict, 2, 2, scan_term, cs, 0), 0);
    }
}

int main(int argc, char **argv)
{
    BFiles bfs = 0;
    Dict dict = 0;
    int i;
    int errors = 0;
    int ret;
    int before = 0, after = 0, number = 10000;
    char scan_term[1024];
    char *arg;

    YAZ_CHECK_INIT(argc, argv);

    strcpy(scan_term, "1004");
    while ((ret = options("b:a:t:n:v:", argv, argc, &arg)) != -2)
    {
	switch(ret)
	{
	case 0:
	    break;
	case 'b':
	    before = atoi(arg);
	    break;
	case 'a':
	    after = atoi(arg);
	    break;
	case 't':
            if (strlen(arg) >= sizeof(scan_term)-1)
            {
                fprintf(stderr, "scan term too long\n");
                exit(1);
            }
            strcpy(scan_term, arg);
	    break;
	case 'n':
	    number = atoi(arg);
	    break;
	case 'v':
	    yaz_log_init_level(yaz_log_mask_str(arg));
	}
    }

    bfs = bfs_create(".:100M", 0);
    YAZ_CHECK(bfs);
    if (bfs)
    {
        bf_reset(bfs);
        dict = dict_open(bfs, "dict", 10, 1, 0, 0);
        YAZ_CHECK(dict);
    }
    if (dict)
    {
        /* Insert "10", "11", "12", .. "99", "100", ... number */
        for (i = 10; i<number; i++)
        {
            char w[32];
            sprintf(w, "%d", i);
            YAZ_CHECK_EQ(dict_insert (dict, w, sizeof(int), &i), 0);
        }
        
        if (after > 0 || before > 0)
            do_scan(dict, before, after, scan_term, 0, 1);
        else
            tst(dict);

        dict_close(dict);
    }
    if (bfs)
        bfs_destroy(bfs);
    YAZ_CHECK_TERM;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

