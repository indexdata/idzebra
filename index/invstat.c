/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: invstat.c,v $
 * Revision 1.4  1996-11-08 11:10:21  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.3  1996/06/04 10:18:58  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.2  1996/05/22  08:25:56  adam
 * Minor change.
 *
 * Revision 1.1  1996/05/14 14:04:34  adam
 * In zebraidx, the 'stat' command is improved. Statistics about ISAM/DICT
 * is collected.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "index.h"
#include "recindex.h"

struct inv_stat_info {
    ISAM isam;
    ISAMC isamc;
    int no_dict_entries;
    int no_dict_bytes;
    int isam_bounds[20];
    int isam_occurrences[20];
    char tmp[128];
};

static int inv_stat_handle (char *name, const char *info, int pos,
                            void *client)
{
    int occur;
    int i = 0;
    struct inv_stat_info *stat_info = (struct inv_stat_info*) client;
    ISAM_P isam_p;

    stat_info->no_dict_entries++;
    stat_info->no_dict_bytes += strlen(name);

    assert (*info == sizeof(ISAM_P));
    memcpy (&isam_p, info+1, sizeof(ISAM_P));

    if (stat_info->isam)
    {
        ISPT ispt;

        ispt = is_position (stat_info->isam, isam_p);
        occur = is_numkeys (ispt);
        is_pt_free (ispt);
    }
    if (stat_info->isamc)
    {
        ISAMC_PP pp;
        int occurx = 0;
        char buf[128];

        pp = isc_pp_open (stat_info->isamc, isam_p);
        occur = isc_pp_num (pp);
#if 1
        while (isc_pp_read(pp, buf))
            occurx++;
        assert (occurx == occur);
#endif
        isc_pp_close (pp);
    }

    while (occur > stat_info->isam_bounds[i] && stat_info->isam_bounds[i])
        i++;
    ++(stat_info->isam_occurrences[i]);

    return 0;
}

void inv_prstat (void)
{
    Dict dict;
    ISAM isam = NULL;
    ISAMC isamc = NULL;
    Records records;
    int i, prev;
    int before = 0;
    int after = 1000000000;
    struct inv_stat_info stat_info;
    char term_dict[2*IT_MAX_WORD+2];

    term_dict[0] = 1;
    term_dict[1] = 0;

    dict = dict_open (FNAME_DICT, 100, 0);
    if (!dict)
    {
        logf (LOG_FATAL, "dict_open fail");
        exit (1);
    }
    if (res_get_match (common_resource, "isam", "c", NULL))
    {
        isamc = isc_open (FNAME_ISAMC, 0, key_isamc_m ());
        if (!isamc)
        {
            logf (LOG_FATAL, "isc_open fail");
            exit (1);
        }
    }
    else
    {
        isam = is_open (FNAME_ISAM, key_compare, 0, sizeof(struct it_key));
        if (!isam)
        {
            logf (LOG_FATAL, "is_open fail");
            exit (1);
        }
    }
    records = rec_open (0);

    stat_info.no_dict_entries = 0;
    stat_info.no_dict_bytes = 0;
    stat_info.isam = isam;
    stat_info.isamc = isamc;
    stat_info.isam_bounds[0] = 1;
    stat_info.isam_bounds[1] = 2;
    stat_info.isam_bounds[2] = 3;
    stat_info.isam_bounds[3] = 5;
    stat_info.isam_bounds[4] = 10;
    stat_info.isam_bounds[5] = 20;
    stat_info.isam_bounds[6] = 30;
    stat_info.isam_bounds[7] = 50;
    stat_info.isam_bounds[8] = 100;
    stat_info.isam_bounds[9] = 200;
    stat_info.isam_bounds[10] = 5000;
    stat_info.isam_bounds[11] = 10000;
    stat_info.isam_bounds[12] = 20000;
    stat_info.isam_bounds[13] = 50000;
    stat_info.isam_bounds[14] = 100000;
    stat_info.isam_bounds[15] = 200000;
    stat_info.isam_bounds[16] = 500000;
    stat_info.isam_bounds[17] = 1000000;
    stat_info.isam_bounds[18] = 0;

    for (i = 0; i<20; i++)
        stat_info.isam_occurrences[i] = 0;

    dict_scan (dict, term_dict, &before, &after, &stat_info, inv_stat_handle);

    rec_close (&records);
    dict_close (dict);
    if (isam)
        is_close (isam);
    if (isamc)
        isc_close (isamc);

    fprintf (stderr, "%d dictionary entries. %d bytes for strings\n",
             stat_info.no_dict_entries, stat_info.no_dict_bytes);
    fprintf (stderr, " size   occurrences\n");
    prev = 1;
    for (i = 0; stat_info.isam_bounds[i]; i++)
    {
        int here = stat_info.isam_bounds[i];
        fprintf (stderr, "%7d-%-7d %7d\n",
                 prev, here, stat_info.isam_occurrences[i]);
        prev = here+1;
    }
    fprintf (stderr, "%7d-        %7d\n",
             prev, stat_info.isam_occurrences[i]);
}
