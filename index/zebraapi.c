/*
 * Copyright (C) 1995-2002, Index Data
 * All rights reserved.
 *
 * $Id: zebraapi.c,v 1.46 2002-02-20 23:07:54 adam Exp $
 */

#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <yaz/diagbib1.h>
#include "zserver.h"
#include <charmap.h>

static void zebra_chdir (ZebraService zh)
{
    const char *dir = res_get (zh->res, "chdir");
    if (!dir)
	return;
    logf (LOG_DEBUG, "chdir %s", dir);
#ifdef WIN32
    _chdir(dir);
#else
    chdir (dir);
#endif
}

static void zebra_flush_reg (ZebraHandle zh)
{
    zebraExplain_flush (zh->service->zei, 1, zh);
    
    extract_flushWriteKeys (zh);
    zebra_index_merge (zh);
}


static int zebra_register_activate (ZebraHandle zh, int rw, int useshadow);
static int zebra_register_deactivate (ZebraHandle zh);

static int zebra_begin_read (ZebraHandle zh);
static void zebra_end_read (ZebraHandle zh);

ZebraHandle zebra_open (ZebraService zs)
{
    ZebraHandle zh;

    assert (zs);
    if (zs->stop_flag)
	return 0;

    zh = (ZebraHandle) xmalloc (sizeof(*zh));
    yaz_log (LOG_LOG, "zebra_open zs=%p returns %p", zs, zh);

    zh->service = zs;
    zh->sets = 0;
    zh->destroyed = 0;
    zh->errCode = 0;
    zh->errString = 0;

    zh->trans_no = 0;
    zh->seqno = 0;
    zh->last_val = 0;

    zh->lock_normal = zebra_lock_create ("norm.LCK", 0);
    zh->lock_shadow = zebra_lock_create ("shadow.LCK", 0);

    zh->key_buf = 0;
    zh->admin_databaseName = 0;

    zh->keys.buf_max = 0;
    zh->keys.buf = 0;

    zebra_mutex_cond_lock (&zs->session_lock);

    zh->next = zs->sessions;
    zs->sessions = zh;

    zebra_mutex_cond_unlock (&zs->session_lock);

    return zh;
}


ZebraService zebra_start (const char *configName)
{
    ZebraService zh = xmalloc (sizeof(*zh));

    yaz_log (LOG_LOG, "zebra_start %s", configName);

    zh->configName = xstrdup(configName);
    zh->sessions = 0;
    zh->stop_flag = 0;
    zh->active = 1;

    zh->registerState = -1;
    zh->registerChange = 0;

    if (!(zh->res = res_open (zh->configName)))
    {
	logf (LOG_WARN, "Failed to read resources `%s'", zh->configName);
//	return zh;
    }
    zebra_chdir (zh);

    zebra_mutex_cond_init (&zh->session_lock);
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

static int zebra_register_activate (ZebraHandle zh, int rw, int useshadow)
{
    ZebraService zs = zh->service;
    int record_compression = REC_COMPRESS_NONE;
    char *recordCompression = 0;

    yaz_log (LOG_LOG, "zebra_open_register_activate rw = %d useshadow=%d",
             rw, useshadow);

    zs->dh = data1_create ();
    if (!zs->dh)
        return -1;
    zs->bfs = bfs_create (res_get (zs->res, "register"));
    if (!zs->bfs)
    {
        data1_destroy(zs->dh);
        return -1;
    }
    bf_lockDir (zs->bfs, res_get (zs->res, "lockDir"));
    if (useshadow)
        bf_cache (zs->bfs, res_get (zs->res, "shadow"));
    data1_set_tabpath (zs->dh, res_get(zs->res, "profilePath"));
    zs->recTypes = recTypes_init (zs->dh);
    recTypes_default_handlers (zs->recTypes);

    zs->zebra_maps = zebra_maps_open (zs->res);
    zs->rank_classes = NULL;

    zs->records = 0;
    zs->dict = 0;
    zs->sortIdx = 0;
    zs->isams = 0;
    zs->matchDict = 0;
#if ZMBOL
    zs->isam = 0;
    zs->isamc = 0;
    zs->isamd = 0;
#endif
    zs->zei = 0;
    zs->matchDict = 0;
    
    zebraRankInstall (zs, rank1_class);

    recordCompression = res_get_def (zh->service->res,
				     "recordCompression", "none");
    if (!strcmp (recordCompression, "none"))
	record_compression = REC_COMPRESS_NONE;
    if (!strcmp (recordCompression, "bzip2"))
	record_compression = REC_COMPRESS_BZIP2;

    if (!(zs->records = rec_open (zs->bfs, rw, record_compression)))
    {
	logf (LOG_WARN, "rec_open");
	return -1;
    }
    if (rw)
    {
        zs->matchDict = dict_open (zs->bfs, GMATCH_DICT, 50, 1, 0);
    }
    if (!(zs->dict = dict_open (zs->bfs, FNAME_DICT, 80, rw, 0)))
    {
	logf (LOG_WARN, "dict_open");
	return -1;
    }
    if (!(zs->sortIdx = sortIdx_open (zs->bfs, rw)))
    {
	logf (LOG_WARN, "sortIdx_open");
	return -1;
    }
    if (res_get_match (zs->res, "isam", "s", ISAM_DEFAULT))
    {
	struct ISAMS_M_s isams_m;
	if (!(zs->isams = isams_open (zs->bfs, FNAME_ISAMS, rw,
				      key_isams_m(zs->res, &isams_m))))
	{
	    logf (LOG_WARN, "isams_open");
	    return -1;
	}
    }
#if ZMBOL
    else if (res_get_match (zs->res, "isam", "i", ISAM_DEFAULT))
    {
	if (!(zs->isam = is_open (zs->bfs, FNAME_ISAM, key_compare, rw,
				  sizeof (struct it_key), zs->res)))
	{
	    logf (LOG_WARN, "is_open");
	    return -1;
	}
    }
    else if (res_get_match (zs->res, "isam", "c", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
	if (!(zs->isamc = isc_open (zs->bfs, FNAME_ISAMC,
				    rw, key_isamc_m(zs->res, &isamc_m))))
	{
	    logf (LOG_WARN, "isc_open");
	    return -1;
	}
    }
    else if (res_get_match (zs->res, "isam", "d", ISAM_DEFAULT))
    {
	struct ISAMD_M_s isamd_m;
	
	if (!(zs->isamd = isamd_open (zs->bfs, FNAME_ISAMD,
				      rw, key_isamd_m(zs->res, &isamd_m))))
	{
	    logf (LOG_WARN, "isamd_open");
	    return -1;
	}
    }
#endif
    zs->zei = zebraExplain_open (zs->records, zs->dh,
				 zs->res, rw, zh,
				 explain_extract);
    if (!zs->zei)
    {
	logf (LOG_WARN, "Cannot obtain EXPLAIN information");
	return -1;
    }
    zs->active = 2;
    yaz_log (LOG_LOG, "zebra_register_activate ok");
    return 0;
}

void zebra_admin_shutdown (ZebraHandle zh)
{
    zebra_mutex_cond_lock (&zh->service->session_lock);
    zh->service->stop_flag = 1;
    if (!zh->service->sessions)
	zebra_register_deactivate(zh);
    zh->service->active = 0;
    zebra_mutex_cond_unlock (&zh->service->session_lock);
}

void zebra_admin_start (ZebraHandle zh)
{
    ZebraService zs = zh->service;
    zh->errCode = 0;
    zebra_mutex_cond_lock (&zs->session_lock);
    if (!zs->stop_flag)
	zh->service->active = 1;
    zebra_mutex_cond_unlock (&zs->session_lock);
}

static int zebra_register_deactivate (ZebraHandle zh)
{
    ZebraService zs = zh->service;
    zs->stop_flag = 0;
    if (zs->active <= 1)
    {
	yaz_log(LOG_LOG, "zebra_register_deactivate (ignored since active=%d)",
		zs->active);
	return 0;
    }
    yaz_log(LOG_LOG, "zebra_register_deactivate");
    zebra_chdir (zs);
    if (zs->records)
    {
        zebraExplain_close (zs->zei, 0);
        dict_close (zs->dict);
        if (zs->matchDict)
            dict_close (zs->matchDict);
	sortIdx_close (zs->sortIdx);
	if (zs->isams)
	    isams_close (zs->isams);
#if ZMBOL
        if (zs->isam)
            is_close (zs->isam);
        if (zs->isamc)
            isc_close (zs->isamc);
        if (zs->isamd)
            isamd_close (zs->isamd);
#endif
        rec_close (&zs->records);
    }
    resultSetInvalidate (zh);

    recTypes_destroy (zs->recTypes);
    zebra_maps_close (zs->zebra_maps);
    zebraRankDestroy (zs);
    bfs_destroy (zs->bfs);
    data1_destroy (zs->dh);

    if (zs->passwd_db)
	passwd_db_close (zs->passwd_db);
    zs->active = 1;
    return 0;
}

void zebra_stop(ZebraService zs)
{
    if (!zs)
	return ;
    yaz_log (LOG_LOG, "zebra_stop");

    zebra_mutex_cond_lock (&zs->session_lock);
    while (zs->sessions)
    {
        zebra_register_deactivate(zs->sessions);
        zebra_close (zs->sessions);
    }
        
    zebra_mutex_cond_unlock (&zs->session_lock);

    zebra_mutex_cond_destroy (&zs->session_lock);

    res_close (zs->res);
    xfree (zs->configName);
    xfree (zs);
}

void zebra_close (ZebraHandle zh)
{
    ZebraService zs;
    struct zebra_session **sp;

    if (!zh)
        return;

    zs = zh->service;
    yaz_log (LOG_LOG, "zebra_close zh=%p", zh);
    if (!zh)
	return ;
    resultSetDestroy (zh, -1, 0, 0);

    if (zh->key_buf)
    {
	xfree (zh->key_buf);
	zh->key_buf = 0;
    }
    
    xfree (zh->admin_databaseName);
    zebra_mutex_cond_lock (&zs->session_lock);
    zebra_lock_destroy (zh->lock_normal);
    zebra_lock_destroy (zh->lock_shadow);
    sp = &zs->sessions;
    while (1)
    {
	assert (*sp);
	if (*sp == zh)
	{
	    *sp = (*sp)->next;
	    break;
	}
	sp = &(*sp)->next;
    }
//    if (!zs->sessions && zs->stop_flag)
//	zebra_register_deactivate(zs);
    zebra_mutex_cond_unlock (&zs->session_lock);
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
    struct map_baseinfo *p = (struct map_baseinfo *) vp;
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

    res_trav (zh->service->res, "mapdb", &info, map_basenames_func);
    
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
    zh->hits = 0;
    if (zebra_begin_read (zh))
	return;
    map_basenames (zh, stream, &num_bases, &basenames);
    resultSetAddRPN (zh, stream, decode, query, num_bases, basenames, setname);

    zebra_end_read (zh);

    logf(LOG_APP,"SEARCH:%d:",zh->hits);
}

void zebra_records_retrieve (ZebraHandle zh, ODR stream,
			     const char *setname, Z_RecordComposition *comp,
			     oid_value input_format, int num_recs,
			     ZebraRetrievalRecord *recs)
{
    ZebraPosSet poset;
    int i, *pos_array;

    if (zebra_begin_read (zh))
	return;

    pos_array = (int *) xmalloc (num_recs * sizeof(*pos_array));
    for (i = 0; i<num_recs; i++)
	pos_array[i] = recs[i].position;
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
	    if (poset[i].term)
	    {
		recs[i].errCode = 0;
		recs[i].format = VAL_SUTRS;
		recs[i].len = strlen(poset[i].term);
		recs[i].buf = poset[i].term;
		recs[i].base = poset[i].db;
	    }
	    else if (poset[i].sysno)
	    {
		recs[i].errCode =
		    zebra_record_fetch (zh, poset[i].sysno, poset[i].score,
					stream, input_format, comp,
					&recs[i].format, &recs[i].buf,
					&recs[i].len,
					&recs[i].base);
		recs[i].errString = NULL;
	    }
	    else
	    {
	        char num_str[20];

		sprintf (num_str, "%d", pos_array[i]);	
		zh->errCode = 13;
                zh->errString = nmem_strdup (stream->mem, num_str);
                break;
	    }
	}
	zebraPosSetDestroy (zh, poset, num_recs);
    }
    zebra_end_read (zh);
    xfree (pos_array);
}

void zebra_scan (ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
		 oid_value attributeset,
		 int num_bases, char **basenames,
		 int *position, int *num_entries, ZebraScanEntry **entries,
		 int *is_partial)
{
    if (zebra_begin_read (zh))
    {
	*entries = 0;
	*num_entries = 0;
	return;
    }
    map_basenames (zh, stream, &num_bases, &basenames);
    rpn_scan (zh, stream, zapt, attributeset,
	      num_bases, basenames, position,
	      num_entries, entries, is_partial);
    zebra_end_read (zh);
}

void zebra_sort (ZebraHandle zh, ODR stream,
		 int num_input_setnames, const char **input_setnames,
		 const char *output_setname, Z_SortKeySpecList *sort_sequence,
		 int *sort_status)
{
    if (zebra_begin_read (zh))
	return;
    resultSetSort (zh, stream->mem, num_input_setnames, input_setnames,
		   output_setname, sort_sequence, sort_status);
    zebra_end_read(zh);
}

int zebra_deleleResultSet(ZebraHandle zh, int function,
			  int num_setnames, char **setnames,
			  int *statuses)
{
    int i, status;
    if (zebra_begin_read(zh))
	return Z_DeleteStatus_systemProblemAtTarget;
    switch (function)
    {
    case Z_DeleteRequest_list:
	resultSetDestroy (zh, num_setnames, setnames, statuses);
	break;
    case Z_DeleteRequest_all:
	resultSetDestroy (zh, -1, 0, statuses);
	break;
    }
    zebra_end_read (zh);
    status = Z_DeleteStatus_success;
    for (i = 0; i<num_setnames; i++)
	if (statuses[i] == Z_DeleteStatus_resultSetDidNotExist)
	    status = statuses[i];
    return status;
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

int zebra_auth (ZebraService zh, const char *user, const char *pass)
{
    if (!zh->passwd_db || !passwd_db_auth (zh->passwd_db, user, pass))
    {
        logf(LOG_APP,"AUTHOK:%s", user?user:"ANONYMOUS");
	return 0;
    }

    logf(LOG_APP,"AUTHFAIL:%s", user?user:"ANONYMOUS");
    return 1;
}

void zebra_admin_import_begin (ZebraHandle zh, const char *database)
{
    zebra_begin_trans (zh);
    xfree (zh->admin_databaseName);
    zh->admin_databaseName = xstrdup(database);
}

void zebra_admin_import_end (ZebraHandle zh)
{
    zebra_end_trans (zh);
}

void zebra_admin_import_segment (ZebraHandle zh, Z_Segment *segment)
{
    int sysno;
    int i;
    if (zh->service->active < 2)
	return;
    for (i = 0; i<segment->num_segmentRecords; i++)
    {
	Z_NamePlusRecord *npr = segment->segmentRecords[i];
	const char *databaseName = npr->databaseName;

	if (!databaseName)
	    databaseName = zh->admin_databaseName;
	printf ("--------------%d--------------------\n", i);
	if (npr->which == Z_NamePlusRecord_intermediateFragment)
	{
	    Z_FragmentSyntax *fragment = npr->u.intermediateFragment;
	    if (fragment->which == Z_FragmentSyntax_notExternallyTagged)
	    {
		Odr_oct *oct = fragment->u.notExternallyTagged;
		printf ("%.*s", (oct->len > 100 ? 100 : oct->len) ,
			oct->buf);
		
		sysno = 0;
		extract_rec_in_mem (zh, "grs.sgml",
				    oct->buf, oct->len,
				    databaseName,
				    0 /* delete_flag */,
				    0 /* test_mode */,
				    &sysno /* sysno */,
				    1 /* store_keys */,
				    1 /* store_data */,
				    0 /* match criteria */);
	    }
	}
    }
}

void zebra_admin_create (ZebraHandle zh, const char *database)
{
    ZebraService zs;

    zebra_begin_trans (zh);

    zs = zh->service;
    /* announce database */
    if (zebraExplain_newDatabase (zh->service->zei, database, 0 
                                  /* explainDatabase */))
    {
	zh->errCode = 224;
	zh->errString = "Database already exist";
    }
    zebra_end_trans (zh);
}

int zebra_string_norm (ZebraHandle zh, unsigned reg_id,
		       const char *input_str, int input_len,
		       char *output_str, int output_len)
{
    WRBUF wrbuf;
    if (!zh->service->zebra_maps)
	return -1;
    wrbuf = zebra_replace(zh->service->zebra_maps, reg_id, "",
			  input_str, input_len);
    if (!wrbuf)
	return -2;
    if (wrbuf_len(wrbuf) >= output_len)
	return -3;
    if (wrbuf_len(wrbuf))
	memcpy (output_str, wrbuf_buf(wrbuf), wrbuf_len(wrbuf));
    output_str[wrbuf_len(wrbuf)] = '\0';
    return wrbuf_len(wrbuf);
}


void zebra_set_state (int val, int seqno)
{
    long p = getpid();
    FILE *f = fopen ("state.LCK", "w");
    fprintf (f, "%c %d %ld\n", val, seqno, p);
    fclose (f);
}

void zebra_get_state (char *val, int *seqno)
{
    FILE *f = fopen ("state.LCK", "r");

    *val = 'o';
    *seqno = 0;

    if (f)
    {
        fscanf (f, "%c %d", val, seqno);
        fclose (f);
    }
}

static int zebra_begin_read (ZebraHandle zh)
{
    int dirty = 0;
    char val;
    int seqno;


    (zh->trans_no)++;

    if (zh->trans_no != 1)
    {
        zebra_flush_reg (zh);
        return 0;
    }

    zebra_get_state (&val, &seqno);
    if (val == 'd')
        val = 'o';
    if (seqno != zh->seqno)
    {
        yaz_log (LOG_LOG, "reopen seqno cur/old %d/%d", seqno, zh->seqno);
        dirty = 1;
    }
    else if (zh->last_val != val)
    {
        yaz_log (LOG_LOG, "reopen last cur/old %d/%d", val, zh->last_val);
        dirty = 1;
    }
    if (!dirty)
        return 0;

    if (val == 'c')
        zebra_lock_r (zh->lock_shadow);
    else
        zebra_lock_r (zh->lock_normal);
    
    zh->last_val = val;
    zh->seqno = seqno;

    zebra_register_deactivate (zh);

    zebra_register_activate (zh, 0, val == 'c' ? 1 : 0);
    return 0;
}

static void zebra_end_read (ZebraHandle zh)
{
    (zh->trans_no)--;

    if (zh->trans_no != 0)
        return;

    zebra_unlock (zh->lock_normal);
    zebra_unlock (zh->lock_shadow);
}

void zebra_begin_trans (ZebraHandle zh)
{
    int pass;
    int seqno = 0;
    char val = '?';
    const char *rval;

    (zh->trans_no++);
    if (zh->trans_no != 1)
    {
        return;
    }

    yaz_log (LOG_LOG, "zebra_begin_trans");
#if HAVE_SYS_TIMES_H
    times (&zh->tms1);
#endif

    /* lock */
    rval = res_get (zh->service->res, "shadow");

    for (pass = 0; pass < 2; pass++)
    {
        if (rval)
        {
            zebra_lock_r (zh->lock_normal);
            zebra_lock_w (zh->lock_shadow);
        }
        else
        {
            zebra_lock_w (zh->lock_normal);
            zebra_lock_w (zh->lock_shadow);
        }
        
        zebra_get_state (&val, &seqno);
        if (val == 'c')
        {
            yaz_log (LOG_LOG, "previous transaction didn't finish commit");
            zebra_unlock (zh->lock_shadow);
            zebra_unlock (zh->lock_normal);
            zebra_commit (zh);
            continue;
        }
        else if (val == 'd')
        {
            if (rval)
            {
                BFiles bfs = bfs_create (res_get (zh->service->res, "shadow"));
                yaz_log (LOG_LOG, "previous transaction didn't reach commit");
                bf_commitClean (bfs, rval);
                bfs_destroy (bfs);
            }
            else
            {
                yaz_log (LOG_WARN, "your previous transaction didn't finish");
            }
        }
        break;
    }
    if (pass == 2)
    {
        yaz_log (LOG_FATAL, "zebra_begin_trans couldn't finish commit");
        abort();
        return;
    }
    zebra_set_state ('d', seqno);

    zebra_register_activate (zh, 1, rval ? 1 : 0);
    zh->seqno = seqno;
}

void zebra_end_trans (ZebraHandle zh)
{
    char val;
    int seqno;
    const char *rval;

    zh->trans_no--;
    if (zh->trans_no != 0)
        return;

    yaz_log (LOG_LOG, "zebra_end_trans");
    rval = res_get (zh->service->res, "shadow");

    zebra_flush_reg (zh);

    zebra_register_deactivate (zh);

    zebra_get_state (&val, &seqno);
    if (val != 'd')
    {
        BFiles bfs = bfs_create (res_get (zh->service->res, "shadow"));
        bf_commitClean (bfs, rval);
        bfs_destroy (bfs);
    }
    if (!rval)
        seqno++;
    zebra_set_state ('o', seqno);

    zebra_unlock (zh->lock_shadow);
    zebra_unlock (zh->lock_normal);

#if HAVE_SYS_TIMES_H
    times (&zh->tms2);
    logf (LOG_LOG, "user/system: %ld/%ld",
                    (long) (zh->tms2.tms_utime - zh->tms1.tms_utime),
                    (long) (zh->tms2.tms_stime - zh->tms1.tms_stime));

#endif
}

void zebra_repository_update (ZebraHandle zh)
{
    zebra_begin_trans (zh);
    logf (LOG_LOG, "updating %s", zh->rGroup.path);
    repositoryUpdate (zh);    
    zebra_end_trans (zh);
}

void zebra_repository_delete (ZebraHandle zh)
{
    logf (LOG_LOG, "deleting %s", zh->rGroup.path);
    repositoryDelete (zh);
}

void zebra_repository_show (ZebraHandle zh)
{
    repositoryShow (zh);
}

void zebra_commit (ZebraHandle zh)
{
    int seqno;
    char val;
    const char *rval = res_get (zh->service->res, "shadow");
    BFiles bfs;

    if (!rval)
    {
        logf (LOG_WARN, "Cannot perform commit");
        logf (LOG_WARN, "No shadow area defined");
        return;
    }

    zebra_lock_w (zh->lock_normal);
    zebra_lock_r (zh->lock_shadow);

    bfs = bfs_create (res_get (zh->service->res, "register"));

    zebra_get_state (&val, &seqno);

    if (rval && *rval)
        bf_cache (bfs, rval);
    if (bf_commitExists (bfs))
    {
        zebra_set_state ('c', seqno);

        logf (LOG_LOG, "commit start");
        bf_commitExec (bfs);
#ifndef WIN32
        sync ();
#endif
        logf (LOG_LOG, "commit clean");
        bf_commitClean (bfs, rval);
        seqno++;
        zebra_set_state ('o', seqno);
    }
    else
    {
        logf (LOG_LOG, "nothing to commit");
    }
    bfs_destroy (bfs);

    zebra_unlock (zh->lock_shadow);
    zebra_unlock (zh->lock_normal);
}

void zebra_init (ZebraHandle zh)
{
    const char *rval = res_get (zh->service->res, "shadow");
    BFiles bfs = 0;

    bfs = bfs_create (res_get (zh->service->res, "register"));
    if (rval && *rval)
        bf_cache (bfs, rval);
    
    bf_reset (bfs);
    bfs_destroy (bfs);
    zebra_set_state ('o', 0);
}

void zebra_compact (ZebraHandle zh)
{
    BFiles bfs = bfs_create (res_get (zh->service->res, "register"));
    inv_compact (bfs);
    bfs_destroy (bfs);
}

int zebra_record_insert (ZebraHandle zh, const char *buf, int len)
{
    int sysno = 0;
    zebra_begin_trans (zh);
    extract_rec_in_mem (zh, "grs.sgml",
                        buf, len,
                        "Default",  /* database */
                        0 /* delete_flag */,
                        0 /* test_mode */,
                        &sysno /* sysno */,
                        1 /* store_keys */,
                        1 /* store_data */,
                        0 /* match criteria */);
    zebra_end_trans (zh);
    return sysno;
}
