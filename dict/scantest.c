/* $Id: scantest.c,v 1.1 2004-12-07 14:57:08 adam Exp $
   Copyright (C) 2003,2004
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <yaz/options.h>
#include <dict.h>

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
    hi->ar[idx] = malloc(strlen(name)+1);
    strcpy(hi->ar[idx], name);
#if 0
    printf ("pos=%d idx=%d name=%s\n", pos, idx, name);
#endif
    return 0;
}

int tst(Dict dict, int before, int after, char *scan_term, char **cmp_strs,
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

int main(int argc, char **argv)
{
    BFiles bfs;
    Dict dict;
    int i;
    int errors = 0;
    int ret;
    int before = 0, after = 0, number = 10000;
    char scan_term[1024];
    char *arg;

    strcpy(scan_term, "1004");
    while ((ret = options("b:a:t:n:", argv, argc, &arg)) != -2)
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
	    strcpy(scan_term, arg);
	    break;
	case 'n':
	    number = atoi(arg);
	    break;
	}
    }

    bfs = bfs_create(".:100M", 0);
    if (!bfs)
    {
	fprintf(stderr, "bfs_create failed\n");
	exit(1);
    }
    dict = dict_open(bfs, "dict", 10, 1, 0, 0);
    for (i = 10; i<number; i++)
    {
	int r;
	char w[32];
	sprintf(w, "%d", i);
	r = dict_insert (dict, w, sizeof(int), &i);
    }

    if (after > 0 || before > 0)
	tst(dict, before, after, scan_term, 0, 1);
    else
    {
	if (argc <= 1)
	{
	    char *cs[] = {
		"4497",
		"4498",
		"4499",
	    "45"};
	    strcpy(scan_term, "4499");
	    errors += tst(dict, 2, 2, scan_term, cs, 0);
	}
	if (argc <= 1)
	{
	    char *cs[] = {
		"4498",
		"4499",
		"45",
		"450"};
	    strcpy(scan_term, "45");
	    errors += tst(dict, 2, 2, scan_term, cs, 0);
	}
    }
    dict_close(dict);
    bfs_destroy(bfs);
    if (errors)
	exit(1);
    exit(0);
}
