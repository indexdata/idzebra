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
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "index.h"

struct inv_stat_info {
    ZebraHandle zh;
    zint no_isam_entries[9];
    int no_dict_entries;
    int no_dict_bytes;
    int isam_bounds[20];
    int isam_occurrences[20];
    char tmp[128];
    int isamb_levels[10][5];
    zint isamb_sizes[10];
    zint isamb_blocks[10];
    unsigned long cksum;
    int dumpwords;
};

#define SINGLETON_TYPE 8 /* the type to use for singletons that */
                         /* have no block and no block type */

static void print_dict_item (ZebraHandle zh, const char *s, zint count,
            int firstsys, int firstseq, int lastsys, int lastseq )
{
    char dst[IT_MAX_WORD+1];
    int ord;
    int len = key_SU_decode(&ord, (const unsigned char *) s);
    const char *index_type;
    const char *db = 0;

    if (!zh)
        *dst = '\0';
    else
    {
        zebraExplain_lookup_ord (zh->reg->zei, ord, &index_type, &db, 0);

        zebra_term_untrans(zh, index_type, dst, s + len);
    }
    printf("%02d:%10" ZINT_FORMAT0 " %s %d.%d - %d.%d\n", ord, count, dst,
           firstsys, firstseq, lastsys, lastseq);
}

static int inv_stat_handle (char *name, const char *info, int pos,
                            void *client)
{
    zint occur = 0;
    int i = 0;
    struct inv_stat_info *stat_info = (struct inv_stat_info*) client;
    ISAM_P isam_p;
    int firstsys=-1;
    int firstseq=-1;
    int lastsys=-1;
    int lastseq=-1;

    stat_info->no_dict_entries++;
    stat_info->no_dict_bytes += strlen(name);

    assert (*info == sizeof(ISAM_P));
    memcpy (&isam_p, info+1, sizeof(ISAM_P));

    if (stat_info->zh->reg->isams)
    {
        ISAMS_PP pp;
        int occurx = 0;
        struct it_key key;

        pp = isams_pp_open (stat_info->zh->reg->isams, isam_p);
        occur = isams_pp_num (pp);
        while (isams_pp_read(pp, &key))
        {
            occurx++;
        }
        assert (occurx == occur);
        stat_info->no_isam_entries[0] += occur;
        isams_pp_close (pp);
    }
    if (stat_info->zh->reg->isamc)
    {
        ISAMC_PP pp;
        zint occurx = 0;
        struct it_key key;

        pp = isamc_pp_open (stat_info->zh->reg->isamc, isam_p);
        occur = isamc_pp_num (pp);
        while (isamc_pp_read(pp, &key))
        {
            occurx++;
        }
        assert (occurx == occur);
        stat_info->no_isam_entries[isamc_type(isam_p)] += occur;
        isamc_pp_close (pp);
    }
    if (stat_info->zh->reg->isamb)
    {
        ISAMB_PP pp;
        struct it_key key;
        int cat = CAST_ZINT_TO_INT(isam_p & 3);
        int level;
        zint size;
        zint blocks;

        pp = isamb_pp_open_x(stat_info->zh->reg->isamb, isam_p, &level, 0);

        while (isamb_pp_read(pp, &key))
        {
            occur++;
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
        print_dict_item(stat_info->zh, name, occur,
                        firstsys, firstseq, lastsys, lastseq);
    return 0;
}

static void show_bfs_stats(BFiles bfs)
{
    int i = 0;
    const char *directory = 0;
    double used_bytes, max_bytes;
    printf("Register:\n");
    while (bfs_register_directory_stat(bfs, i, &directory,
                                       &used_bytes, &max_bytes))
    {
        printf ("%s %10.0lf %10.0lf\n", directory, used_bytes, max_bytes);
        i++;
    }
    i = 0;
    printf("Shadow:\n");
    while (bfs_shadow_directory_stat(bfs, i, &directory,
                                       &used_bytes, &max_bytes))
    {
        printf ("%s %10.0lf %10.0lf\n", directory, used_bytes, max_bytes);
        i++;
    }
}

int zebra_register_statistics (ZebraHandle zh, int dumpdict)
{
    int i, prev;
    int before = 0;
    zint occur;
    int after = 1000000000;
    struct inv_stat_info stat_info;
    char term_dict[2*IT_MAX_WORD+2];

    if (zebra_begin_read (zh))
        return 1;

    show_bfs_stats(zebra_get_bfs(zh));

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
        for (i = 0; isamc_block_used (zh->reg->isamc, i) >= 0; i++)
        {
            fprintf (stdout, " %8" ZINT_FORMAT0 " %8" ZINT_FORMAT0,
                     isamc_block_used (zh->reg->isamc, i),
                     stat_info.no_isam_entries[i]);

            if (stat_info.no_isam_entries[i])
                fprintf(stdout, " %8d   %f",
                        (int) ((1023.0 + (double)
                          isamc_block_used(zh->reg->isamc, i) *
                          isamc_block_size(zh->reg->isamc,i))/1024),
                        ((double) isamc_block_used(zh->reg->isamc, i) *
                          isamc_block_size(zh->reg->isamc,i))/
                        stat_info.no_isam_entries[i]);
            fprintf (stdout, "\n");
        }
    }

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
            fprintf (stdout, "Blocks:    " ZINT_FORMAT "\n", stat_info.isamb_blocks[i]);
            fprintf (stdout, "Size:      " ZINT_FORMAT "\n", stat_info.isamb_sizes[i]);
            fprintf (stdout, "Entries:   " ZINT_FORMAT "\n",
                     stat_info.no_isam_entries[i]);
            fprintf (stdout, "Total      " ZINT_FORMAT "\n", stat_info.isamb_blocks[i]*
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
    fprintf (stdout, "Word pos       " ZINT_FORMAT "\n", occur);
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
    rec_prstat(zh->reg->records, 0);
    xmalloc_trav("unfreed"); /*! while hunting memory leaks */
    zebra_end_read (zh);
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

