/*
 * Copyright (C) 1995-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebraapi.c,v $
 * Revision 1.1  1998-03-05 08:45:13  adam
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
        zebraExplain_close (zh->zei, 0);
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
        return -1;
    if (!(zh->sortIdx = sortIdx_open (zh->bfs, 0)))
	return -1;
    zh->isam = NULL;
    zh->isamc = NULL;
    if (!res_get_match (zh->res, "isam", "i", NULL))
    {
        if (!(zh->isamc = isc_open (zh->bfs, FNAME_ISAMC,
				    0, key_isamc_m(zh->res))))
            return -1;

    }
    else
    {
        if (!(zh->isam = is_open (zh->bfs, FNAME_ISAM, key_compare, 0,
                                  sizeof (struct it_key), zh->res)))
            return -1;
    }
    zh->zei = zebraExplain_open (zh->records, zh->dh, 0);

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

ZebraHandle zebra_open (const char *host, const char *configName)
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
    
    zh->records = NULL;
    zh->registered_sets = NULL;
    zh->zebra_maps = zebra_maps_open (zh->res);
    zh->rank_classes = NULL;
    
    zebraRankInstall (zh, rank1_class);
    return zh;
}

void zebra_close (ZebraHandle zh)
{
    if (zh->records)
    {
        resultSetDestroy (zh);
        zebraExplain_close (zh->zei, 0);
        dict_close (zh->dict);
	sortIdx_close (zh->sortIdx);
        if (zh->isam)
            is_close (zh->isam);
        if (zh->isamc)
            isc_close (zh->isamc);
        rec_close (&zh->records);
        zebra_register_unlock (zh);
    }
    zebra_maps_close (zh->zebra_maps);
    zebraRankDestroy (zh);
    bfs_destroy (zh->bfs);
    data1_destroy (zh->dh);
    zebra_server_lock_destroy (zh);

    res_close (zh->res);
    xfree (zh);
}

void zebra_search_rpn (ZebraHandle zh, ODR stream,
		       Z_RPNQuery *query, int num_bases, char **basenames, 
		       const char *setname)
{
    zebra_register_lock (zh);
    zh->errCode = 0;
    zh->errString = NULL;
    zh->hits = 0;
    rpn_search (zh, stream, query, num_bases, basenames, setname);
    zebra_register_unlock (zh);
}

void zebra_records_retrieve (ZebraHandle zh, ODR stream,
			     const char *setname, Z_RecordComposition *comp,
			     oid_value input_format, int num_recs,
			     ZebraRetrievalRecord *recs)
{
    ZebraPosSet poset;
    int i, *pos_array;

    pos_array = xmalloc (sizeof(*pos_array));
    for (i = 0; i<num_recs; i++)
	pos_array[i] = recs[i].position;

    zebra_register_lock (zh);

    poset = zebraPosSetCreate (zh, setname, num_recs, pos_array);
    if (!poset)
    {
        logf (LOG_DEBUG, "zebraPosSetCreate error");
        zh->errCode = 13;
    }
    else
    {
	for (i = 0; i<num_recs; i++)
	{
	    if (!poset[i].sysno)
	    {
		zh->errCode = 13;
		logf (LOG_DEBUG, "Out of range. pos=%d", pos_array[i]);
	    }
	    else
	    {
		zh->errCode =
		    zebra_record_fetch (zh, poset[i].sysno, poset[i].score,
					stream, input_format, comp,
					&recs[i].format, &recs[i].buf,
					&recs[i].len,
					&recs[i].base);
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
    zebra_register_lock (zh);
    rpn_scan (zh, stream, zapt, attributeset,
	      num_bases, basenames, position,
	      num_entries, entries, is_partial);
    zebra_register_unlock (zh);
}

void zebra_sort (ZebraHandle zh, ODR stream,
		 int num_input_setnames, char **input_setnames,
		 char *output_setname, Z_SortKeySpecList *sort_sequence,
		 int *sort_status)
{
    zebra_register_lock (zh);
    resultSetSort (zh, stream, num_input_setnames, input_setnames,
		   output_setname, sort_sequence, sort_status);
    zebra_register_unlock (zh);
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
