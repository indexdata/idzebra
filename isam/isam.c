/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isam.c,v $
 * Revision 1.3  1994-09-26 17:06:35  quinn
 * Back again...
 *
 * Revision 1.1  1994/09/12  08:02:13  quinn
 * Not functional yet
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <util.h>
#include <bfile.h>
#include <isam.h>
#include <common.h>
#include "isutil.h"
#include "rootblk.h"
#include <ismemory.h>
#include "physical.h"
#include "keyops.h"

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
	    log(LOG_WARN, "Ignoring extra args to is resource");
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
ISAM is_open(const char *name, int writeflag)
{
    ISAM new;
    char *nm, *r, *pp[IS_MAX_BLOCKTYPES+1], m[2];
    int num, size, rs, tmp, i;
    is_type_header th;

    log(LOG_DEBUG, "is_open(%s, %s)", name, writeflag ? "RW" : "RDONLY");
    new = xmalloc(sizeof(*new));
    new->writeflag = writeflag;
    for (i = 0; i < IS_MAX_BLOCKTYPES; i++)
	new->types[i].index = 0;                        /* dummy */

    /* determine number and size of blocktypes */
    if (!(r = res_get(common_resource, nm = strconcat(name, ".",
	"blocktypes", 0))) || !(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    new->num_types = num;
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d%1[bBkKmM]", &size, m)) < 1)
    	{
	    log(LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
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
		    log(LOG_FATAL, "Illegal size suffix: %c", *m);
		    return 0;
	}
	new->types[i].dbuf = xmalloc(new->types[i].blocksize);
	m[0] = 'A' + i;
	m[1] = '\0';
	if (!(new->types[i].bf = bf_open(strconcat(name, m, 0), 
	    new->types[i].blocksize, writeflag)))
	{
	    log(LOG_FATAL, "bf_open failed");
	    return 0;
	}
	if ((rs = is_rb_read(&new->types[i], &th)) > 0)
	{
	    if (th.blocksize != new->types[i].blocksize)
	    {
	    	log(LOG_FATAL, "File blocksize mismatch in %s", name);
	    	exit(1);
	    }
	    new->types[i].freelist = th.freelist;
	    new->types[i].top = th.top;
	}
	else if (writeflag) /* write dummy superblock to determine top */
	{
	    if ((rs = is_rb_write(&new->types[i], &th)) <=0)  /* dummy */
	    {
	    	log(LOG_FATAL, "Failed to write initial superblock.");
	    	exit(1);
	    }
	    new->types[i].freelist = -1;
	    new->types[i].top = rs;
	}
	/* ELSE: this is an empty file opened in read-only mode. */
    }
    if (!(r = res_get_def(common_resource, nm = strconcat(name, ".", "keysize",
    	0), "4")))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if ((new->keysize = atoi(r)) <= 0)
    {
    	log(LOG_FATAL, "Must specify positive keysize.");
    	return 0;
    }

    /* determine repack percent */
    if (!(r = res_get_def(common_resource, nm = strconcat(name, ".", "repack",
    	0), IS_DEF_REPACK_PERCENT)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    new->repack = atoi(r);

    /* determine max keys/blocksize */
    if (!(r = res_get(common_resource, nm = strconcat(name, ".",
	"maxkeys", 0))) || !(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if (num < new->num_types -1)
    {
    	log(LOG_FATAL, "Not enough elements in %s", nm);
    	return 0;
    }
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d", &tmp)) < 1)
    	{
	    log(LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
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
	    log(LOG_FATAL, "Blocksize too small in %s", name);
	    exit(1);
	}
    }

    /* determine nice fill rates */
    if (!(r = res_get(common_resource, nm = strconcat(name, ".",
	"nicefill", 0))) || !(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if (num < new->num_types)
    {
    	log(LOG_FATAL, "Not enough elements in %s", nm);
    	return 0;
    }
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d", &tmp)) < 1)
    	{
	    log(LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	new->types[i].nice_keys_block = (new->types[i].max_keys_block0 * tmp) /
	    100;
	if (new->types[i].nice_keys_block < 1)
		new->types[i].nice_keys_block = 1;
    }

    new->cmp = is_default_cmp;
    return new;
}

/*
 * Close isam file.
 */
int is_close(ISAM is)
{
    int i;
    is_type_header th;

    log(LOG_DEBUG, "is_close()");
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
		    log(LOG_FATAL, "Failed to write headerblock");
		    exit(1);
		}
	    }
	    bf_close(is->types[i].bf);
	}
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

ISAM_P is_merge(ISAM is, ISAM_P pos, int num, const char *data)
{
    is_mtable tab;
    int res;
    char keybuf[IS_MAX_RECORD];
    int oldnum, oldtype;
    char operation, *record;

    is_m_establish_tab(is, &tab, pos);
    /* TODO: do something to aquire oldnum at this point */
    if (pos)
    	if (is_m_read_full(&tab, tab.data) < 0)
    	{
	    log(LOG_FATAL, "read_full failed");
	    exit(1);
	}
    oldnum = tab.num_records;
    oldtype = tab.pos_type;
    while (num)
    {
    	operation = *(data)++;
    	record = (char*)data;
    	data += is_keysize(is);
    	num--;
	while (num && !memcmp(record, data, is_keysize(tab.is) + 1))
	{
	    data += 1 + is_keysize(is);
	    num--;
	}
	if ((res = is_m_seek_record(&tab, record)) > 0)  /* no match */
	{
	    if (operation == KEYOP_INSERT)
	    {
	        log(LOG_DEBUG, "XXInserting new record.");
		is_m_write_record(&tab, record);
	    }
	    else
	    	log(LOG_DEBUG, "XXDeletion failed to find match.");
	}
	else /* match found */
	{
	    if (operation == KEYOP_INSERT)
	    {
	        log(LOG_DEBUG, "XXSkipping insertion - match found.");
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
		        log(LOG_DEBUG, "XXNoop delete. skipping.");
		    	data += 1 + is_keysize(is);
		    	num--;
		    	continue;
		    }
		    /* else check if next key can fit in this position */
		    is_m_peek_record(&tab, keybuf);
		    res = (*is->cmp)(data + 1, keybuf);
		    if (res < 0)
		    {
		        log(LOG_DEBUG, "XXReplacing record.");
		    	is_m_replace_record(&tab, data + 1);
		    	data += 1 + is_keysize(is);
		    	num--;
			continue;
		    }
		}
		log(LOG_DEBUG, "Deleting record.");
		is_m_delete_record(&tab);
	    }
	}
    }
    while (tab.pos_type < tab.is->num_types - 1 && tab.num_records >
	tab.is->types[tab.pos_type].max_keys)
	    tab.pos_type++;
    if (!oldnum || tab.pos_type != oldtype || (abs(oldnum - tab.num_records) *
	100) / oldnum > tab.is->repack)
    	is_p_remap(&tab);
    else
    	is_p_align(&tab);
    is_p_sync(&tab);
    return is_address(tab.pos_type, tab.data->diskpos);
}

/*
 * Locate a table of keys in an isam file. The ISPT is an individual
 * position marker for that table.
 */
ISPT is_position(ISAM is, ISAM_P pos);

/*
 * Release ISPT.
 */
void is_pt_free(ISPT ip);
