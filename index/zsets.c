/* $Id: zsets.c,v 1.92 2005-08-18 19:20:38 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
#include <yaz/diagbib1.h>
#include <rset.h>

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
    NMEM rset_nmem; /* for creating the rsets in */
    zint hits;
    int num_bases;
    char **basenames;
    Z_RPNQuery *rpn;
    struct zset_sort_info *sort_info;
    struct zebra_set_term_entry *term_entries;
    int term_entries_max;
    struct zebra_set *next;
    int locked;

    zint cache_position;  /* last position */
    RSFD cache_rfd;       /* rfd (NULL if not existing) */
    zint cache_psysno;    /* sysno for last position */
    zint approx_limit;    /* limit before we do approx */
};

struct zset_sort_entry {
    zint sysno;
    int score;
    char buf[ZSET_SORT_MAX_LEVEL][SORT_IDX_ENTRYSIZE];
};

struct zset_sort_info {
    int max_entries;
    int num_entries;
    struct zset_sort_entry *all_entries;
    struct zset_sort_entry **entries;
};

static int log_level_set=0;
static int log_level_sort=0;
static int log_level_searchhits=0;
static int log_level_searchterms=0;
static int log_level_resultsets=0;

static void loglevels()
{
    if (log_level_set)
        return;
    log_level_sort = yaz_log_module_level("sorting");
    log_level_searchhits = yaz_log_module_level("searchhits");
    log_level_searchterms = yaz_log_module_level("searchterms");
    log_level_resultsets = yaz_log_module_level("resultsets");
    log_level_set = 1;
}

ZEBRA_RES resultSetSearch(ZebraHandle zh, NMEM nmem, NMEM rset_nmem,
			  Z_RPNQuery *rpn, ZebraSet sset)
{
    RSET rset = 0;
    oident *attrset;
    Z_SortKeySpecList *sort_sequence;
    int sort_status, i;
    ZEBRA_RES res = ZEBRA_OK;

    zh->hits = 0;

    sort_sequence = (Z_SortKeySpecList *)
        nmem_malloc(nmem, sizeof(*sort_sequence));
    sort_sequence->num_specs = 10; /* FIXME - Hard-coded number */
    sort_sequence->specs = (Z_SortKeySpec **)
        nmem_malloc(nmem, sort_sequence->num_specs *
                     sizeof(*sort_sequence->specs));
    for (i = 0; i<sort_sequence->num_specs; i++)
        sort_sequence->specs[i] = 0;
    
    attrset = oid_getentbyoid (rpn->attributeSetId);
    res = rpn_search_top(zh, rpn->RPNStructure, attrset->value,
			 nmem, rset_nmem,
			 sort_sequence,
			 sset->num_bases, sset->basenames,
			 &rset);
    if (res != ZEBRA_OK)
    {
	sset->rset = 0;
        return res;
    }
    for (i = 0; sort_sequence->specs[i]; i++)
        ;
    sort_sequence->num_specs = i;
    rset->hits_limit = sset->approx_limit;
    if (!i)
    {
        res = resultSetRank (zh, sset, rset, rset_nmem);
    }
    else
    {
        res = resultSetSortSingle (zh, nmem, sset, rset,
				   sort_sequence, &sort_status);
    }
    sset->rset = rset;
    return res;
}


ZEBRA_RES resultSetAddRPN (ZebraHandle zh, NMEM m, Z_RPNQuery *rpn,
			   int num_bases, char **basenames,
			   const char *setname)
{
    ZebraSet zebraSet;
    int i;
    ZEBRA_RES res;

    zh->hits = 0;

    zebraSet = resultSetAdd(zh, setname, 1);
    if (!zebraSet)
        return ZEBRA_FAIL;
    zebraSet->locked = 1;
    zebraSet->rpn = 0;
    zebraSet->nmem = m;
    zebraSet->rset_nmem = nmem_create(); 

    zebraSet->num_bases = num_bases;
    zebraSet->basenames = 
        nmem_malloc (zebraSet->nmem, num_bases * sizeof(*zebraSet->basenames));
    for (i = 0; i<num_bases; i++)
        zebraSet->basenames[i] = nmem_strdup(zebraSet->nmem, basenames[i]);

    res = resultSetSearch(zh, zebraSet->nmem, zebraSet->rset_nmem,
			  rpn, zebraSet);
    zh->hits = zebraSet->hits;
    if (zebraSet->rset)
        zebraSet->rpn = rpn;
    zebraSet->locked = 0;
    if (!zebraSet->rset)
	return ZEBRA_FAIL;
    return res;
}

void resultSetAddTerm (ZebraHandle zh, ZebraSet s, int reg_type,
                       const char *db, int set,
                       int use, const char *term)
{
    assert(zh); /* compiler shut up */
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

ZebraSet resultSetAdd(ZebraHandle zh, const char *name, int ov)
{
    ZebraSet s;
    int i;

    for (s = zh->sets; s; s = s->next)
        if (!strcmp (s->name, name))
            break;
    
    if (!log_level_set)
        loglevels();
    if (s)
    {
        yaz_log(log_level_resultsets, "updating result set %s", name);
        if (!ov || s->locked)
            return NULL;
        if (s->rset)
	{
	    if (s->cache_rfd)
		rset_close(s->cache_rfd);
            rset_delete (s->rset);
	}
        if (s->rset_nmem)
            nmem_destroy (s->rset_nmem);
        if (s->nmem)
            nmem_destroy (s->nmem);
    }
    else
    {
        const char *sort_max_str = zebra_get_resource(zh, "sortmax", "1000");

        yaz_log(log_level_resultsets, "adding result set %s", name);
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
    s->rset_nmem = 0;
    s->nmem = 0;
    s->rpn = 0;
    s->cache_position = 0;
    s->cache_rfd = 0;
    s->approx_limit = zh->approx_limit;
    return s;
}

ZebraSet resultSetGet(ZebraHandle zh, const char *name)
{
    ZebraSet s;

    for (s = zh->sets; s; s = s->next)
        if (!strcmp (s->name, name))
        {
            if (!s->term_entries && !s->rset && s->rpn)
            {
                NMEM nmem = nmem_create ();
                yaz_log(log_level_resultsets, "research %s", name);
                if (!s->rset_nmem)
                    s->rset_nmem=nmem_create();
		resultSetSearch(zh, nmem, s->rset_nmem, s->rpn, s);
                nmem_destroy (nmem);
            }
            return s;
        }
    return NULL;
}

void resultSetInvalidate (ZebraHandle zh)
{
    ZebraSet s = zh->sets;
    
    yaz_log(log_level_resultsets, "invalidating result sets");
    for (; s; s = s->next)
    {
        if (s->rset)
	{
	    if (s->cache_rfd)
		rset_close(s->cache_rfd);
            rset_delete (s->rset);
	}
        s->rset = 0;
	s->cache_rfd = 0;
	s->cache_position = 0;
        if (s->rset_nmem)
            nmem_destroy(s->rset_nmem);
        s->rset_nmem=0;
    }
}

void resultSetDestroy(ZebraHandle zh, int num, char **names,int *statuses)
{
    ZebraSet * ss = &zh->sets;
    int i;
    
    if (statuses)
        for (i = 0; i<num; i++)
            statuses[i] = Z_DeleteStatus_resultSetDidNotExist;
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
		    rset_close(s->cache_rfd);
                rset_delete (s->rset);
	    }
            if (s->rset_nmem)
                nmem_destroy(s->rset_nmem);
            xfree (s->name);
            xfree (s);
        }
        else
            ss = &s->next;
    }
}

ZebraMetaRecord *zebra_meta_records_create_range(ZebraHandle zh,
						 const char *name, 
						 zint start, int num)
{
    zint pos_small[10];
    zint *pos = pos_small;
    ZebraMetaRecord *mr;
    int i;

    if (num > 10000 || num <= 0)
	return 0;

    if (num > 10)
	pos = xmalloc(sizeof(*pos) * num);
    
    for (i = 0; i<num; i++)
	pos[i] = start+i;

    mr = zebra_meta_records_create(zh, name, num, pos);
    
    if (num > 10)
	xfree(pos);
    return mr;
}

ZebraMetaRecord *zebra_meta_records_create(ZebraHandle zh, const char *name, 
					   int num, zint *positions)
{
    ZebraSet sset;
    ZebraMetaRecord *sr = 0;
    RSET rset;
    int i;
    struct zset_sort_info *sort_info;
    size_t sysno_mem_index = 0;

    if (zh->m_staticrank)
	sysno_mem_index = 1;

    if (!log_level_set)
        loglevels();
    if (!(sset = resultSetGet (zh, name)))
        return NULL;
    if (!(rset = sset->rset))
    {
        if (!sset->term_entries)
            return 0;
        sr = (ZebraMetaRecord *) xmalloc (sizeof(*sr) * num);
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
        sr = (ZebraMetaRecord *) xmalloc (sizeof(*sr) * num);
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
            zint position;
            
            for (i = 0; i<num; i++)
            {
                position = positions[i];
                if (position > 0 && position <= sort_info->num_entries)
                {
                    yaz_log(log_level_sort, "got pos=" ZINT_FORMAT
			    " (sorted)", position);
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
            zint position = 0;
            int num_i = 0;
            zint psysno = 0;
            RSFD rfd;
            struct it_key key;
            
            if (sort_info)
                position = sort_info->num_entries;
            while (num_i < num && positions[num_i] <= position)
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
		    rset_close(sset->cache_rfd);
		rfd = rset_open (rset, RSETF_READ);
	    }
            while (num_i < num && rset_read (rfd, &key, 0))
            {
                zint this_sys = key.mem[sysno_mem_index];
                if (this_sys != psysno)
                {
                    psysno = this_sys;
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
                        yaz_log(log_level_sort, "got pos=" ZINT_FORMAT " (unsorted)", position);
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

void zebra_meta_records_destroy (ZebraHandle zh, ZebraMetaRecord *records,
				 int num)
{
    assert(zh); /* compiler shut up about unused arg */
    xfree (records);
}

struct sortKeyInfo {
    int relation;
    int attrUse;
    int numerical;
};

void resultSetInsertSort (ZebraHandle zh, ZebraSet sset,
                          struct sortKeyInfo *criteria, int num_criteria,
                          zint sysno)
{
    struct zset_sort_entry this_entry;
    struct zset_sort_entry *new_entry = NULL;
    struct zset_sort_info *sort_info = sset->sort_info;
    int i, j;

    sortIdx_sysno (zh->reg->sortIdx, sysno);
    for (i = 0; i<num_criteria; i++)
    {
        sortIdx_type (zh->reg->sortIdx, criteria[i].attrUse);
        sortIdx_read (zh->reg->sortIdx, this_entry.buf[i]);
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

void resultSetInsertRank (ZebraHandle zh, struct zset_sort_info *sort_info,
                          zint sysno, int score, int relation)
{
    struct zset_sort_entry *new_entry = NULL;
    int i, j;
    assert(zh); /* compiler shut up about unused arg */

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

ZEBRA_RES resultSetSort(ZebraHandle zh, NMEM nmem,
			int num_input_setnames, const char **input_setnames,
			const char *output_setname,
			Z_SortKeySpecList *sort_sequence, int *sort_status)
{
    ZebraSet sset;
    RSET rset;

    if (num_input_setnames == 0)
    {
	zebra_setError(zh, YAZ_BIB1_NO_RESULT_SET_NAME_SUPPLIED_ON_SORT, 0);
	return ZEBRA_FAIL;
    }
    if (num_input_setnames > 1)
    {
        zebra_setError(zh, YAZ_BIB1_SORT_TOO_MANY_INPUT_RESULTS, 0);
	return ZEBRA_FAIL;
    }
    if (!log_level_set)
        loglevels();
    yaz_log(log_level_sort, "result set sort input=%s output=%s",
          *input_setnames, output_setname);
    sset = resultSetGet (zh, input_setnames[0]);
    if (!sset)
    {
	zebra_setError(zh, YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
		       input_setnames[0]);
	return ZEBRA_FAIL;
    }
    if (!(rset = sset->rset))
    {
	zebra_setError(zh, YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
		       input_setnames[0]);
	return ZEBRA_FAIL;
    }
    if (strcmp (output_setname, input_setnames[0]))
    {
        rset = rset_dup (rset);
        sset = resultSetAdd (zh, output_setname, 1);
        sset->rset = rset;
    }
    return resultSetSortSingle (zh, nmem, sset, rset, sort_sequence,
				sort_status);
}

ZEBRA_RES resultSetSortSingle(ZebraHandle zh, NMEM nmem,
			      ZebraSet sset, RSET rset,
			      Z_SortKeySpecList *sort_sequence,
			      int *sort_status)
{
    int i;
    int n = 0;
    zint kno = 0;
    zint psysno = 0;
    struct it_key key;
    struct sortKeyInfo sort_criteria[3];
    int num_criteria;
    RSFD rfd;
    TERMID termid;
    TERMID *terms;
    int numTerms = 0;
    size_t sysno_mem_index = 0;

    if (zh->m_staticrank)
	sysno_mem_index = 1;


    assert(nmem); /* compiler shut up about unused param */
    sset->sort_info->num_entries = 0;

    rset_getterms(rset, 0, 0, &n);
    terms = (TERMID *) nmem_malloc(nmem, sizeof(*terms)*n);
    rset_getterms(rset, terms, n, &numTerms);

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
	    zebra_setError(zh, YAZ_BIB1_ILLEGAL_SORT_RELATION, 0);
            return ZEBRA_FAIL;
        }
        if (sks->sortElement->which == Z_SortElement_databaseSpecific)
        {
	    zebra_setError(zh, YAZ_BIB1_DATABASE_SPECIFIC_SORT_UNSUPP, 0);
            return ZEBRA_FAIL;
        }
        else if (sks->sortElement->which != Z_SortElement_generic)
        {
	    zebra_setError(zh, YAZ_BIB1_SORT_ILLEGAL_SORT, 0);
            return ZEBRA_FAIL;
        }       
        sk = sks->sortElement->u.generic;
        switch (sk->which)
        {
        case Z_SortKey_sortField:
            yaz_log(log_level_sort, "key %d is of type sortField",
		    i+1);
            zebra_setError(zh, YAZ_BIB1_CANNOT_SORT_ACCORDING_TO_SEQUENCE, 0);
            return ZEBRA_FAIL;
        case Z_SortKey_elementSpec:
            yaz_log(log_level_sort, "key %d is of type elementSpec",
		    i+1);
            zebra_setError(zh, YAZ_BIB1_CANNOT_SORT_ACCORDING_TO_SEQUENCE, 0);
            return ZEBRA_FAIL;
        case Z_SortKey_sortAttributes:
            yaz_log(log_level_sort, "key %d is of type sortAttributes", i+1);
            sort_criteria[i].attrUse =
                zebra_maps_sort (zh->reg->zebra_maps,
                                 sk->u.sortAttributes,
                                 &sort_criteria[i].numerical);
            yaz_log(log_level_sort, "use value = %d", sort_criteria[i].attrUse);
            if (sort_criteria[i].attrUse == -1)
            {
		zebra_setError(
		    zh, YAZ_BIB1_USE_ATTRIBUTE_REQUIRED_BUT_NOT_SUPPLIED, 0); 
                return ZEBRA_FAIL;
            }
            if (sortIdx_type (zh->reg->sortIdx, sort_criteria[i].attrUse))
            {
		zebra_setError(
		    zh, YAZ_BIB1_CANNOT_SORT_ACCORDING_TO_SEQUENCE, 0);
                return ZEBRA_FAIL;
            }
            break;
        }
    }
    rfd = rset_open (rset, RSETF_READ);
    while (rset_read (rfd, &key, &termid))
    {
        zint this_sys = key.mem[sysno_mem_index];
	if (log_level_searchhits)
	    key_logdump_txt(log_level_searchhits, &key, termid->name);
	kno++;
        if (this_sys != psysno)
        {
            (sset->hits)++;
            psysno = this_sys;
            resultSetInsertSort (zh, sset,
                                 sort_criteria, num_criteria, psysno);
        }
    }
    rset_close (rfd);
    yaz_log(log_level_sort, ZINT_FORMAT " keys, " ZINT_FORMAT " sysnos, sort",
	    kno, sset->hits);   
    for (i = 0; i < numTerms; i++)
        yaz_log(log_level_sort, "term=\"%s\" type=%s count=" ZINT_FORMAT,
                 terms[i]->name, terms[i]->flags, terms[i]->rset->hits_count);
    *sort_status = Z_SortResponse_success;
    return ZEBRA_OK;
}

RSET resultSetRef(ZebraHandle zh, const char *resultSetId)
{
    ZebraSet s;

    if ((s = resultSetGet (zh, resultSetId)))
        return s->rset;
    return NULL;
}

ZEBRA_RES resultSetRank(ZebraHandle zh, ZebraSet zebraSet,
			RSET rset, NMEM nmem)
{
    struct it_key key;
    TERMID termid;
    TERMID *terms;
    zint kno = 0;
    int numTerms = 0;
    int n = 0;
    int i;
    ZebraRankClass rank_class;
    struct zset_sort_info *sort_info;
    const char *rank_handler_name = res_get_def(zh->res, "rank", "rank-1");
    size_t sysno_mem_index = 0;

    if (zh->m_staticrank)
	sysno_mem_index = 1;

    if (!log_level_set)
        loglevels();
    sort_info = zebraSet->sort_info;
    sort_info->num_entries = 0;
    zebraSet->hits = 0;
    rset_getterms(rset, 0, 0, &n);
    terms = (TERMID *) nmem_malloc(nmem, sizeof(*terms)*n);
    rset_getterms(rset, terms, n, &numTerms);


    rank_class = zebraRankLookup(zh, rank_handler_name);
    if (!rank_class)
    {
        yaz_log(YLOG_WARN, "No such rank handler: %s", rank_handler_name);
	zebra_setError(zh, YAZ_BIB1_UNSUPP_SEARCH, "Cannot find rank handler");
        return ZEBRA_FAIL;
    }
    else
    {
	RSFD rfd = rset_open(rset, RSETF_READ);
	struct rank_control *rc = rank_class->control;
	double score;
	zint count = 0;
	
	void *handle =
	    (*rc->begin) (zh->reg, rank_class->class_handle, rset, nmem,
			  terms, numTerms);
	zint psysno = 0;
	while (rset_read(rfd, &key, &termid))
	{
	    zint this_sys = key.mem[sysno_mem_index];
	    zint seqno = key.mem[key.len-1];
	    kno++;
	    if (log_level_searchhits)
		key_logdump_txt(log_level_searchhits, &key, termid->name);
	    if (this_sys != psysno)
	    {
		if (rfd->counted_items > rset->hits_limit)
		    break;
		if (psysno)
		{
		    score = (*rc->calc) (handle, psysno);
		    resultSetInsertRank (zh, sort_info, psysno, score, 'A');
		    count++;
		}
		psysno = this_sys;
	    }
	    (*rc->add) (handle, CAST_ZINT_TO_INT(seqno), termid);
	}
	if (psysno)
	{
	    score = (*rc->calc)(handle, psysno);
	    resultSetInsertRank(zh, sort_info, psysno, score, 'A');
	    count++;
	}
	(*rc->end) (zh->reg, handle);
	rset_close (rfd);
    }
    zebraSet->hits = rset->hits_count;

    yaz_log(log_level_searchterms, ZINT_FORMAT " keys, "
	    ZINT_FORMAT " sysnos, rank",  kno, zebraSet->hits);
    for (i = 0; i < numTerms; i++)
    {
        yaz_log(log_level_searchterms, "term=\"%s\" type=%s count="
		ZINT_FORMAT,
		terms[i]->name, terms[i]->flags, terms[i]->rset->hits_count);
    }
    return ZEBRA_OK;
}

ZebraRankClass zebraRankLookup(ZebraHandle zh, const char *name)
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

void zebraRankInstall(struct zebra_register *reg, struct rank_control *ctrl)
{
    ZebraRankClass p = (ZebraRankClass) xmalloc (sizeof(*p));
    p->control = (struct rank_control *) xmalloc (sizeof(*p->control));
    memcpy (p->control, ctrl, sizeof(*p->control));
    p->control->name = xstrdup (ctrl->name);
    p->init_flag = 0;
    p->next = reg->rank_classes;
    reg->rank_classes = p;
}

void zebraRankDestroy(struct zebra_register *reg)
{
    ZebraRankClass p = reg->rank_classes;
    while (p)
    {
        ZebraRankClass p_next = p->next;
        if (p->init_flag && p->control->destroy)
            (*p->control->destroy)(reg, p->class_handle);
        xfree(p->control->name);
        xfree(p->control);
        xfree(p);
        p = p_next;
    }
    reg->rank_classes = NULL;
}

static int trav_rset_for_termids(RSET rset, TERMID *termid_array,
				 zint *hits_array, int *approx_array)
{
    int no = 0;
    int i;
    for (i = 0; i<rset->no_children; i++)
	no += trav_rset_for_termids(rset->children[i],
				    (termid_array ? termid_array + no : 0),
				    (hits_array ? hits_array + no : 0),
				    (approx_array ? approx_array + no : 0));
    if (rset->term)
    {
	if (termid_array)
	    termid_array[no] = rset->term;
	if (hits_array)
	    hits_array[no] = rset->hits_count;
	if (approx_array)
	    approx_array[no] = rset->hits_approx;
#if 0
	yaz_log(YLOG_LOG, "rset=%p term=%s limit=" ZINT_FORMAT
		" count=" ZINT_FORMAT,
		rset, rset->term->name, rset->hits_limit, rset->hits_count);
#endif
	no++;
    }
    return no;
}

ZEBRA_RES zebra_result_set_term_no(ZebraHandle zh, const char *setname,
				   int *num_terms)
{
    ZebraSet sset = resultSetGet(zh, setname);
    *num_terms = 0;
    if (sset)
    {
	*num_terms = trav_rset_for_termids(sset->rset, 0, 0, 0);
	return ZEBRA_OK;
    }
    return ZEBRA_FAIL;
}

ZEBRA_RES zebra_result_set_term_info(ZebraHandle zh, const char *setname,
				     int no, zint *count, int *approx,
				     char *termbuf, size_t *termlen,
				     const char **term_ref_id)
{
    ZebraSet sset = resultSetGet(zh, setname);
    if (sset)
    {
	int num_terms = trav_rset_for_termids(sset->rset, 0, 0, 0);
	if (no >= 0 && no < num_terms)
	{
	    TERMID *term_array = xmalloc(num_terms * sizeof(*term_array));
	    zint *hits_array = xmalloc(num_terms * sizeof(*hits_array));
	    int *approx_array = xmalloc(num_terms * sizeof(*approx_array));
	    
	    trav_rset_for_termids(sset->rset, term_array,
				  hits_array, approx_array);

	    if (count)
		*count = hits_array[no];
	    if (approx)
		*approx = approx_array[no];
	    if (termbuf)
	    {
		char *inbuf = term_array[no]->name;
		size_t inleft = strlen(inbuf);
		size_t outleft = *termlen - 1;

		if (zh->iconv_from_utf8 != 0)
		{
		    char *outbuf = termbuf;
		    size_t ret;
		    
		    ret = yaz_iconv(zh->iconv_from_utf8, &inbuf, &inleft,
				    &outbuf, &outleft);
		    if (ret == (size_t)(-1))
			*termlen = 0;
		    else
			*termlen = outbuf - termbuf;
		}
		else
		{
		    if (inleft > outleft)
			inleft = outleft;
		    *termlen = inleft;
		    memcpy(termbuf, inbuf, *termlen);
		}
		termbuf[*termlen] = '\0';
	    }
	    if (term_ref_id)
		*term_ref_id = term_array[no]->ref_id;

	    xfree(term_array);
	    xfree(hits_array);
	    xfree(approx_array);
	    return ZEBRA_OK;
	}
    }
    return ZEBRA_FAIL;
}

ZEBRA_RES zebra_snippets_hit_vector(ZebraHandle zh, const char *setname,
				    zint sysno, zebra_snippets *snippets)
{
    ZebraSet sset = resultSetGet(zh, setname);
    yaz_log(YLOG_LOG, "zebra_get_hit_vector setname=%s zysno=" ZINT_FORMAT,
	    setname, sysno);
    if (!sset)
	return ZEBRA_FAIL;
    else
    {
	struct rset_key_control *kc = zebra_key_control_create(zh);
	NMEM nmem = nmem_create();
	struct it_key key;
	RSET rsets[2], rset_comb;
	RSET rset_temp = rstemp_create(nmem, kc, kc->scope, 
				       res_get (zh->res, "setTmpDir"),0 );
	
	TERMID termid;
	RSFD rsfd = rset_open(rset_temp, RSETF_WRITE);
	
	key.mem[0] = sysno;
	key.mem[1] = 0;
	key.mem[2] = 0;
	key.mem[3] = 0;
	key.len = 2;
	rset_write (rsfd, &key);
	rset_close (rsfd);

	rsets[0] = rset_temp;
	rsets[1] = rset_dup(sset->rset);
	
	rset_comb = rsmulti_and_create(nmem, kc, kc->scope, 2, rsets);

	rsfd = rset_open(rset_comb, RSETF_READ);

	while (rset_read(rsfd, &key, &termid))
	{
	    if (termid)
	    {
		struct ord_list *ol;
		for (ol = termid->ol; ol; ol = ol->next)
		{
		    zebra_snippets_append(snippets, key.mem[key.len-1],
					  termid->reg_type,
					  ol->ord, termid->name);
		}
	    }
	}
	rset_close(rsfd);
	
	rset_delete(rset_comb);
	nmem_destroy(nmem);
    }
    return ZEBRA_OK;
}

