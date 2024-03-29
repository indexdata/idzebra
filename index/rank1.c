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
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "index.h"
#include "rank.h"

static int log_level = 0;
static int log_initialized = 0;

struct rank_class_info {
    int dummy;
};

struct rank_term_info {
    int local_occur;
    zint global_occur;
    int global_inv;
    int rank_flag;
    int rank_weight;
    TERMID term;
    int term_index;
};

struct rank_set_info {
    int last_pos;
    int no_entries;
    int no_rank_entries;
    struct rank_term_info *entries;
    NMEM nmem;
};

static int log2_int(zint g)
{
    int n = 0;
    if (g < 0)
        return 0;
    while ((g = g>>1))
        n++;
    return n;
}

/*
 * create: Creates/Initialises this rank handler. This routine is
 *  called exactly once. The routine returns the class_handle.
 */
static void *create(ZebraHandle zh)
{
    struct rank_class_info *ci =
        (struct rank_class_info *) xmalloc(sizeof(*ci));

    if (!log_initialized)
    {
        log_level = yaz_log_module_level("rank1");
        log_initialized = 1;
    }
    yaz_log(log_level, "rank-1 create");
    return ci;
}

/*
 * destroy: Destroys this rank handler. This routine is called
 *  when the handler is no longer needed - i.e. when the server
 *  dies. The class_handle was previously returned by create.
 */
static void destroy(struct zebra_register *reg, void *class_handle)
{
    struct rank_class_info *ci = (struct rank_class_info *) class_handle;

    yaz_log(log_level, "rank-1 destroy");
    xfree(ci);
}


/**
 * begin: Prepares beginning of "real" ranking. Called once for
 *  each result set. The returned handle is a "set handle" and
 *  will be used in each of the handlers below.
 */
static void *begin(struct zebra_register *reg,
                   void *class_handle, RSET rset, NMEM nmem,
                   TERMID *terms, int numterms)
{
    struct rank_set_info *si =
        (struct rank_set_info *) nmem_malloc(nmem,sizeof(*si));
    int i;

    yaz_log(log_level, "rank-1 begin");
    si->no_entries = numterms;
    si->no_rank_entries = 0;
    si->nmem=nmem;
    si->entries = (struct rank_term_info *)
        nmem_malloc(si->nmem, sizeof(*si->entries)*numterms);
    for (i = 0; i < numterms; i++)
    {
        zint g = rset_count(terms[i]->rset);
        yaz_log(log_level, "i=%d flags=%s '%s'", i,
                terms[i]->flags, terms[i]->name );
        if  (!strncmp(terms[i]->flags, "rank,", 5))
        {
            const char *cp = strstr(terms[i]->flags+4, ",w=");
            si->entries[i].rank_flag = 1;
            if (cp)
                si->entries[i].rank_weight = atoi(cp+3);
            else
              si->entries[i].rank_weight = 34; /* sqrroot of 1000 */
            yaz_log(log_level, " i=%d weight=%d g="ZINT_FORMAT, i,
                     si->entries[i].rank_weight, g);
            (si->no_rank_entries)++;
        }
        else
            si->entries[i].rank_flag = 0;
        si->entries[i].local_occur = 0;  /* FIXME */
        si->entries[i].global_occur = g;
        si->entries[i].global_inv = 32 - log2_int(g);
        yaz_log(log_level, " global_inv = %d g = " ZINT_FORMAT,
                (int) (32-log2_int(g)), g);
        si->entries[i].term = terms[i];
        si->entries[i].term_index=i;
        terms[i]->rankpriv = &(si->entries[i]);
    }
    return si;
}

/*
 * end: Terminates ranking process. Called after a result set
 *  has been ranked.
 */
static void end(struct zebra_register *reg, void *set_handle)
{
    yaz_log(log_level, "rank-1 end");
    /* no need to free anything, they are in nmems */
}


/**
 * add: Called for each word occurence in a result set. This routine
 *  should be as fast as possible. This routine should "incrementally"
 *  update the score.
 */
static void add(void *set_handle, int seqno, TERMID term)
{
    struct rank_set_info *si = (struct rank_set_info *) set_handle;
    struct rank_term_info *ti;
    assert(si);
    if (!term)
    {
        yaz_log(log_level, "rank-1 add NULL term");
        return;
    }
    ti= (struct rank_term_info *) term->rankpriv;
    assert(ti);
    si->last_pos = seqno;
    ti->local_occur++;
    yaz_log(log_level, "rank-1 add seqno=%d term=%s count=%d",
            seqno, term->name,ti->local_occur);
}

/*
 * calc: Called for each document in a result. This handler should
 *  produce a score based on previous call(s) to the add handler. The
 *  score should be between 0 and 1000. If score cannot be obtained
 *  -1 should be returned.
 */
static int calc_1(void *set_handle, zint sysno, zint staticrank,
                  int *stop_flag)
{
    int i, lo, divisor, score = 0;
    struct rank_set_info *si = (struct rank_set_info *) set_handle;

    if (!si->no_rank_entries)
        return -1;   /* ranking not enabled for any terms */

    for (i = 0; i < si->no_entries; i++)
    {
        yaz_log(log_level, "calc: i=%d rank_flag=%d lo=%d",
                i, si->entries[i].rank_flag, si->entries[i].local_occur);
        if (si->entries[i].rank_flag && (lo = si->entries[i].local_occur))
            score += (8+log2_int (lo)) * si->entries[i].global_inv *
                si->entries[i].rank_weight;
    }
    divisor = si->no_rank_entries * (8+log2_int(si->last_pos/si->no_entries));
    score = score / divisor;
    yaz_log(log_level, "calc sysno=" ZINT_FORMAT " score=%d", sysno, score);
    if (score > 1000)
        score = 1000;
    /* reset the counts for the next term */
    for (i = 0; i < si->no_entries; i++)
        si->entries[i].local_occur = 0;
    return score;
}

static int calc_2(void *set_handle, zint sysno, zint staticrank,
                  int *stop_flag)
{
    int score = calc_1(set_handle, sysno, staticrank, stop_flag);
    return score - staticrank;
}

/*
 * Pseudo-meta code with sequence of calls as they occur in a
 * server. Handlers are prefixed by --:
 *
 *     server init
 *     -- create
 *     foreach search
 *        rank result set
 *        -- begin
 *        foreach record
 *           foreach word
 *              -- add
 *           -- calc
 *        -- end
 *     -- destroy
 *     server close
 */

static struct rank_control rank_1_control = {
    "rank-1",
    create,
    destroy,
    begin,
    end,
    calc_1,
    add,
};
struct rank_control *rank_1_class = &rank_1_control;

static struct rank_control rank_2_control = {
    "rank-2",
    create,
    destroy,
    begin,
    end,
    calc_2,
    add,
};

struct rank_control *rank_2_class = &rank_2_control;
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

