/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dicttest.c,v $
 * Revision 1.13  1994-10-04 12:08:05  adam
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

char *prog;
static Dict dict;

static int look_hits;

static int grep_handle (Dict_char *name, char *info)
{
    look_hits++;
    printf ("%s\n", name);
    return 0;
}

int main (int argc, char **argv)
{
    const char *name = NULL;
    const char *inputfile = NULL;
    const char *base = NULL;
    int range = -1;
    int rw = 0;
    int infosize = 4;
    int cache = 10;
    int ret;
    int unique = 0;
    char *grep_pattern = NULL;
    char *arg;
    int no_of_iterations = 0;
    int no_of_new = 0, no_of_same = 0, no_of_change = 0;
    int no_of_hits = 0, no_of_misses = 0;

    
    prog = argv[0];
    if (argc < 2)
    {
        fprintf (stderr, "usage:\n "
                 " %s [-r n] [-u] [-g pat] [-s n] [-v n] [-i f] [-w] [-c n]"
                 " base file\n",
                 prog);
        exit (1);
    }
    while ((ret = options ("r:ug:s:v:i:wc:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!base)
                base = arg;
            else if (!name)
                name = arg;
            else
            {
                log (LOG_FATAL, "too many files specified\n");
                exit (1);
            }
        }
        else if (ret == 'g')
        {
            grep_pattern = arg;
        }
        else if (ret == 'r')
        {
            range = atoi (arg);
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
            log (LOG_FATAL, "unknown option");
            exit (1);
        }
    }
    if (!base || !name)
    {
        log (LOG_FATAL, "no base and/or dictionary specified");
        exit (1);
    }
    common_resource = res_open (base);
    if (!common_resource)
    {
        log (LOG_FATAL, "cannot open resource `%s'", base);
        exit (1);
    }
    dict = dict_open (name, cache, rw);
    if (!dict)
    {
        log (LOG_FATAL, "dict_open fail of `%s'", name);
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
            log (LOG_FATAL|LOG_ERRNO, "cannot open %s", inputfile);
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
                        switch(dict_insert (dict, ipf_ptr,
                                            infosize, infobytes))
                        {
                        case 0:
                            no_of_new++;
                            break;
                        case 1:
                            no_of_change++;
                        if (unique)
                            log (LOG_LOG, "%s change\n", ipf_ptr);
                            break;
                        case 2:
                            if (unique)
                                log (LOG_LOG, "%s duplicate\n", ipf_ptr);
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
                        dict_lookup_grep (dict, ipf_ptr, range, grep_handle);
                        if (look_hits)
                            no_of_hits++;
                        else
                            no_of_misses++;
                    }
                    ++no_of_iterations;
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
        log (LOG_LOG, "Grepping '%s'", grep_pattern);
        dict_lookup_grep (dict, grep_pattern, range, grep_handle);
    }
    if (rw)
    {
        log (LOG_LOG, "Insertions.... %d", no_of_iterations);
        log (LOG_LOG, "No of new..... %d", no_of_new);
        log (LOG_LOG, "No of change.. %d", no_of_change);
        log (LOG_LOG, "No of same.... %d", no_of_same);
    }
    else
    {
        log (LOG_LOG, "Lookups....... %d", no_of_iterations);
        log (LOG_LOG, "No of hits.... %d", no_of_hits);
        log (LOG_LOG, "No of misses.. %d", no_of_misses);
    }
    dict_close (dict);
    res_close (common_resource);
    return 0;
}
