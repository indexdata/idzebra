/*
 * Copyright (C) 1995-1998, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebraapi.c,v $
 * Revision 1.13  1998-12-16 12:23:30  adam
 * Added facility for database name mapping using resource mapdb.
 *
 * Revision 1.12  1998/11/16 10:18:10  adam
 * Better error reporting for result sets.
 *
 * Revision 1.11  1998/10/16 08:14:34  adam
 * Updated record control system.
 *
 * Revision 1.10  1998/09/22 10:03:42  adam
 * Changed result sets to be persistent in the sense that they can
 * be re-searched if needed.
 * Fixed memory leak in rsm_or.
 *
 * Revision 1.9  1998/09/02 13:53:17  adam
 * Extra parameter decode added to search routines to implement
 * persistent queries.
 *
 * Revision 1.8  1998/08/24 17:29:23  adam
 * Minor changes.
 *
 * Revision 1.7  1998/06/24 12:16:13  adam
 * Support for relations on text operands. Open range support in
 * DFA module (i.e. [-j], [g-]).
 *
 * Revision 1.6  1998/06/22 11:36:47  adam
 * Added authentication check facility to zebra.
 *
 * Revision 1.5  1998/06/13 00:14:08  adam
 * Minor changes.
 *
 * Revision 1.4  1998/06/12 12:22:12  adam
 * Work on Zebra API.
 *
 * Revision 1.3  1998/05/27 16:57:44  adam
 * Zebra returns surrogate diagnostic for single records when
 * appropriate.
 *
 * Revision 1.2  1998/05/20 10:12:19  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.1  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 */

#include <stdio.h>
#ifdef WINDOWS
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <diagbib1.h>
#include "zserver.h"

static int zebra_register_lock (ZebraHandle zh)
{
    time_t lastChange;
    int state = zebra_server_lock_get_state(zh, &lastChange);

    switch (state)
    {
    case 'c':
        state = 1;
        break;
    default:
        state = 0;
    }
    zebra_server_lock (zh, state);
#if USE_TIMES
    times (&zh->tms1);
#endif
    if (zh->registerState == state)
    {
        if (zh->registerChange >= lastChange)
            return 0;
        logf (LOG_LOG, "Register completely updated since last access");
    }
    else if (zh->registerState == -1)
        logf (LOG_LOG, "Reading register using state %d pid=%ld", state,
              (long) getpid());
    else
        logf (LOG_LOG, "Register has changed state from %d to %d",
              zh->registerState, state);
    zh->registerChange = lastChange;
    if (zh->records)
    {
        zebraExplain_close (zh->zei, 0, 0);
        dict_close (zh->dict);
	sortIdx_close (zh->sortIdx);
        if (zh->isam)
            is_close (zh->isam);
        if (zh->isamc)
            isc_close (zh->isamc);
        rec_close (&zh->records);
    }
    bf_cache (zh->bfs, state ? res_get (zh->res, "shadow") : NULL);
    zh->registerState = state;
    zh->records = rec_open (zh->bfs, 0);
    if (!(zh->dict = dict_open (zh->bfs, FNAME_DICT, 40, 0)))
    {
	logf (LOG_WARN, "dict_open");
        return -1;
    }
    if (!(zh->sortIdx = sortIdx_open (zh->bfs, 0)))
    {
	logf (LOG_WARN, "sortIdx_open");
	return -1;
    }
    zh->isam = NULL;
    zh->isamc = NULL;
    if (!res_get_match (zh->res, "isam", "i", NULL))
    {
        if (!(zh->isamc = isc_open (zh->bfs, FNAME_ISAMC,
				    0, key_isamc_m(zh->res))))
	{
	    logf (LOG_WARN, "isc_open");
            return -1;
	}

    }
    else
    {
        if (!(zh->isam = is_open (zh->bfs, FNAME_ISAM, key_compare, 0,
                                  sizeof (struct it_key), zh->res)))
	{
	    logf (LOG_WARN, "is_open");
            return -1;
	}
    }
    zh->zei = zebraExplain_open (zh->records, zh->dh, zh->res, 0, 0, 0);

    return 0;
}

static void zebra_register_unlock (ZebraHandle zh)
{
    static int waitSec = -1;

#if USE_TIMES
    times (&zh->tms2);
    logf (LOG_LOG, "user/system: %ld/%ld",
			(long) (zh->tms2.tms_utime - zh->tms1.tms_utime),
			(long) (zh->tms2.tms_stime - zh->tms1.tms_stime));
#endif
    if (waitSec == -1)
    {
        char *s = res_get (zh->res, "debugRequestWait");
        if (s)
            waitSec = atoi (s);
        else
            waitSec = 0;
    }
#ifdef WINDOWS
#else
    if (waitSec > 0)
        sleep (waitSec);
#endif
    if (zh->registerState != -1)
        zebra_server_unlock (zh, zh->registerState);
}

ZebraHandle zebra_open (const char *configName)
{
    ZebraHandle zh = xmalloc (sizeof(*zh));

    if (!(zh->res = res_open (configName)))
    {
	logf (LOG_WARN, "Failed to read resources `%s'", configName);
	return NULL;
    }
    zebra_server_lock_init (zh);
    zh->dh = data1_create ();
    zh->bfs = bfs_create (res_get (zh->res, "register"));
    bf_lockDir (zh->bfs, res_get (zh->res, "lockDir"));
    data1_set_tabpath (zh->dh, res_get(zh->res, "profilePath"));
    zh->sets = NULL;
    zh->registerState = -1;  /* trigger open of registers! */
    zh->registerChange = 0;
    zh->recTypes = recTypes_init (zh->dh);
    recTypes_default_handlers (zh->recTypes);

    zh->records = NULL;
    zh->zebra_maps = zebra_maps_open (zh->res);
    zh->rank_classes = NULL;
    zh->errCode = 0;
    zh->errString = 0;
    
    zebraRankInstall (zh, rank1_class);

    if (!res_get (zh->res, "passwd"))
	zh->passwd_db = NULL;
    else
    {
	zh->passwd_db = passwd_db_open ();
	if (!zh->passwd_db)
	    logf (LOG_WARN|LOG_ERRNO, "passwd_db_open failed");
	else
	    passwd_db_file (zh->passwd_db, res_get (zh->res, "passwd"));
    }
    return zh;
}

void zebra_close (ZebraHandle zh)
{
    if (zh->records)
    {
        resultSetDestroy (zh);
        zebraExplain_close (zh->zei, 0, 0);
        dict_close (zh->dict);
	sortIdx_close (zh->sortIdx);
        if (zh->isam)
            is_close (zh->isam);
        if (zh->isamc)
            isc_close (zh->isamc);
        rec_close (&zh->records);
        zebra_register_unlock (zh);
    }
    recTypes_destroy (zh->recTypes);
    zebra_maps_close (zh->zebra_maps);
    zebraRankDestroy (zh);
    bfs_destroy (zh->bfs);
    data1_destroy (zh->dh);
    zebra_server_lock_destroy (zh);

    if (zh->passwd_db)
	passwd_db_close (zh->passwd_db);
    res_close (zh->res);
    xfree (zh);
}

struct map_baseinfo {
    ZebraHandle zh;
    NMEM mem;
    int num_bases;
    char **basenames;
    int new_num_bases;
    char **new_basenames;
    int new_num_max;
};
	
void map_basenames_func (void *vp, const char *name, const char *value)
{
    struct map_baseinfo *p = vp;
    int i, no;
    char fromdb[128], todb[8][128];
    
    no =
	sscanf (value, "%127s %127s %127s %127s %127s %127s %127s %127s %127s",
		fromdb,	todb[0], todb[1], todb[2], todb[3], todb[4],
		todb[5], todb[6], todb[7]);
    if (no < 2)
	return ;
    no--;
    for (i = 0; i<p->num_bases; i++)
	if (p->basenames[i] && !strcmp (p->basenames[i], fromdb))
	{
	    p->basenames[i] = 0;
	    for (i = 0; i < no; i++)
	    {
		if (p->new_num_bases == p->new_num_max)
		    return;
		p->new_basenames[(p->new_num_bases)++] = 
		    nmem_strdup (p->mem, todb[i]);
	    }
	    return;
	}
}

void map_basenames (ZebraHandle zh, ODR stream,
		    int *num_bases, char ***basenames)
{
    struct map_baseinfo info;
    struct map_baseinfo *p = &info;
    int i;

    info.zh = zh;
    info.num_bases = *num_bases;
    info.basenames = *basenames;
    info.new_num_max = 128;
    info.new_num_bases = 0;
    info.new_basenames = (char **)
	odr_malloc (stream, sizeof(*info.new_basenames) * info.new_num_max);
    info.mem = stream->mem;

    res_trav (zh->res, "mapdb", &info, map_basenames_func);
    
    for (i = 0; i<p->num_bases; i++)
	if (p->basenames[i] && p->new_num_bases < p->new_num_max)
	{
	    p->new_basenames[(p->new_num_bases)++] = 
		nmem_strdup (p->mem, p->basenames[i]);
	}
    *num_bases = info.new_num_bases;
    *basenames = info.new_basenames;
    for (i = 0; i<*num_bases; i++)
	logf (LOG_LOG, "base %s", (*basenames)[i]);
}

void zebra_search_rpn (ZebraHandle zh, ODR stream, ODR decode,
		       Z_RPNQuery *query, int num_bases, char **basenames, 
		       const char *setname)
{
    zebra_register_lock (zh);
    zh->errCode = 0;
    zh->errString = NULL;
    zh->hits = 0;

    map_basenames (zh, stream, &num_bases, &basenames);
    resultSetAddRPN (zh, stream, decode, query, num_bases, basenames, setname);

    zebra_register_unlock (zh);
}

void zebra_records_retrieve (ZebraHandle zh, ODR stream,
			     const char *setname, Z_RecordComposition *comp,
			     oid_value input_format, int num_recs,
			     ZebraRetrievalRecord *recs)
{
    ZebraPosSet poset;
    int i, *pos_array;

    zh->errCode = 0;
    zh->errString = NULL;
    pos_array = xmalloc (num_recs * sizeof(*pos_array));
    for (i = 0; i<num_recs; i++)
	pos_array[i] = recs[i].position;

    zebra_register_lock (zh);

    poset = zebraPosSetCreate (zh, setname, num_recs, pos_array);
    if (!poset)
    {
        logf (LOG_DEBUG, "zebraPosSetCreate error");
        zh->errCode = 30;
        zh->errString = nmem_strdup (stream->mem, setname);
    }
    else
    {
	for (i = 0; i<num_recs; i++)
	{
	    if (!poset[i].sysno)
	    {
	        char num_str[20];

		sprintf (num_str, "%d", pos_array[i]);	
		zh->errCode = 13;
                zh->errString = nmem_strdup (stream->mem, num_str);
                break;
	    }
	    else
	    {
		recs[i].errCode =
		    zebra_record_fetch (zh, poset[i].sysno, poset[i].score,
					stream, input_format, comp,
					&recs[i].format, &recs[i].buf,
					&recs[i].len,
					&recs[i].base);
		recs[i].errString = NULL;
	    }
	}
	zebraPosSetDestroy (zh, poset, num_recs);
    }
    zebra_register_unlock (zh);
    xfree (pos_array);
}

void zebra_scan (ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
		 oid_value attributeset,
		 int num_bases, char **basenames,
		 int *position, int *num_entries, ZebraScanEntry **entries,
		 int *is_partial)
{
    zh->errCode = 0;
    zh->errString = NULL;
    zebra_register_lock (zh);
    map_basenames (zh, stream, &num_bases, &basenames);
    rpn_scan (zh, stream, zapt, attributeset,
	      num_bases, basenames, position,
	      num_entries, entries, is_partial);
    zebra_register_unlock (zh);
}

void zebra_sort (ZebraHandle zh, ODR stream,
		 int num_input_setnames, const char **input_setnames,
		 const char *output_setname, Z_SortKeySpecList *sort_sequence,
		 int *sort_status)
{
    zh->errCode = 0;
    zh->errString = NULL;
    zebra_register_lock (zh);
    resultSetSort (zh, stream->mem, num_input_setnames, input_setnames,
		   output_setname, sort_sequence, sort_status);
    zebra_register_unlock (zh);
}

int zebra_errCode (ZebraHandle zh)
{
    return zh->errCode;
}

const char *zebra_errString (ZebraHandle zh)
{
    return diagbib1_str (zh->errCode);
}

char *zebra_errAdd (ZebraHandle zh)
{
    return zh->errString;
}

int zebra_hits (ZebraHandle zh)
{
    return zh->hits;
}

int zebra_auth (ZebraHandle zh, const char *user, const char *pass)
{
    if (!zh->passwd_db || !passwd_db_auth (zh->passwd_db, user, pass))
	return 0;
    return 1;
}

void zebra_setDB (ZebraHandle zh, int num_bases, char **basenames)
{

}

void zebra_setRecordType (ZebraHandle zh, const char *type)
{

}

void zebra_setGroup (ZebraHandle zh, const char *group)
{

}

void zebra_admin (ZebraHandle zh, const char *command)
{

}
