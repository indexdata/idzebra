/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dicttest.c,v $
 * Revision 1.23  2000-07-07 12:49:20  adam
 * Optimized resultSetInsert{Rank,Sort}.
 *
 * Revision 1.22  1999/02/02 14:50:19  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.21  1996/10/29 14:00:03  adam
 * Page size given by DICT_DEFAULT_PAGESIZE in dict.h.
 *
 * Revision 1.20  1996/03/20 09:35:16  adam
 * Function dict_lookup_grep got extra parameter, init_pos, which marks
 * from which position in pattern approximate pattern matching should occur.
 *
 * Revision 1.19  1996/02/02  13:43:50  adam
 * The public functions simply use char instead of Dict_char to represent
 * search strings. Dict_char is used internally only.
 *
 * Revision 1.18  1996/02/01  20:39:52  adam
 * Bug fix: insert didn't work on 8-bit characters due to unsigned char
 * compares in dict_strcmp (strcmp) and signed Dict_char. Dict_char is
 * unsigned now.
 *
 * Revision 1.17  1995/12/06  17:48:30  adam
 * Bug fix: delete didn't work.
 *
 * Revision 1.16  1995/10/09  16:18:31  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.15  1995/09/04  12:33:31  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.14  1994/10/04  17:46:55  adam
 * Function options now returns arg with error option.
 *
 * Revision 1.13  1994/10/04  12:08:05  adam
 * Some bug fixes and some optimizations.
 *
 * Revision 1.12  1994/10/03  17:23:03  adam
 * First version of dictionary lookup with regular expressions and errors.
 *
 * Revision 1.11  1994/09/28  13:07:09  adam
 * Use log_mask_str now.
 *
 * Revision 1.10  1994/09/26  10:17:24  adam
 * Minor changes.
 *
 * Revision 1.9  1994/09/22  14:43:56  adam
 * First functional version of lookup with error correction. A 'range'
 * specified the maximum number of insertions+deletions+substitutions.
 *
 * Revision 1.8  1994/09/22  10:43:44  adam
 * Two versions of depend. Type 1 is the tail-type compatible with
 * all make programs. Type 2 is the GNU make with include facility.
 * Type 2 is default. depend rule chooses current rule.
 *
 * Revision 1.7  1994/09/19  16:34:26  adam
 * Depend rule change. Minor changes in dicttest.c
 *
 * Revision 1.6  1994/09/16  15:39:12  adam
 * Initial code of lookup - not tested yet.
 *
 * Revision 1.5  1994/09/06  13:05:14  adam
 * Further development of insertion. Some special cases are
 * not properly handled yet! assert(0) are put here. The
 * binary search in each page definitely reduce usr CPU.
 *
 * Revision 1.4  1994/09/01  17:49:37  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 * Revision 1.3  1994/09/01  17:44:06  adam
 * depend include change.
 *
 * Revision 1.2  1994/08/18  12:40:54  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.1  1994/08/16  16:26:47  adam
 * Added dict.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <dict.h>
#include <zebrautl.h>

char *prog;
static Dict dict;

static int look_hits;

static int grep_handle (char *name, const char *info, void *client)
{
    look_hits++;
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
                 " %s [-d] [-r n] [-p n] [-u] [-g pat] [-s n] [-v n] [-i f]"
                 " [-w] [-c n] config file\n\n",
                 prog);
        fprintf (stderr, "  -d      delete instead of insert\n");
        fprintf (stderr, "  -r n    set regular match range\n");
        fprintf (stderr, "  -p n    set regular match start range\n");
        fprintf (stderr, "  -u      report if keys change during insert\n");
        fprintf (stderr, "  -g p    try pattern n (see -r)\n");
        fprintf (stderr, "  -s n    set info size to n (instead of 4)\n");
        fprintf (stderr, "  -v n    set logging level\n");
        fprintf (stderr, "  -i f    read file with words\n");
        fprintf (stderr, "  -w      insert/delete instead of lookup\n");
        fprintf (stderr, "  -c n    cache size (number of pages)\n");
        exit (1);
    }
    while ((ret = options ("dr:p:ug:s:v:i:wc:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!config)
                config = arg;
            else if (!name)
                name = arg;
            else
            {
                logf (LOG_FATAL, "too many files specified\n");
                exit (1);
            }
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
        else if (ret == 's')
        {
            infosize = atoi(arg);
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!config || !name)
    {
        logf (LOG_FATAL, "no config and/or dictionary specified");
        exit (1);
    }
    my_resource = res_open (config);
    if (!my_resource)
    {
        logf (LOG_FATAL, "cannot open resource `%s'", config);
        exit (1);
    }
    bfs = bfs_create (res_get(my_resource, "register"));
    if (!bfs)
    {
        logf (LOG_FATAL, "bfs_create fail");
        exit (1);
    }
    dict = dict_open (bfs, name, cache, rw, 0);
    if (!dict)
    {
        logf (LOG_FATAL, "dict_open fail of `%s'", name);
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
            logf (LOG_FATAL|LOG_ERRNO, "cannot open %s", inputfile);
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
                                    logf (LOG_LOG, "%s change\n", ipf_ptr);
                                break;
                            case 2:
                                if (unique)
                                    logf (LOG_LOG, "%s duplicate\n", ipf_ptr);
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
                                          &max_pos, srange, grep_handle);
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
    if (grep_pattern)
    {
        if (range < 0)
            range = 0;
        logf (LOG_LOG, "Grepping '%s'", grep_pattern);
        dict_lookup_grep (dict, grep_pattern, range, NULL, &max_pos,
                          srange, grep_handle);
    }
    if (rw)
    {
        logf (LOG_LOG, "Iterations.... %d", no_of_iterations);            
        if (do_delete)
        {
            logf (LOG_LOG, "No of deleted. %d", no_of_deleted);
            logf (LOG_LOG, "No not found.. %d", no_not_found);
        }
        else
        {
            logf (LOG_LOG, "No of new..... %d", no_of_new);
            logf (LOG_LOG, "No of change.. %d", no_of_change);
        }
    }
    else
    {
        logf (LOG_LOG, "Lookups....... %d", no_of_iterations);
        logf (LOG_LOG, "No of hits.... %d", no_of_hits);
        logf (LOG_LOG, "No of misses.. %d", no_of_misses);
    }
    dict_close (dict);
    bfs_destroy (bfs);
    res_close (my_resource);
    return 0;
}
