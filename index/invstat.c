/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: invstat.c,v $
 * Revision 1.9  1999-02-12 13:29:23  adam
 * Implemented position-flag for registers.
 *
 * Revision 1.8  1999/02/02 14:50:53  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.7  1998/03/13 15:30:50  adam
 * New functions isc_block_used and isc_block_size. Fixed 'leak'
 * in isc_alloc_block.
 *
 * Revision 1.6  1998/03/06 13:54:02  adam
 * Fixed two nasty bugs in isc_merge.
 *
 * Revision 1.5  1997/09/17 12:19:13  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.4  1996/11/08 11:10:21  adam
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
    int no_isam_entries[8];
    int no_dict_entries;
    int no_dict_bytes;
    int isam_bounds[20];
    int isam_occurrences[20];
    char tmp[128];
};

static int inv_stat_handle (char *name, const char *info, int pos,
                            void *client)
{
    int occur = 0;
    int i = 0;
    struct inv_stat_info *stat_info = (struct inv_stat_info*) client;
    ISAM_P isam_p;

    stat_info->no_dict_entries++;
    stat_info->no_dict_bytes += strlen(name);

    assert (*info == sizeof(ISAM_P));
    memcpy (&isam_p, info+1, sizeof(ISAM_P));

    printf ("---\n");
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
	struct it_key key;

        pp = isc_pp_open (stat_info->isamc, isam_p);
        occur = isc_pp_num (pp);
        while (isc_pp_read(pp, &key))
	{
	    printf ("sysno=%d seqno=%d\n", key.sysno, key.seqno);
            occurx++;
	}
        assert (occurx == occur);
	stat_info->no_isam_entries[isc_type(isam_p)] += occur;
        isc_pp_close (pp);
    }

    while (occur > stat_info->isam_bounds[i] && stat_info->isam_bounds[i])
        i++;
    ++(stat_info->isam_occurrences[i]);
    return 0;
}

void inv_prstat (BFiles bfs)
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

    dict = dict_open (bfs, FNAME_DICT, 100, 0);
    if (!dict)
    {
        logf (LOG_FATAL, "dict_open fail");
        exit (1);
    }
    if (!res_get_match (common_resource, "isam", "i", NULL))
    {
        isamc = isc_open (bfs, FNAME_ISAMC, 0, key_isamc_m (common_resource));
        if (!isamc)
        {
            logf (LOG_FATAL, "isc_open fail");
            exit (1);
        }
    }
    else
    {
        isam = is_open (bfs, FNAME_ISAM, key_compare, 0,
			sizeof(struct it_key), common_resource);
        if (!isam)
        {
            logf (LOG_FATAL, "is_open fail");
            exit (1);
        }
    }
    records = rec_open (bfs, 0);

    for (i = 0; i<8; i++)
	stat_info.no_isam_entries[i] = 0;
    stat_info.no_dict_entries = 0;
    stat_info.no_dict_bytes = 0;
    stat_info.isam = isam;
    stat_info.isamc = isamc;
    stat_info.isam_bounds[0] = 1;
    stat_info.isam_bounds[1] = 2;
    stat_info.isam_bounds[2] = 3;
    stat_info.isam_bounds[3] = 6;
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

    if (isamc)
    {
	fprintf (stderr, "   Blocks    Occur  Size KB   Bytes/Entry\n");
	for (i = 0; isc_block_used (isamc, i) >= 0; i++)
	{
	    fprintf (stderr, " %8d %8d", isc_block_used (isamc, i),
		     stat_info.no_isam_entries[i]);

	    if (stat_info.no_isam_entries[i])
		fprintf (stderr, " %8d   %f",
			 (int) ((1023.0 + (double) isc_block_used(isamc, i) *
				 isc_block_size(isamc,i))/1024),
			 ((double) isc_block_used(isamc, i) *
			  isc_block_size(isamc,i))/
			 stat_info.no_isam_entries[i]);
	    fprintf (stderr, "\n");
	}
    }
	
    fprintf (stderr, "\n%d words using %d bytes\n",
             stat_info.no_dict_entries, stat_info.no_dict_bytes);
    fprintf (stderr, "    Occurrences     Words\n");
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

    rec_close (&records);
    dict_close (dict);

    if (isam)
        is_close (isam);
    if (isamc)
        isc_close (isamc);
    
}
