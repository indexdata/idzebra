/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zsets.c,v $
 * Revision 1.13  1998-02-10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.12  1997/09/25 14:57:36  adam
 * Windows NT port.
 *
 * Revision 1.11  1996/12/23 15:30:46  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.10  1995/10/30 15:08:08  adam
 * Bug fixes.
 *
 * Revision 1.9  1995/10/17  18:02:14  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.8  1995/10/10  13:59:25  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.7  1995/10/06  14:38:01  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.6  1995/09/28  09:19:49  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.5  1995/09/27  16:17:32  adam
 * More work on retrieve.
 *
 * Revision 1.4  1995/09/07  13:58:36  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 * Result-set references.
 *
 * Revision 1.3  1995/09/06  16:11:19  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/06  10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.1  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 */
#include <stdio.h>
#include <assert.h>
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

#include "zserver.h"
#include <rstemp.h>

#define SORT_IDX_ENTRYSIZE 64
#define ZSET_SORT_MAX_LEVEL 3

struct zset_sort_entry {
    int sysno;
    char buf[ZSET_SORT_MAX_LEVEL][SORT_IDX_ENTRYSIZE];
};

struct zset_sort_info {
    int max_entries;
    int num_entries;
    struct zset_sort_entry **entries;
};

void resultSetSortReset (struct zset_sort_info **si)
{
    int i;
    if (!*si)
	return ;
    for (i = 0; i<(*si)->num_entries; i++)
	xfree ((*si)->entries[i]);
    xfree ((*si)->entries);
    xfree (*si);
    *si = NULL;
}

ZServerSet *resultSetAdd (ZServerInfo *zi, const char *name, int ov, RSET rset)
{
    ZServerSet *s;

    for (s = zi->sets; s; s = s->next)
        if (!strcmp (s->name, name))
        {
	    logf (LOG_DEBUG, "updating result set %s", name);
            if (!ov)
                return NULL;
	    resultSetSortReset (&s->sort_info);
            rset_delete (s->rset);
            s->rset = rset;
            return s;
        }
    logf (LOG_DEBUG, "adding result set %s", name);
    s = xmalloc (sizeof(*s));
    s->next = zi->sets;
    zi->sets = s;
    s->name = xmalloc (strlen(name)+1);
    strcpy (s->name, name);
    s->rset = rset;
    s->sort_info = NULL;
    return s;
}

ZServerSet *resultSetGet (ZServerInfo *zi, const char *name)
{
    ZServerSet *s;

    for (s = zi->sets; s; s = s->next)
        if (!strcmp (s->name, name))
            return s;
    return NULL;
}

    
void resultSetDestroy (ZServerInfo *zi)
{
    ZServerSet *s, *s1;

    for (s = zi->sets; s; s = s1)
    {
        s1 = s->next;
	resultSetSortReset (&s->sort_info);
        rset_delete (s->rset);
        xfree (s->name);
        xfree (s);
    }
    zi->sets = NULL;
}

ZServerSetSysno *resultSetSysnoGet (ZServerInfo *zi, const char *name, 
                                    int num, int *positions)
{
    ZServerSet *sset;
    ZServerSetSysno *sr;
    RSET rset;
    int i;
    struct zset_sort_info *sort_info;

    if (!(sset = resultSetGet (zi, name)))
        return NULL;
    if (!(rset = sset->rset))
        return NULL;
    sr = xmalloc (sizeof(*sr) * num);
    for (i = 0; i<num; i++)
    {
	sr[i].sysno = 0;
	sr[i].score = -1;
    }
    sort_info = sset->sort_info;
    if (sort_info)
    {
	int position;

	for (i = 0; i<num; i++)
	{
	    position = positions[i];
	    if (position <= sort_info->num_entries)
	    {
		logf (LOG_DEBUG, "got pos=%d (sorted)", position);
		sr[i].sysno = sort_info->entries[position-1]->sysno;
	    }
	}
    }
    /* did we really get all entries using sort ? */
    for (i = 0; i<num; i++)
    {
	if (!sr[i].sysno)
	    break;
    }
    if (i < num) /* nope, get the rest, unsorted - sorry */
    {
	int position = 0;
	int num_i = 0;
	int psysno = 0;
	RSFD rfd;
	struct it_key key;

	if (sort_info)
	    position = sort_info->num_entries;
	while (num_i < num && positions[num_i] < position)
	    num_i++;
	rfd = rset_open (rset, RSETF_READ|RSETF_SORT_RANK);
	while (num_i < num && rset_read (rset, rfd, &key))
	{
	    if (key.sysno != psysno)
	    {
		psysno = key.sysno;
		if (sort_info)
		{
		    /* determine we alreay have this in our set */
		    for (i = sort_info->num_entries; --i >= 0; )
			if (psysno == sort_info->entries[i]->sysno)
			    break;
		    if (i >= 0)
			continue;
		}
		position++;
		assert (num_i < num);
		if (position == positions[num_i])
		{
		    sr[num_i].sysno = psysno;
		    logf (LOG_DEBUG, "got pos=%d (unsorted)", position);
		    rset_score (rset, rfd, &sr[num_i].score);
		    num_i++;
		}
	    }
	}
	rset_close (rset, rfd);
    }
    return sr;
}

void resultSetSysnoDel (ZServerInfo *zi, ZServerSetSysno *records, int num)
{
    xfree (records);
}

struct sortKey {
    int relation;
    int attrUse;
};

void resultSetInsertSort (ZServerInfo *zi, ZServerSet *sset,
			  struct sortKey *criteria, int num_criteria,
			  int sysno)
{
    struct zset_sort_entry this_entry;
    struct zset_sort_entry *new_entry = NULL;
    struct zset_sort_info *sort_info = sset->sort_info;
    int i, j;

    sortIdx_sysno (zi->sortIdx, sysno);
    for (i = 0; i<num_criteria; i++)
    {
	sortIdx_type (zi->sortIdx, criteria[i].attrUse);
	sortIdx_read (zi->sortIdx, this_entry.buf[i]);
    }
    i = sort_info->num_entries;
    while (--i >= 0)
    {
	int rel = 0;
	for (j = 0; j<num_criteria; j++)
	{
	    rel = memcmp (this_entry.buf[j], sort_info->entries[i]->buf[j],
			  SORT_IDX_ENTRYSIZE);
	    if (rel)
		break;
	}	
	if (rel)
	{
	    if (criteria[j].relation == 'D')
		if (rel > 0)
		    break;
	    if (criteria[j].relation == 'A')
		if (rel < 0)
		    break;
	}
    }
    j = sort_info->max_entries-1;
    if (i == j)
	return;
    ++i;
    new_entry = sort_info->entries[j];
    while (j != i)
    {
	sort_info->entries[j] = sort_info->entries[j-1];
	--j;
    }
    sort_info->entries[j] = new_entry;
    assert (new_entry);
    if (sort_info->num_entries != sort_info->max_entries)
	(sort_info->num_entries)++;
    for (i = 0; i<num_criteria; i++)
	memcpy (new_entry->buf[i], this_entry.buf[i], SORT_IDX_ENTRYSIZE);
    new_entry->sysno = sysno;
}
	
int resultSetSort (ZServerInfo *zi, bend_sort_rr *rr)
{
    ZServerSet *sset;
    RSET rset;
    int i, psysno = 0;
    struct it_key key;
    struct sortKey sort_criteria[3];
    int num_criteria;
    RSFD rfd;

    if (rr->num_input_setnames == 0)
    {
	rr->errcode = 208;
	return 0;
    }
    if (rr->num_input_setnames > 1)
    {
	rr->errcode = 230;
	return 0;
    }
    sset = resultSetGet (zi, rr->input_setnames[0]);
    if (!sset)
    {
	rr->errcode = 30;
	rr->errstring =  rr->input_setnames[0];
	return 0;
    }
    if (!(rset = sset->rset))
    {
	rr->errcode = 30;
	rr->errstring =  rr->input_setnames[0];
        return 0;
    }
    num_criteria = rr->sort_sequence->num_specs;
    if (num_criteria > 3)
	num_criteria = 3;
    for (i = 0; i < num_criteria; i++)
    {
	Z_SortKeySpec *sks = rr->sort_sequence->specs[i];
	Z_SortKey *sk;

	if (*sks->sortRelation == Z_SortRelation_ascending)
	    sort_criteria[i].relation = 'A';
	else if (*sks->sortRelation == Z_SortRelation_descending)
	    sort_criteria[i].relation = 'D';
	else
	{
	    rr->errcode = 214;
	    return 0;
	}
	if (sks->sortElement->which == Z_SortElement_databaseSpecific)
	{
	    rr->errcode = 210;
	    return 0;
	}
	else if (sks->sortElement->which != Z_SortElement_generic)
	{
	    rr->errcode = 237;
	    return 0;
	}	
	sk = sks->sortElement->u.generic;
	switch (sk->which)
	{
	case Z_SortKey_sortField:
	    logf (LOG_DEBUG, "Sort: key %d is of type sortField", i+1);
	    rr->errcode = 207;
	    return 0;
	case Z_SortKey_elementSpec:
	    logf (LOG_DEBUG, "Sort: key %d is of type elementSpec", i+1);
	    return 0;
	case Z_SortKey_sortAttributes:
	    logf (LOG_DEBUG, "Sort: key %d is of type sortAttributes", i+1);
	    sort_criteria[i].attrUse =
		zebra_maps_sort (zi->zebra_maps, sk->u.sortAttributes);
	    logf (LOG_DEBUG, "use value = %d", sort_criteria[i].attrUse);
	    if (sort_criteria[i].attrUse == -1)
	    {
		rr->errcode = 116;
		return 0;
	    }
	    if (sortIdx_type (zi->sortIdx, sort_criteria[i].attrUse))
	    {
		rr->errcode = 207;
		return 0;
	    }
	    break;
	}
    }
    if (strcmp (rr->output_setname, rr->input_setnames[0]))
    {
	rset = rset_dup (rset);
	sset = resultSetAdd (zi, rr->output_setname, 1, rset);
    }
    resultSetSortReset (&sset->sort_info);

    sset->sort_info = xmalloc (sizeof(*sset->sort_info));
    sset->sort_info->max_entries = 10;
    sset->sort_info->num_entries = 0;
    sset->sort_info->entries =	xmalloc (sizeof(*sset->sort_info->entries) * 
					 sset->sort_info->max_entries);
    for (i = 0; i<sset->sort_info->max_entries; i++)
	sset->sort_info->entries[i] =
	    xmalloc (sizeof(*sset->sort_info->entries[i]));


    rfd = rset_open (rset, RSETF_READ|RSETF_SORT_SYSNO);
    while (rset_read (rset, rfd, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
	    resultSetInsertSort (zi, sset,
				 sort_criteria, num_criteria, psysno);
        }
    }
    rset_close (rset, rfd);

    rr->errcode = 0;
    rr->sort_status = Z_SortStatus_success;
    
    return 0;
}

