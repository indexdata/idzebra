/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zsets.c,v $
 * Revision 1.15  1998-03-05 08:45:14  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.14  1998/02/10 16:39:15  adam
 * Minor change.
 *
 * Revision 1.13  1998/02/10 12:03:06  adam
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

struct zebra_set {
    char *name;
    RSET rset;
    int size;
    struct zset_sort_info *sort_info;
    struct zebra_set *next;
};

struct zset_sort_entry {
    int sysno;
    int score;
    char buf[ZSET_SORT_MAX_LEVEL][SORT_IDX_ENTRYSIZE];
};

struct zset_sort_info {
    int max_entries;
    int num_entries;
    struct zset_sort_entry **entries;
};

void resultSetRank (ZebraHandle zh, struct zset_sort_info *si,
		    RSET rset, int *hits);

ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov,
			  RSET rset, int *hits)
{
    ZebraSet s;
    int i;

    for (s = zh->sets; s; s = s->next)
        if (!strcmp (s->name, name))
        {
	    logf (LOG_DEBUG, "updating result set %s", name);
            if (!ov)
                return NULL;
            rset_delete (s->rset);
            s->rset = rset;
	    resultSetRank (zh, s->sort_info, rset, hits);
            return s;
        }
    logf (LOG_DEBUG, "adding result set %s", name);
    s = xmalloc (sizeof(*s));
    s->next = zh->sets;
    zh->sets = s;
    s->name = xmalloc (strlen(name)+1);
    strcpy (s->name, name);
    s->rset = rset;
    
    s->sort_info = xmalloc (sizeof(*s->sort_info));
    s->sort_info->max_entries = 1000;
    s->sort_info->entries = xmalloc (sizeof(*s->sort_info->entries) *
				     s->sort_info->max_entries);
    for (i = 0; i < s->sort_info->max_entries; i++)
	s->sort_info->entries[i] = xmalloc (sizeof(**s->sort_info->entries));
    resultSetRank (zh, s->sort_info, rset, hits);
    return s;
}

ZebraSet resultSetGet (ZebraHandle zh, const char *name)
{
    ZebraSet s;

    for (s = zh->sets; s; s = s->next)
        if (!strcmp (s->name, name))
            return s;
    return NULL;
}

    
void resultSetDestroy (ZebraHandle zh)
{
    ZebraSet s, s1;

    for (s = zh->sets; s; s = s1)
    {
	int i;
        s1 = s->next;

	for (i = 0; i < s->sort_info->max_entries; i++)
	    xfree (s->sort_info->entries[i]);
	xfree (s->sort_info);

        rset_delete (s->rset);
        xfree (s->name);
        xfree (s);
    }
    zh->sets = NULL;
}

ZebraPosSet zebraPosSetCreate (ZebraHandle zh, const char *name, 
			       int num, int *positions)
{
    ZebraSet sset;
    ZebraPosSet sr;
    RSET rset;
    int i;
    struct zset_sort_info *sort_info;

    if (!(sset = resultSetGet (zh, name)))
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
		sr[i].score = sort_info->entries[position-1]->score;
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
	int term_index;
	RSFD rfd;
	struct it_key key;

	if (sort_info)
	    position = sort_info->num_entries;
	while (num_i < num && positions[num_i] < position)
	    num_i++;
	rfd = rset_open (rset, RSETF_READ);
	while (num_i < num && rset_read (rset, rfd, &key, &term_index))
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
		    sr[num_i].score = -1;
		    num_i++;
		}
	    }
	}
	rset_close (rset, rfd);
    }
    return sr;
}

void zebraPosSetDestroy (ZebraHandle zh, ZebraPosSet records, int num)
{
    xfree (records);
}

struct sortKey {
    int relation;
    int attrUse;
};

void resultSetInsertSort (ZebraHandle zh, ZebraSet sset,
			  struct sortKey *criteria, int num_criteria,
			  int sysno)
{
    struct zset_sort_entry this_entry;
    struct zset_sort_entry *new_entry = NULL;
    struct zset_sort_info *sort_info = sset->sort_info;
    int i, j;

    sortIdx_sysno (zh->sortIdx, sysno);
    for (i = 0; i<num_criteria; i++)
    {
	sortIdx_type (zh->sortIdx, criteria[i].attrUse);
	sortIdx_read (zh->sortIdx, this_entry.buf[i]);
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
	if (!rel)
	    break;
	if (criteria[j].relation == 'D')
        {
	    if (rel > 0)
	        break;
	}
        else if (criteria[j].relation == 'A')
	{
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
    new_entry->score = -1;
}

void resultSetInsertRank (ZebraHandle zh, struct zset_sort_info *sort_info,
			  int sysno, int score, int relation)
{
    struct zset_sort_entry *new_entry = NULL;
    int i, j;

    i = sort_info->num_entries;
    while (--i >= 0)
    {
	int rel = 0;

	rel = score - sort_info->entries[i]->score;

	if (relation == 'D')
	{
	    if (rel >= 0)
		break;
	}
	else if (relation == 'A')
	{
	    if (rel <= 0)
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
    new_entry->sysno = sysno;
    new_entry->score = score;
}

void resultSetSort (ZebraHandle zh, ODR stream,
		    int num_input_setnames, char **input_setnames,
		    char *output_setname, Z_SortKeySpecList *sort_sequence,
		    int *sort_status)
{
    ZebraSet sset;
    RSET rset;
    int i, psysno = 0;
    struct it_key key;
    struct sortKey sort_criteria[3];
    int num_criteria;
    int term_index;
    RSFD rfd;

    if (num_input_setnames == 0)
    {
	zh->errCode = 208;
	return ;
    }
    if (num_input_setnames > 1)
    {
	zh->errCode = 230;
	return;
    }
    sset = resultSetGet (zh, input_setnames[0]);
    if (!sset)
    {
	zh->errCode = 30;
	zh->errString = input_setnames[0];
	return;
    }
    if (!(rset = sset->rset))
    {
	zh->errCode = 30;
	zh->errString = input_setnames[0];
        return;
    }
    num_criteria = sort_sequence->num_specs;
    if (num_criteria > 3)
	num_criteria = 3;
    for (i = 0; i < num_criteria; i++)
    {
	Z_SortKeySpec *sks = sort_sequence->specs[i];
	Z_SortKey *sk;

	if (*sks->sortRelation == Z_SortRelation_ascending)
	    sort_criteria[i].relation = 'A';
	else if (*sks->sortRelation == Z_SortRelation_descending)
	    sort_criteria[i].relation = 'D';
	else
	{
	    zh->errCode = 214;
	    return;
	}
	if (sks->sortElement->which == Z_SortElement_databaseSpecific)
	{
	    zh->errCode = 210;
	    return;
	}
	else if (sks->sortElement->which != Z_SortElement_generic)
	{
	    zh->errCode = 237;
	    return;
	}	
	sk = sks->sortElement->u.generic;
	switch (sk->which)
	{
	case Z_SortKey_sortField:
	    logf (LOG_DEBUG, "Sort: key %d is of type sortField", i+1);
	    zh->errCode = 207;
	    return;
	case Z_SortKey_elementSpec:
	    logf (LOG_DEBUG, "Sort: key %d is of type elementSpec", i+1);
	    zh->errCode = 207;
	    return;
	case Z_SortKey_sortAttributes:
	    logf (LOG_DEBUG, "Sort: key %d is of type sortAttributes", i+1);
	    sort_criteria[i].attrUse =
		zebra_maps_sort (zh->zebra_maps, sk->u.sortAttributes);
	    logf (LOG_DEBUG, "use value = %d", sort_criteria[i].attrUse);
	    if (sort_criteria[i].attrUse == -1)
	    {
		zh->errCode = 116;
		return;
	    }
	    if (sortIdx_type (zh->sortIdx, sort_criteria[i].attrUse))
	    {
		zh->errCode = 207;
		return;
	    }
	    break;
	}
    }
    if (strcmp (output_setname, input_setnames[0]))
    {
	int hits;
	rset = rset_dup (rset);
	sset = resultSetAdd (zh, output_setname, 1, rset, &hits);
    }
    sset->sort_info->num_entries = 0;

    rfd = rset_open (rset, RSETF_READ);
    while (rset_read (rset, rfd, &key, &term_index))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
	    resultSetInsertSort (zh, sset,
				 sort_criteria, num_criteria, psysno);
        }
    }
    rset_close (rset, rfd);

    zh->errCode = 0;
    *sort_status = Z_SortStatus_success;
    
    return;
}

RSET resultSetRef (ZebraHandle zh, Z_ResultSetId *resultSetId)
{
    ZebraSet s;

    if ((s = resultSetGet (zh, resultSetId)))
	return s->rset;
    return NULL;
}

void resultSetRank (ZebraHandle zh, struct zset_sort_info *sort_info,
		    RSET rset, int *hits)
{
    int kno = 0;
    struct it_key key;
    RSFD rfd;
    int term_index, i;
    ZebraRankClass rank_class;
    struct rank_control *rc;

    sort_info->num_entries = 0;
    *hits = 0;
    rfd = rset_open (rset, RSETF_READ);

    logf (LOG_DEBUG, "resultSetRank");
    for (i = 0; i < rset->no_rset_terms; i++)
	logf (LOG_DEBUG, "term=\"%s\" cnt=%d type=%s",
	      rset->rset_terms[i]->name,
	      rset->rset_terms[i]->nn,
	      rset->rset_terms[i]->flags);

    rank_class = zebraRankLookup (zh, "rank-1");
    rc = rank_class->control;

    if (rset_read (rset, rfd, &key, &term_index))
    {
	int psysno = key.sysno;
	int score;
	void *handle =
	    (*rc->begin) (zh, rank_class->class_handle, rset);
	(*hits)++;
	do
	{
	    kno++;
	    if (key.sysno != psysno)
	    {
		score = (*rc->calc) (handle, psysno);

		resultSetInsertRank (zh, sort_info, psysno, score, 'A');
		(*hits)++;
		psysno = key.sysno;
	    }
	    (*rc->add) (handle, key.seqno, term_index);
	}
	while (rset_read (rset, rfd, &key, &term_index));
	score = (*rc->calc) (handle, psysno);
	resultSetInsertRank (zh, sort_info, psysno, score, 'A');
	(*rc->end) (zh, handle);
    }
    rset_close (rset, rfd);
    logf (LOG_DEBUG, "%d keys, %d distinct sysnos", kno, *hits);
}

ZebraRankClass zebraRankLookup (ZebraHandle zh, const char *name)
{
    ZebraRankClass p = zh->rank_classes;
    while (p && strcmp (p->control->name, name))
	p = p->next;
    if (p && !p->init_flag)
    {
	if (p->control->create)
	    p->class_handle = (*p->control->create)(zh);
	p->init_flag = 1;
    }
    return p;
}

void zebraRankInstall (ZebraHandle zh, struct rank_control *ctrl)
{
    ZebraRankClass p = xmalloc (sizeof(*p));
    p->control = xmalloc (sizeof(*p->control));
    memcpy (p->control, ctrl, sizeof(*p->control));
    p->control->name = xstrdup (ctrl->name);
    p->init_flag = 0;
    p->next = zh->rank_classes;
    zh->rank_classes = p;
}

void zebraRankDestroy (ZebraHandle zh)
{
    ZebraRankClass p = zh->rank_classes;
    while (p)
    {
	ZebraRankClass p_next = p->next;
	if (p->init_flag && p->control->destroy)
	    (*p->control->destroy)(zh, p->class_handle);
	xfree (p->control->name);
	xfree (p->control);
	xfree (p);
	p = p_next;
    }
    zh->rank_classes = NULL;
}
