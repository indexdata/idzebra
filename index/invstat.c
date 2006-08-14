/* $Id: invstat.c,v 1.35.2.1 2006-08-14 10:38:58 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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
    int isamb_levels[10][5];
    int isamb_sizes[10];
    int isamb_blocks[10];
    unsigned long cksum;
    int dumpwords;
};

#define SINGLETON_TYPE 8 /* the type to use for singletons that */ 
                         /* have no block and no block type */

static void print_dict_item (ZebraMaps zm, const char *s, int count,
            int firstsys, int firstseq, int lastsys, int lastseq )
{
    int reg_type = s[1];
    char keybuf[IT_MAX_WORD+1];
    char *to = keybuf;
    const char *from = s + 2;

    while (*from)
    {
        const char *res = zebra_maps_output (zm, reg_type, &from);
        if (!res)
            *to++ = *from++;
        else
            while (*res)
                *to++ = *res++;
    }
    *to = '\0';
    /* yaz_log (LOG_LOG, "%s", keybuf); */
    printf("%10d %s %d.%d - %d.%d\n",count, keybuf,
              firstsys,firstseq, lastsys,lastseq);
}

static int inv_stat_handle (char *name, const char *info, int pos,
                            void *client)
{
    int occur = 0;
    int i = 0;
    struct inv_stat_info *stat_info = (struct inv_stat_info*) client;
    ISAMS_P isam_p;
    int firstsys=-1;
    int firstseq=-1;
    int lastsys=-1;
    int lastseq=-1;

    stat_info->no_dict_entries++;
    stat_info->no_dict_bytes += strlen(name);

    if (!stat_info->zh->reg->isamd)
    {
        assert (*info == sizeof(ISAMS_P));
        memcpy (&isam_p, info+1, sizeof(ISAMS_P));
    }

    if (stat_info->zh->reg->isams)
    {
        ISAMS_PP pp;
        int occurx = 0;
	struct it_key key;

        pp = isams_pp_open (stat_info->zh->reg->isams, isam_p);
        occur = isams_pp_num (pp);
        while (isams_pp_read(pp, &key))
	{
            stat_info->cksum = stat_info->cksum * 65509 + 
                key.sysno + 11 * key.seqno;
            occurx++;
            if (-1==firstsys)
            {
                firstseq=key.seqno;
                firstsys=key.sysno;
            }
            lastsys=key.sysno;
            lastseq=key.seqno;
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
	stat_info->no_isam_entries[is_type(isam_p)] += occur;
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
            stat_info->cksum = stat_info->cksum * 65509 + 
                key.sysno + 11 * key.seqno;
            occurx++;
            if (-1==firstsys)
            {
                firstseq=key.seqno;
                firstsys=key.sysno;
            }
            lastsys=key.sysno;
            lastseq=key.seqno;
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
        /* printf("[%d: %d %d %d %d %d %d] ", */
        /*    info[0], info[1], info[2], info[3], info[4], info[5], info[7]);*/
        pp = isamd_pp_open (stat_info->zh->reg->isamd, info+1, info[0]);
        
        occur = isamd_pp_num (pp);
        while (isamd_pp_read(pp, &key))
	{
            stat_info->cksum = stat_info->cksum * 65509 + 
                key.sysno + 11 * key.seqno;
            occurx++;
            /* printf("%d.%d ", key.sysno, key.seqno); */ /*!*/
            if (-1==firstsys)
            {
                firstseq=key.seqno;
                firstsys=key.sysno;
            }
            lastsys=key.sysno;
            lastseq=key.seqno;
            if ( pp->is->method->debug >8 )
	       logf (LOG_LOG,"sysno=%d seqno=%d (%x/%x) oc=%d/%d ofs=%d ",
	           key.sysno, key.seqno,
	           key.sysno, key.seqno,
	           occur,occurx, pp->offset);
	}
        /* printf("\n"); */ /*!*/
#ifdef SKIPTHIS
        if ( pp->is->method->debug >7 )
	   logf(LOG_LOG,"item %d=%d:%d says %d keys, counted %d",
	      isam_p, isamd_type(isam_p), isamd_block(isam_p),
	      occur, occurx); 
#endif
        if (occurx != occur) 
          logf(LOG_LOG,"Count error!!! read %d, counted %d", occur, occurx);
        assert (occurx == occur);
        i = pp->cat;
        if (info[1])
            i=SINGLETON_TYPE;
	stat_info->no_isam_entries[i] += occur;
        isamd_pp_close (pp);
    }
    if (stat_info->zh->reg->isamb)
    {
        ISAMB_PP pp;
        struct it_key key;
        int cat = isam_p & 3;
        int level;
        int size;
        int blocks;
        
        pp = isamb_pp_open_x(stat_info->zh->reg->isamb, isam_p, &level);

        while (isamb_pp_read(pp, &key))
        {
            stat_info->cksum = stat_info->cksum * 65509 + 
                key.sysno + 11 * key.seqno;
            occur++;
            if (-1==firstsys)
            {
                firstseq=key.seqno;
                firstsys=key.sysno;
            }
            lastsys=key.sysno;
            lastseq=key.seqno;
        }
        isamb_pp_close_x (pp, &size, &blocks);
        stat_info->isamb_blocks[cat] += blocks;
        stat_info->isamb_sizes[cat] += size;
        if (level > 4)
            level = 4;
        stat_info->isamb_levels[cat][level] ++;
	stat_info->no_isam_entries[cat] += occur;
    }
    i=0;
    while (occur > stat_info->isam_bounds[i] && stat_info->isam_bounds[i])
        i++;
    ++(stat_info->isam_occurrences[i]);
    if (stat_info->dumpwords)
       print_dict_item(stat_info->zh->reg->zebra_maps, name, occur,
          firstsys,firstseq, lastsys, lastseq);
    return 0;
}

int zebra_register_statistics (ZebraHandle zh, int dumpdict)
{
    int blocks;
    int size;
    int count;
    int i, prev;
    int before = 0;
    int occur;
    int after = 1000000000;
    struct inv_stat_info stat_info;
    char term_dict[2*IT_MAX_WORD+2];

    if (zebra_begin_read (zh))
	return 1;

    stat_info.zh = zh;
    stat_info.dumpwords=dumpdict;

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

    stat_info.cksum = 0;

    for (i = 0; i<20; i++)
        stat_info.isam_occurrences[i] = 0;

    for (i = 0; i<10; i++)
    {
        int j;
        for (j = 0; j<5; j++)
            stat_info.isamb_levels[i][j] = 0;
        stat_info.isamb_sizes[i] = 0;
        stat_info.isamb_blocks[i] = 0;
    }

    dict_scan (zh->reg->dict, term_dict, &before, &after, &stat_info,
               inv_stat_handle);

    if (zh->reg->isamc)
    {
	fprintf (stdout, "   Blocks    Occur  Size KB   Bytes/Entry\n");
	for (i = 0; isc_block_used (zh->reg->isamc, i) >= 0; i++)
	{
	    fprintf (stdout, " %8d %8d", isc_block_used (zh->reg->isamc, i),
		     stat_info.no_isam_entries[i]);

	    if (stat_info.no_isam_entries[i])
		fprintf (stdout, " %8d   %f",
			 (int) ((1023.0 + (double)
                                 isc_block_used(zh->reg->isamc, i) *
				 isc_block_size(zh->reg->isamc,i))/1024),
			 ((double) isc_block_used(zh->reg->isamc, i) *
			  isc_block_size(zh->reg->isamc,i))/
			 stat_info.no_isam_entries[i]);
	    fprintf (stdout, "\n");
	}
    }
    if (zh->reg->isamd)
    {
	fprintf (stdout, "   Blocks   Occur      KB Bytes/Entry\n");
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
		fprintf (stdout, "%c %7d %7d %7d %5.2f\n",
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
        fprintf (stdout, "\n%d words using %d bytes\n",
             stat_info.no_dict_entries, stat_info.no_dict_bytes);

    if (zh->reg->isamb)
    {
        for (i = 0; i<4; i++)
        {
            int j;
            int bsize = isamb_block_info(zh->reg->isamb, i);
            if (bsize < 0)
                break;
            fprintf (stdout, "Category   %d\n", i);
            fprintf (stdout, "Block size %d\n", bsize);
            fprintf (stdout, "Blocks:    %d\n", stat_info.isamb_blocks[i]);
            fprintf (stdout, "Size:      %d\n", stat_info.isamb_sizes[i]);
            fprintf (stdout, "Entries:   %d\n", stat_info.no_isam_entries[i]);
            fprintf (stdout, "Total      %d\n", stat_info.isamb_blocks[i]*
                     bsize);
            for (j = 0; j<5; j++)
                if (stat_info.isamb_levels[i][j])
                    fprintf (stdout, "Level%d     %d\n", j,
                             stat_info.isamb_levels[i][j]);
            fprintf (stdout, "\n");
        }
    }
    fprintf (stdout, "Checksum       %08lX\n", stat_info.cksum);

    fprintf (stdout, "Distinct words %d\n", stat_info.no_dict_entries);
    occur = 0;
    for (i = 0; i<9; i++)
        occur += stat_info.no_isam_entries[i];
    fprintf (stdout, "Word pos       %d\n", occur);
    fprintf (stdout, "    Occurrences     Words\n");
    prev = 1;
    for (i = 0; stat_info.isam_bounds[i]; i++)
    {
        int here = stat_info.isam_bounds[i];
        fprintf (stdout, "%7d-%-7d %7d\n",
                 prev, here, stat_info.isam_occurrences[i]);
        prev = here+1;
    }
    fprintf (stdout, "%7d-        %7d\n",
             prev, stat_info.isam_occurrences[i]);
    xmalloc_trav("unfreed"); /*! while hunting memory leaks */    
    zebra_end_read (zh);
    return 0;
}

