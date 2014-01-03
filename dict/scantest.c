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
    int start_cut;
    int end_cut;
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

    if (idx < 0)
	return 0;
    if (idx < hi->start_cut || idx > hi->end_cut)
    {
        return 1;
    }

    hi->ar[idx] = malloc(strlen(name)+1);
    strcpy(hi->ar[idx], name);

    return 0;
}

int do_scan(Dict dict, int before, int after, const char *sterm,
            char **cmp_strs,
            int verbose, int start_cut, int end_cut)
{
    struct handle_info hi;
    char scan_term[1024];

    int i;
    int errors = 0;

    strcpy(scan_term, sterm);
    hi.start_cut = start_cut;
    hi.end_cut = end_cut;
    hi.a = after;
    hi.b = before;
    hi.ar = malloc(sizeof(char*) * (after+before+1));
    for (i = 0; i <= after+before; i++)
	hi.ar[i] = 0;
    yaz_log(YLOG_DEBUG, "dict_scan before=%d after=%d term=%s",
            before, after, scan_term);
    dict_scan (dict, scan_term, &before, &after, &hi, handler);
    for (i = 0; i < hi.a+hi.b; i++)
    {
        if (!cmp_strs)
        {
            if (i >= start_cut &&  i <= end_cut)
            {
                if (!hi.ar[i])
                {
                    printf ("--> FAIL i=%d hi.ar[i] == NULL\n", i);
                    errors++;
                }
            }
            else
            {
                if (hi.ar[i])
                {
                    printf ("--> FAIL i=%d hi.ar[i] = %s\n", i, hi.ar[i]);
                    errors++;
                }
            }
        }
        else
	{
            if (i >= start_cut && i <= end_cut)
            {
                if (!hi.ar[i])
                {
                    printf ("--> FAIL i=%d strs == NULL\n", i);
                    errors++;
                }
                else if (!cmp_strs[i])
                {
                    printf ("--> FAIL i=%d cmp_strs == NULL\n", i);
                    errors++;
                }
                else if (strcmp(cmp_strs[i], hi.ar[i]))
                {
                    printf ("--> FAIL i=%d expected %s\n", i, cmp_strs[i]);
                    errors++;
                }
            }
            else
            {
                if (hi.ar[i])
                {
                    printf ("--> FAIL i=%d hi.ar[i] != NULL\n", i);
                    errors++;
                }
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

static void tst(Dict dict, int start, int number)
{
    int i;

    /* insert again with original value again */
    for (i = start; i < number; i += 100)
    {
        int v = i;
        char w[32];
        sprintf(w, "%d", i);
        YAZ_CHECK_EQ(dict_insert(dict, w, sizeof(v), &v), 2);
    }
    /* insert again with different value */
    for (i = start; i < number; i += 100)
    {
        int v = i-1;
        char w[32];
        sprintf(w, "%d", i);
        YAZ_CHECK_EQ(dict_insert(dict, w, sizeof(v), &v), 1);
    }
    /* insert again with original value again */
    for (i = start; i < number; i += 100)
    {
        int v = i;
        char w[32];
        sprintf(w, "%d", i);
        YAZ_CHECK_EQ(dict_insert(dict, w, sizeof(v), &v), 1);
    }

#if 1
    {
        char *cs[] = {
            "4497",
            "4498",
            "4499",
            "45"};
        YAZ_CHECK_EQ(do_scan(dict, 2, 2, "4499", cs, 0, 0, 3), 0);
    }
#endif
#if 1
    {
        char *cs[] = {
            "4498",
            "4499",
            "45",
            "450"};
        YAZ_CHECK_EQ(do_scan(dict, 2, 2, "45", cs, 0, 0, 3), 0);
    }
#endif
#if 1
    /* bug 4592 */
    {
        char *cs[] = {
            "4499",
            "45", /* missing entry ! */
            "450",
            "4500"};
        YAZ_CHECK_EQ(do_scan(dict, 4, 0, "4501", cs, 0, 0, 4), 0);
    }
#endif
#if 1
    {
        char *cs[] = {
            "9996",
            "9997",
            "9998",
            "9999"};
        YAZ_CHECK_EQ(do_scan(dict, 4, 0, "a", cs, 0, 0, 4), 0);
        YAZ_CHECK_EQ(do_scan(dict, 3, 1, "9999", cs, 0, 0, 4), 0);
    }
#endif
#if 1
    {
        char *cs[] = {
            "10",
            "100",
            "1000",
            "1001" };
        YAZ_CHECK_EQ(do_scan(dict, 0, 4, "10", cs, 0, 0, 4), 0);
        YAZ_CHECK_EQ(do_scan(dict, 0, 4, "1", cs, 0, 0, 4), 0);
        YAZ_CHECK_EQ(do_scan(dict, 0, 4, " ", cs, 0, 0, 4), 0);
        YAZ_CHECK_EQ(do_scan(dict, 0, 4, "", cs, 0, 0, 4), 0);
    }
#endif
#if 1
    for (i = 0; i < 20; i++)
        YAZ_CHECK_EQ(do_scan(dict, 20, 20, "45", 0, 0, 20-i, 20+i), 0);
#endif
}

int main(int argc, char **argv)
{
    BFiles bfs = 0;
    Dict dict = 0;
    int i;
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
        dict = dict_open(bfs, "dict", 100, 1, 0, 0);
        YAZ_CHECK(dict);
    }
    if (dict)
    {
        int start = 10;
        /* Insert "10", "11", "12", .. "99", "100", ... number */
        for (i = start; i<number; i++)
        {
            char w[32];
            sprintf(w, "%d", i);
            YAZ_CHECK_EQ(dict_insert(dict, w, sizeof(int), &i), 0);
        }

        if (after > 0 || before > 0)
            do_scan(dict, before, after, scan_term, 0, 1, 0, after+1);
        else
            tst(dict, start, number);

        dict_close(dict);
    }
    if (bfs)
        bfs_destroy(bfs);
    YAZ_CHECK_TERM;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

