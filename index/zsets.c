/* $Id: zsets.c,v 1.74 2004-12-10 11:56:22 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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
static int log_level_sorting=0;
static int log_level_searchhits=0;
static int log_level_searchterms=0;
static int log_level_resultsets=0;

static void loglevels()
{
    if (log_level_set)
        return;
    log_level_sorting = yaz_log_module_level("sorting");
    log_level_searchhits = yaz_log_module_level("searchhits");
    log_level_searchterms = yaz_log_module_level("searchterms");
    log_level_resultsets = yaz_log_module_level("resultsets");
}

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
    zebraSet->rset_nmem=nmem_create(); 

    zebraSet->num_bases = num_bases;
    zebraSet->basenames = 
        nmem_malloc (zebraSet->nmem, num_bases * sizeof(*zebraSet->basenames));
    for (i = 0; i<num_bases; i++)
        zebraSet->basenames[i] = nmem_strdup (zebraSet->nmem, basenames[i]);


    zebraSet->rset = rpn_search (zh, zebraSet->nmem, zebraSet->rset_nmem,
                                 rpn, zebraSet->num_bases,
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

ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov)
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
            rset_delete (s->rset);
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
    s->rset_nmem=0;
    s->nmem = 0;
    s->rpn = 0;
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
                yaz_log(log_level_resultsets, "research %s", name);
                if (!s->rset_nmem)
                    s->rset_nmem=nmem_create();
                s->rset =
                    rpn_search (zh, nmem, s->rset_nmem, s->rpn, s->num_bases,
                                s->basenames, s->name, s);
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
            rset_delete (s->rset);
        s->rset = 0;
        if (s->rset_nmem)
            nmem_destroy(s->rset_nmem);
        s->rset_nmem=0;
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
                rset_delete (s->rset);
            if (s->rset_nmem)
                nmem_destroy(s->rset_nmem);
            xfree (s->name);
            xfree (s);
        }
        else
            ss = &s->next;
    }
}

ZebraMetaRecord *zebra_meta_records_create_range (ZebraHandle zh,
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

ZebraMetaRecord *zebra_meta_records_create (ZebraHandle zh, const char *name, 
					    int num, zint *positions)
{
    ZebraSet sset;
    ZebraMetaRecord *sr = 0;
    RSET rset;
    int i;
    struct zset_sort_info *sort_info;

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
            int position;
            
            for (i = 0; i<num; i++)
            {
                position = positions[i];
                if (position > 0 && position <= sort_info->num_entries)
                {
                    yaz_log(log_level_sorting, "got pos=%d (sorted)", position);
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
            zint psysno = 0;
            RSFD rfd;
            struct it_key key;
            
            if (sort_info)
                position = sort_info->num_entries;
            while (num_i < num && positions[num_i] < position)
                num_i++;
            rfd = rset_open (rset, RSETF_READ);
            while (num_i < num && rset_read (rfd, &key, 0))
            {
                zint this_sys = key.mem[0];
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
                        yaz_log(log_level_sorting, "got pos=%d (unsorted)", position);
                        sr[num_i].score = -1;
                        num_i++;
                    }
                }
            }
            rset_close (rfd);
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
    if (!log_level_set)
        loglevels();
    yaz_log(log_level_sorting, "result set sort input=%s output=%s",
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
    {
        rset = rset_dup (rset);
        sset = resultSetAdd (zh, output_setname, 1);
        sset->rset = rset;
    }
    resultSetSortSingle (zh, nmem, sset, rset, sort_sequence, sort_status);
}

void resultSetSortSingle (ZebraHandle zh, NMEM nmem,
                          ZebraSet sset, RSET rset,
                          Z_SortKeySpecList *sort_sequence, int *sort_status)
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
            yaz_log(log_level_sorting, "Sort: key %d is of type sortField", i+1);
            zh->errCode = 207;
            return;
        case Z_SortKey_elementSpec:
            yaz_log(log_level_sorting, "Sort: key %d is of type elementSpec", i+1);
            zh->errCode = 207;
            return;
        case Z_SortKey_sortAttributes:
            yaz_log(log_level_sorting, "Sort: key %d is of type sortAttributes", i+1);
            sort_criteria[i].attrUse =
                zebra_maps_sort (zh->reg->zebra_maps,
                                 sk->u.sortAttributes,
                                 &sort_criteria[i].numerical);
            yaz_log(log_level_sorting, "use value = %d", sort_criteria[i].attrUse);
            if (sort_criteria[i].attrUse == -1)
            {
                zh->errCode = 116;
                return;
            }
            if (sortIdx_type (zh->reg->sortIdx, sort_criteria[i].attrUse))
            {
                zh->errCode = 207;
                return;
            }
            break;
        }
    }
    rfd = rset_open (rset, RSETF_READ);
    while (rset_read (rfd, &key, &termid))
      /* FIXME - pass a TERMID *, and use it for something below !! */
    {
        zint this_sys = key.mem[0];
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
    yaz_log(log_level_sorting, ZINT_FORMAT " keys, " ZINT_FORMAT " sysnos, sort",
		    kno, sset->hits);   
    for (i = 0; i < numTerms; i++)
        yaz_log(log_level_sorting, "term=\"%s\" type=%s count=" ZINT_FORMAT,
                 terms[i]->name, terms[i]->flags, rset_count(terms[i]->rset));
    *sort_status = Z_SortResponse_success;
}

RSET resultSetRef (ZebraHandle zh, const char *resultSetId)
{
    ZebraSet s;

    if ((s = resultSetGet (zh, resultSetId)))
        return s->rset;
    return NULL;
}

void resultSetRank (ZebraHandle zh, ZebraSet zebraSet, RSET rset, NMEM nmem)
{
    zint kno = 0;
    struct it_key key;
    RSFD rfd;
    TERMID termid;
    TERMID *terms;
    int numTerms = 0;
    int n = 0;
    int i;
    ZebraRankClass rank_class;
    struct rank_control *rc;
    struct zset_sort_info *sort_info;
    const char *rank_handler_name = res_get_def(zh->res, "rank", "rank-1");
    double cur,tot; 
    zint est=-2; /* -2 not done, -1 can't do, >0 actual estimate*/
    zint esthits;
    double ratio;

    if (!log_level_set)
        loglevels();
    sort_info = zebraSet->sort_info;
    sort_info->num_entries = 0;
    zebraSet->hits = 0;
    rset_getterms(rset, 0, 0, &n);
    terms = (TERMID *) nmem_malloc(nmem, sizeof(*terms)*n);
    rset_getterms(rset, terms, n, &numTerms);

    rfd = rset_open (rset, RSETF_READ);

    rank_class = zebraRankLookup (zh, rank_handler_name);
    if (!rank_class)
    {
        yaz_log(YLOG_WARN, "No such rank handler: %s", rank_handler_name);
        return;
    }
    rc = rank_class->control;

    if (rset_read (rfd, &key, &termid))
    {
        zint psysno = key.mem[0];
        int score;
        void *handle =
            (*rc->begin) (zh->reg, rank_class->class_handle, rset, nmem,
                          terms, numTerms);
        (zebraSet->hits)++;
        esthits = atoi(res_get_def(zh->res, "estimatehits","0"));
        if (!esthits) 
            est = -1; /* can not do */
        do
        {
            zint this_sys = key.mem[0]; /* FIXME - assumes scope==2 */
            zint seqno = key.mem[key.len-1]; /* FIXME - assumes scope==2 */
            kno++;
            key_logdump_txt(log_level_searchhits,&key," Got hit");
            if (this_sys != psysno)
            {
                score = (*rc->calc) (handle, psysno);

                resultSetInsertRank (zh, sort_info, psysno, score, 'A');
                (zebraSet->hits)++;
                psysno = this_sys;
            }
            (*rc->add) (handle, seqno, termid);
            
            if ((est==-2) && (zebraSet->hits==esthits))
            { /* time to estimate the hits */
                rset_pos(rfd,&cur,&tot); 
                if (tot>0) {
                    ratio = cur/tot;
                    est = (zint)(0.5+zebraSet->hits/ratio);
                    yaz_log(log_level_searchhits, "Estimating hits (%s) "
                                  "%0.1f->" ZINT_FORMAT
                                  "; %0.1f->" ZINT_FORMAT,
                                  rset->control->desc,
                                  cur, zebraSet->hits,
                                  tot, est);
                    i = 0; /* round to 3 significant digits */
                    while (est>1000) {
                        est /= 10;
                        i++;
                    }
                    while (i--)
			est *= 10;
                    zebraSet->hits = est;
                }
            }
        }
        while (rset_read (rfd, &key,&termid) && (est<0) );
        score = (*rc->calc)(handle, psysno);
        resultSetInsertRank(zh, sort_info, psysno, score, 'A');
        (*rc->end) (zh->reg, handle);
    }
    rset_close (rfd);

    yaz_log(log_level_searchterms, ZINT_FORMAT " keys, " ZINT_FORMAT " sysnos, rank",
	    kno, zebraSet->hits);
    for (i = 0; i < numTerms; i++)
    {
        yaz_log(log_level_searchterms, "term=\"%s\" type=%s count=" ZINT_FORMAT,
                 terms[i]->name, terms[i]->flags, rset_count(terms[i]->rset));
    }
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
        xfree(p->control->name);
        xfree(p->control);
        xfree(p);
        p = p_next;
    }
    reg->rank_classes = NULL;
}
