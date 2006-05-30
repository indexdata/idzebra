/* $Id: zsets.c,v 1.49.2.6 2006-05-30 21:43:15 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004,2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/


#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "index.h"
#include <rstemp.h>

#define SORT_IDX_ENTRYSIZE 64
#define ZSET_SORT_MAX_LEVEL 3

struct zebra_set_term_entry {
    int reg_type;
    char *db;
    int set;
    int use;
    char *term;
};
struct zebra_set {
    char *name;
    RSET rset;
    NMEM nmem;
    int hits;
    int num_bases;
    char **basenames;
    Z_RPNQuery *rpn;
    Z_SortKeySpecList *sortSpec;
    struct zset_sort_info *sort_info;
    struct zebra_set_term_entry *term_entries;
    int term_entries_max;
    struct zebra_set *next;
    int locked;

    int cache_position;  /* last position */
    RSFD cache_rfd;      /* rfd (NULL if not existing) */
    int cache_psysno;
};

struct zset_sort_entry {
    int sysno;
    int score;
    char buf[ZSET_SORT_MAX_LEVEL][SORT_IDX_ENTRYSIZE];
};

struct zset_sort_info {
    int max_entries;
    int num_entries;
    struct zset_sort_entry *all_entries;
    struct zset_sort_entry **entries;
};

ZebraSet resultSetAddRPN (ZebraHandle zh, NMEM m,
			  Z_RPNQuery *rpn, int num_bases,
                          char **basenames, 
			  const char *setname)
{
    ZebraSet zebraSet;
    int i;

    zh->errCode = 0;
    zh->errString = NULL;
    zh->hits = 0;

    zebraSet = resultSetAdd (zh, setname, 1);
    if (!zebraSet)
	return 0;
    zebraSet->locked = 1;
    zebraSet->rpn = 0;
    zebraSet->nmem = m;
    
    zebraSet->num_bases = num_bases;
    zebraSet->basenames = 
        nmem_malloc (zebraSet->nmem, num_bases * sizeof(*zebraSet->basenames));
    for (i = 0; i<num_bases; i++)
        zebraSet->basenames[i] = nmem_strdup (zebraSet->nmem, basenames[i]);


    zebraSet->rset = rpn_search (zh, zebraSet->nmem, rpn,
                                 zebraSet->num_bases,
		                 zebraSet->basenames, zebraSet->name,
				 zebraSet);
    zh->hits = zebraSet->hits;
    if (zebraSet->rset)
        zebraSet->rpn = rpn;
    zebraSet->locked = 0;
    return zebraSet;
}

void resultSetAddTerm (ZebraHandle zh, ZebraSet s, int reg_type,
		       const char *db, int set,
		       int use, const char *term)
{
    if (!s->nmem)
	s->nmem = nmem_create ();
    if (!s->term_entries)
    {
	int i;
	s->term_entries_max = 1000;
	s->term_entries =
	    nmem_malloc (s->nmem, s->term_entries_max * 
			 sizeof(*s->term_entries));
	for (i = 0; i < s->term_entries_max; i++)
	    s->term_entries[i].term = 0;
    }
    if (s->hits < s->term_entries_max)
    {
	s->term_entries[s->hits].reg_type = reg_type;
	s->term_entries[s->hits].db = nmem_strdup (s->nmem, db);
	s->term_entries[s->hits].set = set;
	s->term_entries[s->hits].use = use;
	s->term_entries[s->hits].term = nmem_strdup (s->nmem, term);
    }
    (s->hits)++;
}


int zebra_resultSetTerms (ZebraHandle zh, const char *setname, 
                          int no, int *count, 
                          int *type, char *out, size_t *len)
{
    ZebraSet s = resultSetGet (zh, setname);
    int no_max = 0;

    if (count)
        *count = 0;
    if (!s || !s->rset)
        return 0;
    no_max = s->rset->no_rset_terms;
    if (no < 0 || no >= no_max)
        return 0;
    if (count)
        *count = s->rset->rset_terms[no]->count;
    if (type)
        *type = s->rset->rset_terms[no]->type;
    
    if (out)
    {
        char *inbuf = s->rset->rset_terms[no]->name;
        size_t inleft = strlen(inbuf);
        size_t outleft = *len - 1;
	int converted = 0;

        if (zh->iconv_from_utf8 != 0)
        {
            char *outbuf = out;
            size_t ret;
            
            ret = yaz_iconv(zh->iconv_from_utf8, &inbuf, &inleft,
                        &outbuf, &outleft);
            if (ret == (size_t)(-1))
                *len = 0;
            else
                *len = outbuf - out;
	    converted = 1;
        }
        if (!converted)
        {
            if (inleft > outleft)
                inleft = outleft;
            *len = inleft;
            memcpy (out, inbuf, *len);
        }
        out[*len] = 0;
    }
    return no_max;
}


ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov)
{
    ZebraSet s;
    int i;

    for (s = zh->sets; s; s = s->next)
        if (!strcmp (s->name, name))
	    break;
    if (s)
    {
	yaz_log (LOG_DEBUG, "updating result set %s", name);
	if (!ov || s->locked)
	    return NULL;
	if (s->rset)
	{
	    if (s->cache_rfd)
		rset_close(s->rset, s->cache_rfd);
	    rset_delete (s->rset);
	}
	if (s->nmem)
	    nmem_destroy (s->nmem);
    }
    else
    {
	const char *sort_max_str = zebra_get_resource(zh, "sortmax", "1000");

	yaz_log (LOG_DEBUG, "adding result set %s", name);
	s = (ZebraSet) xmalloc (sizeof(*s));
	s->next = zh->sets;
	zh->sets = s;
	s->name = (char *) xmalloc (strlen(name)+1);
	strcpy (s->name, name);

	s->sort_info = (struct zset_sort_info *)
	    xmalloc (sizeof(*s->sort_info));
	s->sort_info->max_entries = atoi(sort_max_str);
	if (s->sort_info->max_entries < 2)
	    s->sort_info->max_entries = 2;

	s->sort_info->entries = (struct zset_sort_entry **)
	    xmalloc (sizeof(*s->sort_info->entries) *
		     s->sort_info->max_entries);
	s->sort_info->all_entries = (struct zset_sort_entry *)
	    xmalloc (sizeof(*s->sort_info->all_entries) *
		     s->sort_info->max_entries);
	for (i = 0; i < s->sort_info->max_entries; i++)
	    s->sort_info->entries[i] = s->sort_info->all_entries + i;
    }
    s->locked = 0;
    s->term_entries = 0;
    s->hits = 0;
    s->rset = 0;
    s->nmem = 0;
    s->rpn = 0;
    s->sortSpec = 0;
    s->cache_position = 0;
    s->cache_rfd = 0;
    return s;
}

ZebraSet resultSetGet (ZebraHandle zh, const char *name)
{
    ZebraSet s;

    for (s = zh->sets; s; s = s->next)
        if (!strcmp (s->name, name))
        {
            if (!s->term_entries && !s->rset && s->rpn)
            {
                NMEM nmem = nmem_create ();
                yaz_log (LOG_LOG, "research %s", name);
                s->rset =
                    rpn_search (zh, nmem, s->rpn, s->num_bases,
				s->basenames, s->name, s);
		if (s->rset && s->sortSpec)
		{
		    int sort_status;
		    yaz_log (LOG_LOG, "resort %s", name);
		    resultSetSortSingle (zh, nmem, s, s->rset, s->sortSpec,
					 &sort_status);
		}
                nmem_destroy (nmem);
            }
            return s;
        }
    return NULL;
}

void resultSetInvalidate (ZebraHandle zh)
{
    ZebraSet s = zh->sets;
    
    for (; s; s = s->next)
    {
        if (s->rset)
	{
	    if (s->cache_rfd)
		rset_close(s->rset, s->cache_rfd);
            rset_delete (s->rset);
	}
        s->rset = 0;
	s->cache_rfd = 0;
	s->cache_position = 0;
    }
}

void resultSetDestroy (ZebraHandle zh, int num, char **names,int *statuses)
{
    ZebraSet * ss = &zh->sets;
    int i;
    
    if (statuses)
	for (i = 0; i<num; i++)
	    statuses[i] = Z_DeleteStatus_resultSetDidNotExist;
    zh->errCode = 0;
    zh->errString = NULL;
    while (*ss)
    {
	int i = -1;
	ZebraSet s = *ss;
	if (num >= 0)
	{
	    for (i = 0; i<num; i++)
		if (!strcmp (s->name, names[i]))
		{
		    if (statuses)
			statuses[i] = Z_DeleteStatus_success;
		    i = -1;
		    break;
		}
	}
	if (i < 0)
	{
	    *ss = s->next;
	    
	    xfree (s->sort_info->all_entries);
	    xfree (s->sort_info->entries);
	    xfree (s->sort_info);
	    
	    if (s->nmem)
		nmem_destroy (s->nmem);
	    if (s->rset)
	    {
		if (s->cache_rfd)
		    rset_close (s->rset, s->cache_rfd);
		rset_delete (s->rset);
	    }
	    xfree (s->name);
	    xfree (s);
	}
	else
	    ss = &s->next;
    }
}

ZebraPosSet zebraPosSetCreate (ZebraHandle zh, const char *name, 
			       int num, int *positions)
{
    ZebraSet sset;
    ZebraPosSet sr = 0;
    RSET rset;
    int i;
    struct zset_sort_info *sort_info;

    if (!(sset = resultSetGet (zh, name)))
        return NULL;
    if (!(rset = sset->rset))
    {
	if (!sset->term_entries)
	    return 0;
	sr = (ZebraPosSet) xmalloc (sizeof(*sr) * num);
	for (i = 0; i<num; i++)
	{
	    sr[i].sysno = 0;
	    sr[i].score = -1;
	    sr[i].term = 0;
	    sr[i].db = 0;

	    if (positions[i] <= sset->term_entries_max)
	    {
		sr[i].term = sset->term_entries[positions[i]-1].term;
		sr[i].db = sset->term_entries[positions[i]-1].db;
	    }
	}
    }
    else
    {
	sr = (ZebraPosSet) xmalloc (sizeof(*sr) * num);
	for (i = 0; i<num; i++)
	{
	    sr[i].sysno = 0;
	    sr[i].score = -1;
	    sr[i].term = 0;
	    sr[i].db = 0;
	}
	sort_info = sset->sort_info;
	if (sort_info)
	{
	    int position;
	    
	    for (i = 0; i<num; i++)
	    {
		position = positions[i];
		if (position > 0 && position <= sort_info->num_entries)
		{
		    yaz_log (LOG_DEBUG, "got pos=%d (sorted)", position);
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
	    
	    if (sset->cache_rfd && 
		num_i < num && positions[num_i] > sset->cache_position)
	    {
		position = sset->cache_position;
		rfd = sset->cache_rfd;
		psysno = sset->cache_psysno;
	    } 
	    else
	    {
		if (sset->cache_rfd)
		    rset_close(rset, sset->cache_rfd);
		rfd = rset_open (rset, RSETF_READ);
	    }
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
			yaz_log (LOG_DEBUG, "got pos=%d (unsorted)", position);
			sr[num_i].score = -1;
			num_i++;
		    }
		}
	    }
	    sset->cache_position = position;
	    sset->cache_psysno = psysno;
	    sset->cache_rfd = rfd;
	}
    }
    return sr;
}

void zebraPosSetDestroy (ZebraHandle zh, ZebraPosSet records, int num)
{
    xfree (records);
}

struct sortKeyInfo {
    int relation;
    int attrUse;
    int numerical;
};

void resultSetInsertSort (ZebraHandle zh, ZebraSet sset,
			  struct sortKeyInfo *criteria, int num_criteria,
			  int sysno)
{
    struct zset_sort_entry this_entry;
    struct zset_sort_entry *new_entry = NULL;
    struct zset_sort_info *sort_info = sset->sort_info;
    int i, j;

    sortIdx_sysno (zh->reg->sortIdx, sysno);
    for (i = 0; i<num_criteria; i++)
    {
        memset(this_entry.buf[i], '\0', SORT_IDX_ENTRYSIZE);
	if (criteria[i].attrUse != -1)
	{
	    sortIdx_type (zh->reg->sortIdx, criteria[i].attrUse);
	    sortIdx_read (zh->reg->sortIdx, this_entry.buf[i]);
	}
    }
    i = sort_info->num_entries;
    while (--i >= 0)
    {
	int rel = 0;
	for (j = 0; j<num_criteria; j++)
	{
            if (criteria[j].numerical)
            {
                double diff = atof(this_entry.buf[j]) -
                              atof(sort_info->entries[i]->buf[j]);
                rel = 0;
                if (diff > 0.0)
                    rel = 1;
                else if (diff < 0.0)
                    rel = -1;
            }
            else
            {
	        rel = memcmp (this_entry.buf[j], sort_info->entries[i]->buf[j],
			  SORT_IDX_ENTRYSIZE);
            }
	    if (rel)
		break;
	}	
	if (!rel)
	    break;
	if (criteria[j].relation == 'A')
        {
	    if (rel > 0)
	        break;
	}
        else if (criteria[j].relation == 'D')
	{
	    if (rel < 0)
		break;
	}
    }
    ++i;
    j = sort_info->max_entries;
    if (i == j)
	return;

    if (sort_info->num_entries == j)
	--j;
    else
	j = (sort_info->num_entries)++;
    new_entry = sort_info->entries[j];
    while (j != i)
    {
	sort_info->entries[j] = sort_info->entries[j-1];
	--j;
    }
    sort_info->entries[i] = new_entry;
    assert (new_entry);
    for (i = 0; i<num_criteria; i++)
	memcpy (new_entry->buf[i], this_entry.buf[i], SORT_IDX_ENTRYSIZE);
    new_entry->sysno = sysno;
    new_entry->score = -1;
}

void resultSetInsertRank(ZebraHandle zh, struct zset_sort_info *sort_info,
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
    ++i;
    j = sort_info->max_entries;
    if (i == j)
	return;

    if (sort_info->num_entries == j)
	--j;
    else
	j = (sort_info->num_entries)++;
    
    new_entry = sort_info->entries[j];
    while (j != i)
    {
	sort_info->entries[j] = sort_info->entries[j-1];
	--j;
    }
    sort_info->entries[i] = new_entry;
    assert (new_entry);
    new_entry->sysno = sysno;
    new_entry->score = score;
}


static Z_RPNQuery *copy_RPNQuery(Z_RPNQuery *src, NMEM nmem)
{
    Z_RPNQuery *dst = 0;
    ODR encode = odr_createmem(ODR_ENCODE);
    ODR decode = odr_createmem(ODR_DECODE);

    if (z_RPNQuery(encode, &src, 0, 0))
    {
	int len;
	char *buf = odr_getbuf(encode, &len, 0);

	if (buf)
	{
	    odr_setbuf(decode, buf, len, 0);
	    z_RPNQuery(decode, &dst, 0, 0);
	}
    }
    nmem_transfer(nmem, decode->mem);
    odr_destroy(encode);
    odr_destroy(decode);
    return dst;
}

static Z_SortKeySpecList *copy_SortKeySpecList(Z_SortKeySpecList *src, NMEM nmem)
{
    Z_SortKeySpecList *dst = 0;
    ODR encode = odr_createmem(ODR_ENCODE);
    ODR decode = odr_createmem(ODR_DECODE);

    if (z_SortKeySpecList(encode, &src, 0, 0))
    {
	int len;
	char *buf = odr_getbuf(encode, &len, 0);

	if (buf)
	{
	    odr_setbuf(decode, buf, len, 0);
	    z_SortKeySpecList(decode, &dst, 0, 0);
	}
    }
    nmem_transfer(nmem, decode->mem);
    odr_destroy(encode);
    odr_destroy(decode);
    return dst;
}

ZebraSet resultSetClone(ZebraHandle zh, const char *setname,
			ZebraSet rset)
{
    ZebraSet nset;
    int i;

    nset = resultSetAdd(zh, setname, 1);
    if (!nset)
	return 0;

    nset->nmem = nmem_create();

    nset->num_bases = rset->num_bases;
    nset->basenames = 
        nmem_malloc (nset->nmem, nset->num_bases * sizeof(*rset->basenames));
    for (i = 0; i<rset->num_bases; i++)
        nset->basenames[i] = nmem_strdup(nset->nmem, rset->basenames[i]);

    if (rset->rset)
	nset->rset = rset_dup(rset->rset);
    if (rset->rpn)
	nset->rpn = copy_RPNQuery(rset->rpn, nset->nmem);
    return nset;
}

void resultSetSort (ZebraHandle zh, NMEM nmem,
		    int num_input_setnames, const char **input_setnames,
		    const char *output_setname,
		    Z_SortKeySpecList *sort_sequence, int *sort_status)
{
    ZebraSet sset;
    RSET rset;

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
    yaz_log (LOG_DEBUG, "result set sort input=%s output=%s",
	  *input_setnames, output_setname);
    sset = resultSetGet (zh, input_setnames[0]);
    if (!sset)
    {
	zh->errCode = 30;
	zh->errString = nmem_strdup (nmem, input_setnames[0]);
	return;
    }
    if (!(rset = sset->rset))
    {
	zh->errCode = 30;
	zh->errString = nmem_strdup (nmem, input_setnames[0]);
        return;
    }
    if (strcmp (output_setname, input_setnames[0]))
	sset = resultSetClone(zh, output_setname, sset);
    sset->sortSpec = copy_SortKeySpecList(sort_sequence, sset->nmem);
    resultSetSortSingle (zh, nmem, sset, rset, sort_sequence, sort_status);
}

void resultSetSortSingle (ZebraHandle zh, NMEM nmem,
			  ZebraSet sset, RSET rset,
			  Z_SortKeySpecList *sort_sequence, int *sort_status)
{
    int kno = 0;
    int i, psysno = 0;
    struct it_key key;
    struct sortKeyInfo sort_criteria[3];
    int num_criteria;
    int term_index;
    RSFD rfd;

    sset->sort_info->num_entries = 0;

    sset->hits = 0;
    num_criteria = sort_sequence->num_specs;
    if (num_criteria > 3)
	num_criteria = 3;
    for (i = 0; i < num_criteria; i++)
    {
	Z_SortKeySpec *sks = sort_sequence->specs[i];
	Z_SortKey *sk;

	if (*sks->sortRelation == Z_SortKeySpec_ascending)
	    sort_criteria[i].relation = 'A';
	else if (*sks->sortRelation == Z_SortKeySpec_descending)
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

	if (sks->which == Z_SortKeySpec_missingValueData)
	{
	    zh->errCode = 213;
	    return;
	}

	sk = sks->sortElement->u.generic;
	switch (sk->which)
	{
	case Z_SortKey_sortField:
	    yaz_log (LOG_DEBUG, "Sort: key %d is of type sortField", i+1);
	    zh->errCode = 207;
	    return;
	case Z_SortKey_elementSpec:
	    yaz_log (LOG_DEBUG, "Sort: key %d is of type elementSpec", i+1);
	    zh->errCode = 207;
	    return;
	case Z_SortKey_sortAttributes:
	    yaz_log (LOG_DEBUG, "Sort: key %d is of type sortAttributes", i+1);
	    sort_criteria[i].attrUse =
		zebra_maps_sort (zh->reg->zebra_maps,
				 sk->u.sortAttributes,
                                 &sort_criteria[i].numerical);
	    yaz_log (LOG_DEBUG, "use value = %d", sort_criteria[i].attrUse);
	    if (sort_criteria[i].attrUse == -1 && sks->which != Z_SortKeySpec_null)
	    {
		zh->errCode = 116;
		return;
	    }
	    if (sortIdx_type (zh->reg->sortIdx, sort_criteria[i].attrUse))
	    {
		if (sks->which != Z_SortKeySpec_null)
		{
		    zh->errCode = 207;
		    return;
		}
		else
		    sort_criteria[i].attrUse = -1;
	    }
	    break;
	}
    }
    rfd = rset_open (rset, RSETF_READ);
    while (rset_read (rset, rfd, &key, &term_index))
    {
	kno++;
        if (key.sysno != psysno)
        {
	    (sset->hits)++;
            psysno = key.sysno;
	    resultSetInsertSort (zh, sset,
				 sort_criteria, num_criteria, psysno);
        }
    }
    rset_close (rset, rfd);

    yaz_log (LOG_LOG, "%d keys, %d sysnos, sort", kno, sset->hits);
    for (i = 0; i < rset->no_rset_terms; i++)
	yaz_log (LOG_LOG, "term=\"%s\" nn=%d type=%s count=%d",
                 rset->rset_terms[i]->name,
                 rset->rset_terms[i]->nn,
                 rset->rset_terms[i]->flags,
                 rset->rset_terms[i]->count);

    *sort_status = Z_SortResponse_success;
}

RSET resultSetRef (ZebraHandle zh, const char *resultSetId)
{
    ZebraSet s;

    if ((s = resultSetGet (zh, resultSetId)))
	return s->rset;
    return NULL;
}

void resultSetRank (ZebraHandle zh, ZebraSet zebraSet, RSET rset)
{
    int kno = 0;
    struct it_key key;
    RSFD rfd;
    int term_index, i;
    ZebraRankClass rank_class;
    struct rank_control *rc;
    struct zset_sort_info *sort_info;
    const char *rank_handler_name = res_get_def(zh->res, "rank", "rank-1");

    sort_info = zebraSet->sort_info;
    sort_info->num_entries = 0;
    zebraSet->hits = 0;
    rfd = rset_open (rset, RSETF_READ);

    rank_class = zebraRankLookup (zh, rank_handler_name);
    if (!rank_class)
    {
        yaz_log (LOG_WARN, "No such rank handler: %s", rank_handler_name);
        return;
    }
    rc = rank_class->control;

    if (rset_read (rset, rfd, &key, &term_index))
    {
	int psysno = key.sysno;
	int score;
	void *handle =
	    (*rc->begin) (zh->reg, rank_class->class_handle, rset);
	(zebraSet->hits)++;
	do
	{
	    kno++;
	    if (key.sysno != psysno)
	    {
		score = (*rc->calc) (handle, psysno);

		resultSetInsertRank (zh, sort_info, psysno, score, 'A');
		(zebraSet->hits)++;
		psysno = key.sysno;
	    }
	    (*rc->add) (handle, key.seqno, term_index);
	}
	while (rset_read (rset, rfd, &key, &term_index));
	score = (*rc->calc) (handle, psysno);
	resultSetInsertRank (zh, sort_info, psysno, score, 'A');
	(*rc->end) (zh->reg, handle);
    }
    rset_close (rset, rfd);

    yaz_log (LOG_LOG, "%d keys, %d sysnos, rank", kno, zebraSet->hits);
    for (i = 0; i < rset->no_rset_terms; i++)
	yaz_log (LOG_LOG, "term=\"%s\" nn=%d type=%s count=%d",
                 rset->rset_terms[i]->name,
                 rset->rset_terms[i]->nn,
                 rset->rset_terms[i]->flags,
                 rset->rset_terms[i]->count);
    
}

ZebraRankClass zebraRankLookup (ZebraHandle zh, const char *name)
{
    ZebraRankClass p = zh->reg->rank_classes;
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

void zebraRankInstall (struct zebra_register *reg, struct rank_control *ctrl)
{
    ZebraRankClass p = (ZebraRankClass) xmalloc (sizeof(*p));
    p->control = (struct rank_control *) xmalloc (sizeof(*p->control));
    memcpy (p->control, ctrl, sizeof(*p->control));
    p->control->name = xstrdup (ctrl->name);
    p->init_flag = 0;
    p->next = reg->rank_classes;
    reg->rank_classes = p;
}

void zebraRankDestroy (struct zebra_register *reg)
{
    ZebraRankClass p = reg->rank_classes;
    while (p)
    {
	ZebraRankClass p_next = p->next;
	if (p->init_flag && p->control->destroy)
	    (*p->control->destroy)(reg, p->class_handle);
	xfree (p->control->name);
	xfree (p->control);
	xfree (p);
	p = p_next;
    }
    reg->rank_classes = NULL;
}
