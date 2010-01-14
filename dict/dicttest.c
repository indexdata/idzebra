/* This file is part of the Zebra server.
   Copyright (C) 1994-2010 Index Data

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
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <idzebra/dict.h>
#include <idzebra/util.h>
#include <idzebra/res.h>
#include <yaz/yaz-util.h>

char *prog;
static Dict dict;

static int look_hits;

static int grep_handler (char *name, const char *info, void *client)
{
    look_hits++;
    printf ("%s\n", name);
    return 0;
}

static int scan_handler (char *name, const char *info, int pos, void *client)
{
    printf ("%s\n", name);
    return 0;
}

int main (int argc, char **argv)
{
    Res my_resource = 0;
    BFiles bfs;
    const char *name = NULL;
    const char *inputfile = NULL;
    const char *config = NULL;
    const char *delete_term = NULL;
    int scan_the_thing = 0;
    int do_delete = 0;
    int range = -1;
    int srange = 0;
    int rw = 0;
    int infosize = 4;
    int cache = 10;
    int ret;
    int unique = 0;
    char *grep_pattern = NULL;
    char *arg;
    int no_of_iterations = 0;
    int no_of_new = 0, no_of_same = 0, no_of_change = 0;
    int no_of_hits = 0, no_of_misses = 0, no_not_found = 0, no_of_deleted = 0;
    int max_pos;
    
    prog = argv[0];
    if (argc < 2)
    {
        fprintf (stderr, "usage:\n "
                 " %s [-d] [-D t] [-S] [-r n] [-p n] [-u] [-g pat] [-s n] "
                 "[-v n] [-i f] [-w] [-c n] config file\n\n",
                 prog);
        fprintf (stderr, "  -d      delete instead of insert\n");
        fprintf (stderr, "  -D t    delete subtree instead of insert\n");
        fprintf (stderr, "  -r n    set regular match range\n");
        fprintf (stderr, "  -p n    set regular match start range\n");
        fprintf (stderr, "  -u      report if keys change during insert\n");
        fprintf (stderr, "  -g p    try pattern n (see -r)\n");
        fprintf (stderr, "  -s n    set info size to n (instead of 4)\n");
        fprintf (stderr, "  -v n    set logging level\n");
        fprintf (stderr, "  -i f    read file with words\n");
        fprintf (stderr, "  -w      insert/delete instead of lookup\n");
        fprintf (stderr, "  -c n    cache size (number of pages)\n");
        fprintf (stderr, "  -S      scan the dictionary\n");
        exit (1);
    }
    while ((ret = options ("D:Sdr:p:ug:s:v:i:wc:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!config)
                config = arg;
            else if (!name)
                name = arg;
            else
            {
                yaz_log (YLOG_FATAL, "too many files specified\n");
                exit (1);
            }
        }
	else if (ret == 'D')
	{
	    delete_term = arg;
	}
        else if (ret == 'd')
            do_delete = 1;
        else if (ret == 'g')
        {
            grep_pattern = arg;
        }
        else if (ret == 'r')
        {
            range = atoi (arg);
        }
        else if (ret == 'p')
        {
            srange = atoi (arg);
        }
        else if (ret == 'u')
        {
            unique = 1;
        }
        else if (ret == 'c')
        {
            cache = atoi(arg);
            if (cache<2)
                cache = 2;
        }
        else if (ret == 'w')
            rw = 1;
        else if (ret == 'i')
            inputfile = arg;
	else if (ret == 'S')
	    scan_the_thing = 1;
        else if (ret == 's')
        {
            infosize = atoi(arg);
        }
        else if (ret == 'v')
        {
            yaz_log_init (yaz_log_mask_str(arg), prog, NULL);
        }
        else
        {
            yaz_log (YLOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!config || !name)
    {
        yaz_log (YLOG_FATAL, "no config and/or dictionary specified");
        exit (1);
    }
    my_resource = res_open(0, 0);
    if (!my_resource)
    {
        yaz_log (YLOG_FATAL, "cannot open resource `%s'", config);
        exit (1);
    }
    res_read_file(my_resource, config);

    bfs = bfs_create (res_get(my_resource, "register"), 0);
    if (!bfs)
    {
        yaz_log (YLOG_FATAL, "bfs_create fail");
        exit (1);
    }
    dict = dict_open (bfs, name, cache, rw, 0, 4096);
    if (!dict)
    {
        yaz_log (YLOG_FATAL, "dict_open fail of `%s'", name);
        exit (1);
    }
    if (inputfile)
    {
        FILE *ipf;
        char ipf_buf[1024];
        int line = 1;
        char infobytes[120];
        memset (infobytes, 0, 120);

        if (!(ipf = fopen(inputfile, "r")))
        {
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "cannot open %s", inputfile);
            exit (1);
        }
        
        while (fgets (ipf_buf, 1023, ipf))
        {
            char *ipf_ptr = ipf_buf;
            sprintf (infobytes, "%d", line);
            for (;*ipf_ptr && *ipf_ptr != '\n';ipf_ptr++)
            {
                if (isalpha(*ipf_ptr) || *ipf_ptr == '_')
                {
                    int i = 1;
                    while (ipf_ptr[i] && (isalnum(ipf_ptr[i]) ||
                                          ipf_ptr[i] == '_'))
                        i++;
                    if (ipf_ptr[i])
                        ipf_ptr[i++] = '\0';
                    if (rw)
                    {
			if (do_delete)
                            switch (dict_delete (dict, ipf_ptr))
                            {
                            case 0:
                                no_not_found++;
                                break;
                            case 1:
                                no_of_deleted++;
                            }
                        else
                            switch(dict_insert (dict, ipf_ptr,
                                                infosize, infobytes))
                            {
                            case 0:
                                no_of_new++;
                                break;
                            case 1:
                                no_of_change++;
                                if (unique)
                                    yaz_log (YLOG_LOG, "%s change\n", ipf_ptr);
                                break;
                            case 2:
                                if (unique)
                                    yaz_log (YLOG_LOG, "%s duplicate\n", ipf_ptr);
                                no_of_same++;
                                break;
                            }
                    }
                    else if(range < 0)
                    {
                        char *cp;

                        cp = dict_lookup (dict, ipf_ptr);
                        if (cp && *cp)
                            no_of_hits++;
                        else
                            no_of_misses++;
                    }
                    else
                    {
                        look_hits = 0;
                        dict_lookup_grep (dict, ipf_ptr, range, NULL,
                                          &max_pos, srange, grep_handler);
                        if (look_hits)
                            no_of_hits++;
                        else
                            no_of_misses++;
                    }
                    ++no_of_iterations;
		    if ((no_of_iterations % 10000) == 0)
		    {
			printf ("."); fflush(stdout);
		    }
                    ipf_ptr += (i-1);
                }
            }
            ++line;
        }
        fclose (ipf);
    }
    if (rw && delete_term)
    {
	yaz_log (YLOG_LOG, "dict_delete_subtree %s", delete_term);
	dict_delete_subtree (dict, delete_term, 0, 0);
    }
    if (grep_pattern)
    {
        if (range < 0)
            range = 0;
        yaz_log (YLOG_LOG, "Grepping '%s'", grep_pattern);
        dict_lookup_grep (dict, grep_pattern, range, NULL, &max_pos,
                          srange, grep_handler);
    }
    if (rw)
    {
        yaz_log (YLOG_LOG, "Iterations.... %d", no_of_iterations);            
        if (do_delete)
        {
            yaz_log (YLOG_LOG, "No of deleted. %d", no_of_deleted);
            yaz_log (YLOG_LOG, "No not found.. %d", no_not_found);
        }
        else
        {
            yaz_log (YLOG_LOG, "No of new..... %d", no_of_new);
            yaz_log (YLOG_LOG, "No of change.. %d", no_of_change);
        }
    }
    else
    {
        yaz_log (YLOG_LOG, "Lookups....... %d", no_of_iterations);
        yaz_log (YLOG_LOG, "No of hits.... %d", no_of_hits);
        yaz_log (YLOG_LOG, "No of misses.. %d", no_of_misses);
    }
    if (scan_the_thing)
    {
	char term_dict[1024];
        
	int before = 1000000;
	int after = 1000000;
	yaz_log (YLOG_LOG, "dict_scan");
	term_dict[0] = 1;
	term_dict[1] = 0;
	dict_scan (dict, term_dict, &before, &after, 0, scan_handler);
    }
    dict_close (dict);
    bfs_destroy (bfs);
    res_close (my_resource);
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

