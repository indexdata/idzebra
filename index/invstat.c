/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 * log at eof
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "index.h"
#include "../isamc/isamd-p.h"

struct inv_stat_info {
    ZebraHandle zh;
    int no_isam_entries[9];
    int no_dict_entries;
    int no_dict_bytes;
    int isam_bounds[20];
    int isam_occurrences[20];
    char tmp[128];
};

#define SINGLETON_TYPE 8 /* the type to use for singletons that */ 
                         /* have no block and no block type */

static int inv_stat_handle (char *name, const char *info, int pos,
                            void *client)
{
    int occur = 0;
    int i = 0;
    struct inv_stat_info *stat_info = (struct inv_stat_info*) client;
    ISAMS_P isam_p;

    stat_info->no_dict_entries++;
    stat_info->no_dict_bytes += strlen(name);

    assert (*info == sizeof(ISAMS_P));
    memcpy (&isam_p, info+1, sizeof(ISAMS_P));

    if (stat_info->zh->reg->isams)
    {
        ISAMS_PP pp;
        int occurx = 0;
	struct it_key key;

        pp = isams_pp_open (stat_info->zh->reg->isams, isam_p);
        occur = isams_pp_num (pp);
        while (isams_pp_read(pp, &key))
	{
	    //printf ("sysno=%d seqno=%d\n", key.sysno, key.seqno);
            occurx++;
	}
        assert (occurx == occur);
	stat_info->no_isam_entries[0] += occur;
        isams_pp_close (pp);
    }
    if (stat_info->zh->reg->isam)
    {
        ISPT ispt;

        ispt = is_position (stat_info->zh->reg->isam, isam_p);
        occur = is_numkeys (ispt);
        is_pt_free (ispt);
    }
    if (stat_info->zh->reg->isamc)
    {
        ISAMC_PP pp;
        int occurx = 0;
	struct it_key key;

        pp = isc_pp_open (stat_info->zh->reg->isamc, isam_p);
        occur = isc_pp_num (pp);
        while (isc_pp_read(pp, &key))
	{
	    //printf ("sysno=%d seqno=%d\n", key.sysno, key.seqno);
            occurx++;
	}
        assert (occurx == occur);
	stat_info->no_isam_entries[isc_type(isam_p)] += occur;
        isc_pp_close (pp);
    }
    if (stat_info->zh->reg->isamd)
    {
        ISAMD_PP pp;
        int occurx = 0;
	struct it_key key;

        pp = isamd_pp_open (stat_info->zh->reg->isamd, isam_p);
        
        occur = isamd_pp_num (pp);
        while (isamd_pp_read(pp, &key))
	{
            occurx++;
            if ( pp->is->method->debug >8 )
	       logf (LOG_LOG,"sysno=%d seqno=%d (%x/%x) oc=%d/%d ofs=%d ",
	           key.sysno, key.seqno,
	           key.sysno, key.seqno,
	           occur,occurx, pp->offset);
	}
        if ( pp->is->method->debug >7 )
	   logf(LOG_LOG,"item %d=%d:%d says %d keys, counted %d",
	      isam_p, isamd_type(isam_p), isamd_block(isam_p),
	      occur, occurx); 
        if (occurx != occur) 
          logf(LOG_LOG,"Count error!!! read %d, counted %d", occur, occurx);
        assert (occurx == occur);
        if ( is_singleton(isam_p) )
  	    stat_info->no_isam_entries[SINGLETON_TYPE] += occur;
	else
	    stat_info->no_isam_entries[isamd_type(isam_p)] += occur;
        isamd_pp_close (pp);
    }
    if (stat_info->zh->reg->isamb)
    {
        ISAMB_PP pp;
        struct it_key key;

        pp = isamb_pp_open(stat_info->zh->reg->isamb, isam_p);
        while (isamb_pp_read(pp, &key))
            occur++;
        isamb_pp_close (pp);
    }

    while (occur > stat_info->isam_bounds[i] && stat_info->isam_bounds[i])
        i++;
    ++(stat_info->isam_occurrences[i]);
    return 0;
}

void zebra_register_statistics (ZebraHandle zh)
{
    int blocks;
    int size;
    int count;
    int i, prev;
    int before = 0;
    int after = 1000000000;
    struct inv_stat_info stat_info;
    char term_dict[2*IT_MAX_WORD+2];

    if (zebra_begin_read (zh))
	return;

    stat_info.zh = zh;

    term_dict[0] = 1;
    term_dict[1] = 0;

    for (i = 0; i<=SINGLETON_TYPE; i++)
	stat_info.no_isam_entries[i] = 0;
    stat_info.no_dict_entries = 0;
    stat_info.no_dict_bytes = 0;
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

    dict_scan (zh->reg->dict, term_dict, &before, &after, &stat_info,
               inv_stat_handle);

    if (zh->reg->isamc)
    {
	fprintf (stderr, "   Blocks    Occur  Size KB   Bytes/Entry\n");
	for (i = 0; isc_block_used (zh->reg->isamc, i) >= 0; i++)
	{
	    fprintf (stderr, " %8d %8d", isc_block_used (zh->reg->isamc, i),
		     stat_info.no_isam_entries[i]);

	    if (stat_info.no_isam_entries[i])
		fprintf (stderr, " %8d   %f",
			 (int) ((1023.0 + (double)
                                 isc_block_used(zh->reg->isamc, i) *
				 isc_block_size(zh->reg->isamc,i))/1024),
			 ((double) isc_block_used(zh->reg->isamc, i) *
			  isc_block_size(zh->reg->isamc,i))/
			 stat_info.no_isam_entries[i]);
	    fprintf (stderr, "\n");
	}
    }
    if (zh->reg->isamd)
    {
	fprintf (stderr, "   Blocks   Occur      KB Bytes/Entry\n");
	if (zh->reg->isamd->method->debug >0) 
            logf(LOG_LOG,"   Blocks   Occur      KB Bytes/Entry");
	for (i = 0; i<=SINGLETON_TYPE; i++)
	{
	    blocks= isamd_block_used(zh->reg->isamd,i);
	    size= isamd_block_size(zh->reg->isamd,i);
	    count=stat_info.no_isam_entries[i];
	    if (i==SINGLETON_TYPE) 
	        blocks=size=0;
	    if (stat_info.no_isam_entries[i]) 
	    {
		fprintf (stderr, "%c %7d %7d %7d %5.2f\n",
    		         (i==SINGLETON_TYPE)?('z'):('A'+i),
    		         blocks,
    		         count,
    		    	 (int) ((1023.0 + (double) blocks * size)/1024),
    			 ((double) blocks * size)/count);
	        if (zh->reg->isamd->method->debug >0) 
    		    logf(LOG_LOG, "%c %7d %7d %7d %5.2f",
    		         (i==SINGLETON_TYPE)?('z'):('A'+i),
    		         blocks,
    		         count,
    		    	 (int) ((1023.0 + (double) blocks * size)/1024),
    			 ((double) blocks * size)/count);
	    } /* entries */
	} /* for */
    } /* isamd */
    if ( (zh->reg->isamd) && (zh->reg->isamd->method->debug>0))
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
    xmalloc_trav("unfreed"); /*! while hunting memory leaks */    
    zebra_end_read (zh);
}


/*
 *
 * $Log: invstat.c,v $
 * Revision 1.25  2002-04-26 08:44:47  adam
 * Index statistics working again
 *
 * Revision 1.24  2002/04/05 08:46:26  adam
 * Zebra with full functionality
 *
 * Revision 1.23  2002/04/04 14:14:13  adam
 * Multiple registers (alpha early)
 *
 * Revision 1.22  2002/02/20 17:30:01  adam
 * Work on new API. Locking system re-implemented
 *
 * Revision 1.21  2000/07/13 10:14:20  heikki
 * Removed compiler warnings when making zebra
 *
 * Revision 1.20  1999/12/01 13:30:30  adam
 * Updated configure for Zmbol/Zebra dependent settings.
 *
 * Revision 1.19  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.18  1999/10/06 11:46:36  heikki
 * mproved statistics on isam-d
 *
 * Revision 1.17  1999/08/20 08:28:37  heikki
 * Log levels
 *
 * Revision 1.16  1999/08/18 08:38:22  heikki
 * Memory leak hunting
 *
 * Revision 1.15  1999/08/18 08:34:53  heikki
 * isamd
 *
 * Revision 1.14  1999/07/14 10:59:26  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.13  1999/07/08 14:23:27  heikki
 * Fixed a bug in isamh_pp_read and cleaned up a bit
 *
 * Revision 1.12  1999/07/06 12:28:04  adam
 * Updated record index structure. Format includes version ID. Compression
 * algorithm ID is stored for each record block.
 *
 * Revision 1.11  1999/05/15 14:36:38  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.10  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.9  1999/02/12 13:29:23  adam
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
 */
