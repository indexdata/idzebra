/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isam.c,v $
 * Revision 1.21  1996-03-29 14:11:47  quinn
 * Change to is_merge
 *
 * Revision 1.20  1996/03/19  13:14:57  quinn
 * Moved an xfree()
 *
 * Revision 1.19  1996/02/10  12:20:56  quinn
 * *** empty log message ***
 *
 * Revision 1.18  1996/02/06  10:19:56  quinn
 * Attempt at fixing bug. Not all blocks were read before they were unlinked
 * prior to a remap operation.
 *
 * Revision 1.17  1995/12/06  15:48:44  quinn
 * Fixed update-problem.
 *
 * Revision 1.16  1995/12/06  14:48:26  quinn
 * Fixed some strange bugs.
 *
 * Revision 1.15  1995/12/06  09:59:45  quinn
 * Fixed memory-consumption bug in memory.c
 * Added more blocksizes to the default ISAM configuration.
 *
 * Revision 1.14  1995/11/24  17:26:19  quinn
 * Mostly about making some ISAM stuff in the config file optional.
 *
 * Revision 1.13  1995/10/17  18:03:15  adam
 * Commented out qsort in is_merge.
 *
 * Revision 1.12  1995/09/06  16:11:41  adam
 * Keysize parameter to is_open (if non-zero).
 *
 * Revision 1.11  1995/09/04  12:33:46  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.10  1994/09/28  16:58:32  quinn
 * Small mod.
 *
 * Revision 1.9  1994/09/28  12:56:15  quinn
 * Added access functions (ISPT)
 *
 * Revision 1.8  1994/09/28  12:32:17  quinn
 * Trivial
 *
 * Revision 1.7  1994/09/28  11:56:25  quinn
 * Added sort of input to is_merge
 *
 * Revision 1.6  1994/09/28  11:29:33  quinn
 * Added cmp parameter.
 *
 * Revision 1.5  1994/09/27  20:03:50  quinn
 * Seems relatively bug-free.
 *
 * Revision 1.4  1994/09/26  17:11:29  quinn
 * Trivial
 *
 * Revision 1.3  1994/09/26  17:06:35  quinn
 * Back again...
 *
 * Revision 1.1  1994/09/12  08:02:13  quinn
 * Not functional yet
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <alexutil.h>
#include <bfile.h>
#include <isam.h>
#include <common.h>
#include "isutil.h"
#include "rootblk.h"
#include "keyops.h"

static ispt_struct *ispt_freelist = 0;

static struct
{
    int total_merge_operations;
    int total_items;
    int dub_items_removed;
    int new_items;
    int failed_deletes;
    int skipped_inserts;
    int delete_insert_noop;
    int delete_replace;
    int delete;
    int remaps;
    int block_jumps;
    int tab_deletes;
    int new_tables;
} statistics;

static ISPT ispt_alloc()
{
    ISPT p;

    if (ispt_freelist)
    {
    	p = ispt_freelist;
    	ispt_freelist = ispt_freelist->next;
    }
    else
    	p = xmalloc(sizeof(ispt_struct));
    return p;
}

static void ispt_free(ISPT pt)
{
    pt->next = ispt_freelist;
    ispt_freelist = pt;
}

static int splitargs(const char *s, char *bf[], int max)
{
    int ct = 0;
    for (;;)
    {
    	while (*s && isspace(*s))
	    s++;
    	bf[ct] = (char *) s;
	if (!*s)
		return ct;
	ct++;
	if (ct > max)
	{
	    logf (LOG_WARN, "Ignoring extra args to is resource");
	    bf[ct] = '\0';
	    return(ct - 1);
	}
	while (*s && !isspace(*s))
	    s++;
    }
}

/*
 * Open isam file.
 * Process resources.
 */
ISAM is_open(const char *name, int (*cmp)(const void *p1, const void *p2),
    int writeflag, int keysize)
{
    ISAM new;
    char *nm, *r, *pp[IS_MAX_BLOCKTYPES+1], m[2];
    int num, size, rs, tmp, i;
    is_type_header th;

    logf (LOG_DEBUG, "is_open(%s, %s)", name, writeflag ? "RW" : "RDONLY");
    if (writeflag)
    {
	statistics.total_merge_operations = 0;
	statistics.total_items = 0;
	statistics.dub_items_removed = 0;
	statistics.new_items = 0;
	statistics.failed_deletes = 0;
	statistics.skipped_inserts = 0;
	statistics.delete_insert_noop = 0;
	statistics.delete_replace = 0;
	statistics.delete = 0;
	statistics.remaps = 0;
	statistics.new_tables = 0;
	statistics.block_jumps = 0;
	statistics.tab_deletes = 0;
    }

    new = xmalloc(sizeof(*new));
    new->writeflag = writeflag;
    for (i = 0; i < IS_MAX_BLOCKTYPES; i++)
	new->types[i].index = 0;                        /* dummy */

    /* determine number and size of blocktypes */
    if (!(r = res_get_def(common_resource, nm = strconcat(name, ".",
	"blocktypes", 0), "64 512 4K 32K")) ||
	!(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	logf (LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    new->num_types = num;
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d%1[bBkKmM]", &size, m)) < 1)
    	{
	    logf (LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	if (rs == 1)
		*m = 'b';
	switch (*m)
	{
		case 'b': case 'B':
		    new->types[i].blocksize = size; break;
		case 'k': case 'K':
		    new->types[i].blocksize = size * 1024; break;
		case 'm': case 'M':
		    new->types[i].blocksize = size * 1048576; break;
		default:
		    logf (LOG_FATAL, "Illegal size suffix: %c", *m);
		    return 0;
	}
	new->types[i].dbuf = xmalloc(new->types[i].blocksize);
	m[0] = 'A' + i;
	m[1] = '\0';
	if (!(new->types[i].bf = bf_open(strconcat(name, m, 0), 
	    new->types[i].blocksize, writeflag)))
	{
	    logf (LOG_FATAL, "bf_open failed");
	    return 0;
	}
	if ((rs = is_rb_read(&new->types[i], &th)) > 0)
	{
	    if (th.blocksize != new->types[i].blocksize)
	    {
	    	logf (LOG_FATAL, "File blocksize mismatch in %s", name);
	    	exit(1);
	    }
	    new->types[i].freelist = th.freelist;
	    new->types[i].top = th.top;
	}
	else if (writeflag) /* write dummy superblock to determine top */
	{
	    if ((rs = is_rb_write(&new->types[i], &th)) <=0)  /* dummy */
	    {
	    	logf (LOG_FATAL, "Failed to write initial superblock.");
	    	exit(1);
	    }
	    new->types[i].freelist = -1;
	    new->types[i].top = rs;
	}
	/* ELSE: this is an empty file opened in read-only mode. */
    }
    if (keysize > 0)
        new->keysize = keysize;
    else
    {
        if (!(r = res_get_def(common_resource, nm = strconcat(name, ".",
                                                              "keysize",
                                                              0), "4")))
        {
            logf (LOG_FATAL, "Failed to locate resource %s", nm);
            return 0;
        }
        if ((new->keysize = atoi(r)) <= 0)
        {
            logf (LOG_FATAL, "Must specify positive keysize.");
            return 0;
        }
    }

    /* determine repack percent */
    if (!(r = res_get_def(common_resource, nm = strconcat(name, ".", "repack",
    	0), IS_DEF_REPACK_PERCENT)))
    {
    	logf (LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    new->repack = atoi(r);

    /* determine max keys/blocksize */
    if (!(r = res_get_def(common_resource, nm = strconcat(name, ".",
	"maxkeys", 0), "50 640 10000")) || !(num = splitargs(r, pp,
	IS_MAX_BLOCKTYPES)))
    {
    	logf (LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if (num < new->num_types -1)
    {
    	logf (LOG_FATAL, "Not enough elements in %s", nm);
    	return 0;
    }
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d", &tmp)) < 1)
    	{
	    logf (LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	new->types[i].max_keys = tmp;
    }

    /* determine max keys/block */
    for (i = 0; i < new->num_types; i++)
    {
    	if (!new->types[i].index)
    	{
	    new->types[i].max_keys_block = (new->types[i].blocksize - 2 *
		sizeof(int)) / new->keysize;
	    new->types[i].max_keys_block0 = (new->types[i].blocksize - 3 *
		sizeof(int)) / new->keysize;
	}
	else
	    new->types[i].max_keys_block = new->types[i].max_keys_block0 /
		new->keysize;
	if (new->types[i].max_keys_block0 < 1)
	{
	    logf (LOG_FATAL, "Blocksize too small in %s", name);
	    exit(1);
	}
    }

    /* determine nice fill rates */
    if (!(r = res_get_def(common_resource, nm = strconcat(name, ".",
	"nicefill", 0), "90 90 90 95")) || !(num = splitargs(r, pp,
	IS_MAX_BLOCKTYPES)))
    {
    	logf (LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if (num < new->num_types)
    {
    	logf (LOG_FATAL, "Not enough elements in %s", nm);
    	return 0;
    }
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d", &tmp)) < 1)
    	{
	    logf (LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	new->types[i].nice_keys_block = (new->types[i].max_keys_block0 * tmp) /
	    100;
	if (new->types[i].nice_keys_block < 1)
		new->types[i].nice_keys_block = 1;
    }

    new->cmp = cmp ? cmp : is_default_cmp;
    return new;
}

/*
 * Close isam file.
 */
int is_close(ISAM is)
{
    int i;
    is_type_header th;

    logf (LOG_DEBUG, "is_close()");
    for (i = 0; i < is->num_types; i++)
    {
    	if (is->types[i].bf)
    	{
	    if (is->writeflag)
	    {
		th.blocksize = is->types[i].blocksize;
		th.keysize = is->keysize;
		th.freelist = is->types[i].freelist;
		th.top = is->types[i].top;
		if (is_rb_write(&is->types[i], &th) < 0)
		{
		    logf (LOG_FATAL, "Failed to write headerblock");
		    exit(1);
		}
	    }
	    bf_close(is->types[i].bf);
	}
    }
    if (is->writeflag)
    {
	logf(LOG_LOG, "ISAM statistics:");
	logf(LOG_LOG, "total_merge_operations      %d",
	    statistics.total_merge_operations);
	logf(LOG_LOG, "total_items                 %d", statistics.total_items);
	logf(LOG_LOG, "dub_items_removed           %d",
	    statistics.dub_items_removed);
	logf(LOG_LOG, "new_items                   %d", statistics.new_items);
	logf(LOG_LOG, "failed_deletes              %d",
	    statistics.failed_deletes);
	logf(LOG_LOG, "skipped_inserts             %d",
	    statistics.skipped_inserts);
	logf(LOG_LOG, "delete_insert_noop          %d",
	    statistics.delete_insert_noop);
	logf(LOG_LOG, "delete_replace              %d",
	    statistics.delete_replace);
	logf(LOG_LOG, "delete                      %d", statistics.delete);
	logf(LOG_LOG, "remaps                      %d", statistics.remaps);
	logf(LOG_LOG, "block_jumps                 %d", statistics.block_jumps);
	logf(LOG_LOG, "tab_deletes                 %d", statistics.tab_deletes);
    }
    xfree(is);
    return 0;
}

static ISAM_P is_address(int type, int pos)
{
    ISAM_P r;

    r = pos << 2;
    r |= type;
    return r;
}

ISAM_P is_merge(ISAM is, ISAM_P pos, int num, char *data)
{
    is_mtable tab;
    int res;
    char keybuf[IS_MAX_RECORD];
    int oldnum, oldtype, i;
    char operation, *record;

    statistics.total_merge_operations++;
    statistics.total_items += num;
    if (!pos)
	statistics.new_tables++;

    is_m_establish_tab(is, &tab, pos);
    if (pos)
    	if (is_m_read_full(&tab, tab.data) < 0)
    	{
	    logf (LOG_FATAL, "read_full failed");
	    exit(1);
	}
    oldnum = tab.num_records;
    oldtype = tab.pos_type;
    while (num)
    {
    	operation = *(data)++;
    	record = (char*) data;
    	data += is_keysize(is);
    	num--;
	while (num && !memcmp(record - 1, data, is_keysize(tab.is) + 1))
	{
	    data += 1 + is_keysize(is);
	    num--;
	    statistics.dub_items_removed++;
	}
	if ((res = is_m_seek_record(&tab, record)) > 0)  /* no match */
	{
	    if (operation == KEYOP_INSERT)
	    {
	        logf (LOG_DEBUG, "XXInserting new record.");
		is_m_write_record(&tab, record);
		statistics.new_items++;
	    }
	    else
	    {
	    	logf (LOG_DEBUG, "XXDeletion failed to find match.");
		statistics.failed_deletes++;
	    }
	}
	else /* match found */
	{
	    if (operation == KEYOP_INSERT)
	    {
	        logf (LOG_DEBUG, "XXSkipping insertion - match found.");
		statistics.skipped_inserts++;
	    	continue;
	    }
	    else if (operation == KEYOP_DELETE)
	    {
	    	/* try to avoid needlessly moving data */
	        if (num && *(data) == KEYOP_INSERT)
	        {
		    /* next key is identical insert? - NOOP - skip it */
		    if (!memcmp(record, data + 1, is_keysize(is)))
		    {
		        logf (LOG_DEBUG, "XXNoop delete. skipping.");
		    	data += 1 + is_keysize(is);
		    	num--;
			while (num && !memcmp(data, data + is_keysize(tab.is) +
			    1, is_keysize(tab.is) + 1))
			{
			    data += 1 + is_keysize(is);
			    num--;
			    statistics.dub_items_removed++;
			}
			statistics.delete_insert_noop++;
		    	continue;
		    }
		    /* else check if next key can fit in this position */
		    if (is_m_peek_record(&tab, keybuf) &&
			(*is->cmp)(data + 1, keybuf) < 0)
		    {
		        logf (LOG_DEBUG, "XXReplacing record.");
		    	is_m_replace_record(&tab, data + 1);
		    	data += 1 + is_keysize(is);
		    	num--;
			while (num && !memcmp(data, data + is_keysize(tab.is) +
			    1, is_keysize(tab.is) + 1))
			{
			    data += 1 + is_keysize(is);
			    num--;
			    statistics.dub_items_removed++;
			}
			statistics.delete_replace++;
			continue;
		    }
		}
		logf (LOG_DEBUG, "Deleting record.");
		is_m_delete_record(&tab);
		statistics.delete++;
	    }
	}
    }
    i = tab.pos_type;
    while (i < tab.is->num_types - 1 && tab.num_records >
	tab.is->types[i].max_keys)
	i++;
    if (i != tab.pos_type)
    {
	/* read remaining blocks */
	for (; tab.cur_mblock; tab.cur_mblock = tab.cur_mblock->next)
	    if (tab.cur_mblock->state < IS_MBSTATE_CLEAN)
		is_m_read_full(&tab, tab.cur_mblock);
    	is_p_unmap(&tab);
	tab.pos_type = i;
	if (pos)
	    statistics.block_jumps++;
    }
    if (!oldnum || tab.pos_type != oldtype || (abs(oldnum - tab.num_records) *
	100) / oldnum > tab.is->repack)
    {
    	is_p_remap(&tab);
	statistics.remaps++;
    }
    else
    	is_p_align(&tab);
    if (tab.data)
    {
	is_p_sync(&tab);
	pos = is_address(tab.pos_type, tab.data->diskpos);
    }
    else
    {
    	pos = 0;
	statistics.tab_deletes++;
    }
    is_m_release_tab(&tab);
    return pos;
}

/*
 * Locate a table of keys in an isam file. The ISPT is an individual
 * position marker for that table.
 */
ISPT is_position(ISAM is, ISAM_P pos)
{
    ispt_struct *p;

    p = ispt_alloc();
    is_m_establish_tab(is, &p->tab, pos);
    return p;
}

/*
 * Release ISPT.
 */
void is_pt_free(ISPT ip)
{
    is_m_release_tab(&ip->tab);
    ispt_free(ip);
}

/*
 * Read a key from a table.
 */
int is_readkey(ISPT ip, void *buf)
{
    return is_m_read_record(&ip->tab, buf, 0);
}    

int is_numkeys(ISPT ip)
{
    return is_m_num_records(&ip->tab);
}

void is_rewind(ISPT ip)
{
    is_m_rewind(&ip->tab);
}    
