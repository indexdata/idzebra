/*
 * Copyright (C) 1995-2000, Index Data
 * All rights reserved.
 *
 * $Log: zebraapi.c,v $
 * Revision 1.30  2000-04-05 09:49:35  adam
 * On Unix, zebra/z'mbol uses automake.
 *
 * Revision 1.29  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.28  2000/03/15 15:00:30  adam
 * First work on threaded version.
 *
 * Revision 1.27  2000/02/24 12:31:17  adam
 * Added zebra_string_norm.
 *
 * Revision 1.26  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.25  1999/11/04 15:00:45  adam
 * Implemented delete result set(s).
 *
 * Revision 1.24  1999/10/14 14:33:50  adam
 * Added truncation 5=106.
 *
 * Revision 1.23  1999/09/07 11:36:32  adam
 * Minor changes.
 *
 * Revision 1.22  1999/08/02 10:13:47  adam
 * Fixed bug regarding zebra_hits.
 *
 * Revision 1.21  1999/07/14 10:59:26  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.20  1999/07/06 12:28:04  adam
 * Updated record index structure. Format includes version ID. Compression
 * algorithm ID is stored for each record block.
 *
 * Revision 1.19  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.18  1999/05/15 14:36:38  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.17  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.16  1999/02/19 10:38:30  adam
 * Implemented chdir-setting.
 *
 * Revision 1.15  1999/02/17 12:18:12  adam
 * Fixed zebra_close so that a NULL pointer is ignored.
 *
 * Revision 1.14  1999/02/02 14:51:11  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.13  1998/12/16 12:23:30  adam
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

static int extract_rec_in_mem (ZebraHandle zh, const char *recordType,
			       const char *buf, size_t buf_size,
			       const char *databaseName, int delete_flag,
			       int test_mode, int *sysno,
			       int store_keys, int store_data,
			       const char *match_criteria);

static int explain_extract (void *handle, Record rec, data1_node *n);
static void extract_index (ZebraHandle zh);

static void zebra_register_unlock (ZebraHandle zh);

static int zebra_register_lock (ZebraHandle zh)
{
    zh->errCode = 0;
    zh->errString = 0;
    if (!zh->service->active)
    {
	zh->errCode = 1019;
	return 1;
    }
    return 0;
}

static void zebra_register_unlock (ZebraHandle zh)
{
}

ZebraHandle zebra_open (ZebraService zs)
{
    ZebraHandle zh;

    assert (zs);
    if (zs->stop_flag)
    {
	zh->errCode = 1019;
	return 0;
    }

    zh = (ZebraHandle) xmalloc (sizeof(*zh));

    zh->service = zs;
    zh->sets = 0;
    zh->destroyed = 0;
    zh->errCode = 0;
    zh->errString = 0;

    zh->key_buf = 0;
    zh->admin_databaseName = 0;
    
    zebra_mutex_cond_lock (&zs->session_lock);

    zh->next = zs->sessions;
    zs->sessions = zh;

    zebra_mutex_cond_unlock (&zs->session_lock);
    return zh;
}

static int zebra_register_activate (ZebraService zh);
static int zebra_register_deactivate (ZebraService zh);

ZebraService zebra_start (const char *configName)
{
    ZebraService zh = xmalloc (sizeof(*zh));

    yaz_log (LOG_LOG, "zebra_start %s", configName);

    zh->configName = xstrdup(configName);
    zh->sessions = 0;
    zh->stop_flag = 0;
    zh->active = 0;
    zebra_mutex_cond_init (&zh->session_lock);
    zebra_register_activate (zh);
    return zh;
}

static int zebra_register_activate (ZebraService zh)
{
    if (zh->active)
	return 0;
    yaz_log (LOG_LOG, "zebra_register_activate");
    if (!(zh->res = res_open (zh->configName)))
    {
	logf (LOG_WARN, "Failed to read resources `%s'", zh->configName);
	return -1;
    }
    zebra_chdir (zh);
    zh->dh = data1_create ();
    if (!zh->dh)
        return -1;
    zh->bfs = bfs_create (res_get (zh->res, "register"));
    if (!zh->bfs)
    {
        data1_destroy(zh->dh);
        return -1;
    }
    bf_lockDir (zh->bfs, res_get (zh->res, "lockDir"));
    data1_set_tabpath (zh->dh, res_get(zh->res, "profilePath"));
    zh->registerState = -1;  /* trigger open of registers! */
    zh->registerChange = 0;
    zh->recTypes = recTypes_init (zh->dh);
    recTypes_default_handlers (zh->recTypes);

    zh->records = NULL;
    zh->zebra_maps = zebra_maps_open (zh->res);
    zh->rank_classes = NULL;

    zh->records = 0;
    zh->dict = 0;
    zh->sortIdx = 0;
    zh->isams = 0;
    zh->isam = 0;
    zh->isamc = 0;
    zh->isamd = 0;
    zh->zei = 0;
    
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

    if (!(zh->records = rec_open (zh->bfs, 1, 0)))
    {
	logf (LOG_WARN, "rec_open");
	return -1;
    }
    if (!(zh->dict = dict_open (zh->bfs, FNAME_DICT, 40, 1, 0)))
    {
	logf (LOG_WARN, "dict_open");
	return -1;
    }
    if (!(zh->sortIdx = sortIdx_open (zh->bfs, 0)))
    {
	logf (LOG_WARN, "sortIdx_open");
	return -1;
    }
    if (res_get_match (zh->res, "isam", "s", ISAM_DEFAULT))
    {
	struct ISAMS_M_s isams_m;
	if (!(zh->isams = isams_open (zh->bfs, FNAME_ISAMS, 1,
				      key_isams_m(zh->res, &isams_m))))
	{
	    logf (LOG_WARN, "isams_open");
	    return -1;
	}
    }
#if ZMBOL
    else if (res_get_match (zh->res, "isam", "i", ISAM_DEFAULT))
    {
	if (!(zh->isam = is_open (zh->bfs, FNAME_ISAM, key_compare, 1,
				  sizeof (struct it_key), zh->res)))
	{
	    logf (LOG_WARN, "is_open");
	    return -1;
	}
    }
    else if (res_get_match (zh->res, "isam", "c", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
	if (!(zh->isamc = isc_open (zh->bfs, FNAME_ISAMC,
				    1, key_isamc_m(zh->res, &isamc_m))))
	{
	    logf (LOG_WARN, "isc_open");
	    return -1;
	}
    }
    else if (res_get_match (zh->res, "isam", "d", ISAM_DEFAULT))
    {
	struct ISAMD_M_s isamd_m;
	
	if (!(zh->isamd = isamd_open (zh->bfs, FNAME_ISAMD,
				      1, key_isamd_m(zh->res, &isamd_m))))
	{
	    logf (LOG_WARN, "isamd_open");
	    return -1;
	}
    }
#endif
    zh->zei = zebraExplain_open (zh->records, zh->dh,
				 zh->res, 1, 0 /* rGroup */,
				 explain_extract);
    if (!zh->zei)
    {
	logf (LOG_WARN, "Cannot obtain EXPLAIN information");
	return -1;
    }
    zh->active = 1;
    yaz_log (LOG_LOG, "zebra_register_activate ok");
    return 0;
}

void zebra_admin_shutdown (ZebraHandle zh)
{
    zebraExplain_flush (zh->service->zei, 1, zh);
    extract_index (zh);

    zebra_mutex_cond_lock (&zh->service->session_lock);
    zh->service->stop_flag = 1;
    if (!zh->service->sessions)
	zebra_register_deactivate(zh->service);
    zebra_mutex_cond_unlock (&zh->service->session_lock);
}

void zebra_admin_start (ZebraHandle zh)
{
    ZebraService zs = zh->service;
    zh->errCode = 0;
    zebra_mutex_cond_lock (&zs->session_lock);
    if (!zs->stop_flag)
	zebra_register_activate(zs);
    zebra_mutex_cond_unlock (&zs->session_lock);
}

static int zebra_register_deactivate (ZebraService zh)
{
    zh->stop_flag = 0;
    if (!zh->active)
	return 0;
    yaz_log(LOG_LOG, "zebra_register_deactivate");
    zebra_chdir (zh);
    if (zh->records)
    {
        zebraExplain_close (zh->zei, 1);
        dict_close (zh->dict);
	sortIdx_close (zh->sortIdx);
	if (zh->isams)
	    isams_close (zh->isams);
#if ZMBOL
        if (zh->isam)
            is_close (zh->isam);
        if (zh->isamc)
            isc_close (zh->isamc);
        if (zh->isamd)
            isamd_close (zh->isamd);
#endif
        rec_close (&zh->records);
    }
    recTypes_destroy (zh->recTypes);
    zebra_maps_close (zh->zebra_maps);
    zebraRankDestroy (zh);
    bfs_destroy (zh->bfs);
    data1_destroy (zh->dh);

    if (zh->passwd_db)
	passwd_db_close (zh->passwd_db);
    res_close (zh->res);
    zh->active = 0;
    return 0;
}

void zebra_stop(ZebraService zh)
{
    if (!zh)
	return ;
    yaz_log (LOG_LOG, "zebra_stop");

    assert (!zh->sessions);

    zebra_mutex_cond_destroy (&zh->session_lock);

    zebra_register_deactivate(zh);
    xfree (zh->configName);
    xfree (zh);
}

void zebra_close (ZebraHandle zh)
{
    ZebraService zs = zh->service;
    struct zebra_session **sp;
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
    if (!zs->sessions && zs->stop_flag)
	zebra_register_deactivate(zs);
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
    if (zebra_register_lock (zh))
	return;
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

    if (zebra_register_lock (zh))
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
    if (zebra_register_lock (zh))
    {
	*entries = 0;
	*num_entries = 0;
	return;
    }
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
    if (zebra_register_lock (zh))
	return;
    resultSetSort (zh, stream->mem, num_input_setnames, input_setnames,
		   output_setname, sort_sequence, sort_status);
    zebra_register_unlock (zh);
}

int zebra_deleleResultSet(ZebraHandle zh, int function,
			  int num_setnames, char **setnames,
			  int *statuses)
{
    int i, status;
    if (zebra_register_lock (zh))
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
    zebra_register_unlock (zh);
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
	return 0;
    return 1;
}

void zebra_admin_import_begin (ZebraHandle zh, const char *database)
{
    if (zebra_register_lock (zh))
	return;
    xfree (zh->admin_databaseName);
    zh->admin_databaseName = xstrdup(database);
    zebra_register_unlock(zh);
}

void zebra_admin_import_end (ZebraHandle zh)
{
    zebraExplain_flush (zh->service->zei, 1, zh);
    extract_index (zh);
}

void zebra_admin_import_segment (ZebraHandle zh, Z_Segment *segment)
{
    int sysno;
    int i;
    if (zebra_register_lock (zh))
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
    zebra_register_unlock(zh);
}

void zebra_admin_create (ZebraHandle zh, const char *database)
{
    ZebraService zs = zh->service;
    if (zebra_register_lock(zh))
    {
	zh->errCode = 1019;
	return;
    }
    /* announce database */
    if (zebraExplain_newDatabase (zs->zei, database, 0 /* explainDatabase */))
    {
	zh->errCode = 224;
	zh->errString = "Database already exist";
    }
    zebra_register_unlock(zh);
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

static void extract_init (struct recExtractCtrl *p, RecWord *w)
{
    w->zebra_maps = p->zebra_maps;
    w->seqnos = p->seqno;
    w->attrSet = VAL_BIB1;
    w->attrUse = 1016;
    w->reg_type = 'w';
    w->extractCtrl = p;
}

static void extract_add_index_string (RecWord *p, const char *string,
				      int length)
{
    char *dst;
    unsigned char attrSet;
    unsigned short attrUse;
    int lead = 0;
    int diff = 0;
    int *pseqno = &p->seqnos[p->reg_type];
    ZebraHandle zh = p->extractCtrl->handle;
    struct recKeys *keys = &zh->keys;

    if (keys->buf_used+1024 > keys->buf_max)
    {
        char *b;

        b = (char *) xmalloc (keys->buf_max += 128000);
        if (keys->buf_used > 0)
            memcpy (b, keys->buf, keys->buf_used);
        xfree (keys->buf);
        keys->buf = b;
    }
    dst = keys->buf + keys->buf_used;

    attrSet = p->attrSet;
    if (keys->buf_used > 0 && keys->prevAttrSet == attrSet)
        lead |= 1;
    else
        keys->prevAttrSet = attrSet;
    attrUse = p->attrUse;
    if (keys->buf_used > 0 && keys->prevAttrUse == attrUse)
        lead |= 2;
    else
        keys->prevAttrUse = attrUse;
#if 1
    diff = 1 + *pseqno - keys->prevSeqNo;
    if (diff >= 1 && diff <= 15)
        lead |= (diff << 2);
    else
        diff = 0;
#endif
    keys->prevSeqNo = *pseqno;
    
    *dst++ = lead;

    if (!(lead & 1))
    {
        memcpy (dst, &attrSet, sizeof(attrSet));
        dst += sizeof(attrSet);
    }
    if (!(lead & 2))
    {
        memcpy (dst, &attrUse, sizeof(attrUse));
        dst += sizeof(attrUse);
    }
    *dst++ = p->reg_type;
    memcpy (dst, string, length);
    dst += length;
    *dst++ = '\0';

    if (!diff)
    {
        memcpy (dst, pseqno, sizeof(*pseqno));
        dst += sizeof(*pseqno);
    }
    keys->buf_used = dst - keys->buf;
    if (*pseqno)
	(*pseqno)++;
}

static void extract_add_sort_string (RecWord *p, const char *string,
				     int length)
{
    struct sortKey *sk;
    ZebraHandle zh = p->extractCtrl->handle;
    struct sortKey *sortKeys = zh->sortKeys;

    for (sk = sortKeys; sk; sk = sk->next)
	if (sk->attrSet == p->attrSet && sk->attrUse == p->attrUse)
	    return;

    sk = (struct sortKey *) xmalloc (sizeof(*sk));
    sk->next = sortKeys;
    sortKeys = sk;

    sk->string = (char *) xmalloc (length);
    sk->length = length;
    memcpy (sk->string, string, length);

    sk->attrSet = p->attrSet;
    sk->attrUse = p->attrUse;
}

static void extract_add_string (RecWord *p, const char *string, int length)
{
    assert (length > 0);
    if (zebra_maps_is_sort (p->zebra_maps, p->reg_type))
	extract_add_sort_string (p, string, length);
    else
	extract_add_index_string (p, string, length);
}

static void extract_add_incomplete_field (RecWord *p)
{
    const char *b = p->string;
    int remain = p->length;
    const char **map = 0;

    if (remain > 0)
	map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain);

    while (map)
    {
	char buf[IT_MAX_WORD+1];
	int i, remain;

	/* Skip spaces */
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->length - (b - p->string);
	    if (remain > 0)
		map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain);
	    else
		map = 0;
	}
	if (!map)
	    break;
	i = 0;
	while (map && *map && **map != *CHR_SPACE)
	{
	    const char *cp = *map;

	    while (i < IT_MAX_WORD && *cp)
		buf[i++] = *(cp++);
	    remain = p->length - (b - p->string);
	    if (remain > 0)
		map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain);
	    else
		map = 0;
	}
	if (!i)
	    return;
	extract_add_string (p, buf, i);
    }
    (p->seqnos[p->reg_type])++; /* to separate this from next one  */
}

static void extract_add_complete_field (RecWord *p)
{
    const char *b = p->string;
    char buf[IT_MAX_WORD+1];
    const char **map = 0;
    int i = 0, remain = p->length;

    if (remain > 0)
	map = zebra_maps_input (p->zebra_maps, p->reg_type, &b, remain);

    while (remain > 0 && i < IT_MAX_WORD)
    {
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->length - (b - p->string);
	    if (remain > 0)
		map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain);
	    else
		map = 0;
	}
	if (!map)
	    break;

	if (i && i < IT_MAX_WORD)
	    buf[i++] = *CHR_SPACE;
	while (map && *map && **map != *CHR_SPACE)
	{
	    const char *cp = *map;

	    if (i >= IT_MAX_WORD)
		break;
	    while (i < IT_MAX_WORD && *cp)
		buf[i++] = *(cp++);
	    remain = p->length  - (b - p->string);
	    if (remain > 0)
		map = zebra_maps_input (p->zebra_maps, p->reg_type, &b,
					remain);
	    else
		map = 0;
	}
    }
    if (!i)
	return;
    extract_add_string (p, buf, i);
}

static void extract_token_add (RecWord *p)
{
    WRBUF wrbuf;
    if ((wrbuf = zebra_replace(p->zebra_maps, p->reg_type, 0,
			       p->string, p->length)))
    {
	p->string = wrbuf_buf(wrbuf);
	p->length = wrbuf_len(wrbuf);
    }
    if (zebra_maps_is_complete (p->zebra_maps, p->reg_type))
	extract_add_complete_field (p);
    else
	extract_add_incomplete_field(p);
}

static void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid)
{
    ZebraHandle zh = (ZebraHandle) (p->handle);
    zebraExplain_addSchema (zh->service->zei, oid);
}

static void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
				   int cmd, struct sortKey **skp)
{
    struct sortKey *sk = *skp;
    SortIdx sortIdx = zh->service->sortIdx;

    sortIdx_sysno (sortIdx, sysno);
    while (sk)
    {
	struct sortKey *sk_next = sk->next;
	sortIdx_type (sortIdx, sk->attrUse);
	sortIdx_add (sortIdx, sk->string, sk->length);
	xfree (sk->string);
	xfree (sk);
	sk = sk_next;
    }
    *skp = 0;
}

struct encode_info {
    int  sysno;
    int  seqno;
    int  cmd;
    char buf[768];
};

void encode_key_init (struct encode_info *i)
{
    i->sysno = 0;
    i->seqno = 0;
    i->cmd = -1;
}

char *encode_key_int (int d, char *bp)
{
    if (d <= 63)
        *bp++ = d;
    else if (d <= 16383)
    {
        *bp++ = 64 + (d>>8);
        *bp++ = d  & 255;
    }
    else if (d <= 4194303)
    {
        *bp++ = 128 + (d>>16);
        *bp++ = (d>>8) & 255;
        *bp++ = d & 255;
    }
    else
    {
        *bp++ = 192 + (d>>24);
        *bp++ = (d>>16) & 255;
        *bp++ = (d>>8) & 255;
        *bp++ = d & 255;
    }
    return bp;
}

void encode_key_write (char *k, struct encode_info *i, FILE *outf)
{
    struct it_key key;
    char *bp = i->buf;

    while ((*bp++ = *k++))
        ;
    memcpy (&key, k+1, sizeof(struct it_key));
    bp = encode_key_int ( (key.sysno - i->sysno) * 2 + *k, bp);
    if (i->sysno != key.sysno)
    {
        i->sysno = key.sysno;
        i->seqno = 0;
    }
    else if (!i->seqno && !key.seqno && i->cmd == *k)
	return;
    bp = encode_key_int (key.seqno - i->seqno, bp);
    i->seqno = key.seqno;
    i->cmd = *k;
    if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "fwrite");
        exit (1);
    }
}

static void extract_flushWriteKeys (ZebraHandle zh)
{
    FILE *outf;
    char out_fname[200];
    char *prevcp, *cp;
    struct encode_info encode_info;
    int ptr_i = zh->ptr_i;
#if SORT_EXTRA
    int i;
#endif
    if (!zh->key_buf || ptr_i <= 0)
        return;

    (zh->key_file_no)++;
    logf (LOG_LOG, "sorting section %d", (zh->key_file_no));
#if !SORT_EXTRA
    qsort (zh->key_buf + zh->ptr_top - ptr_i, ptr_i, sizeof(char*),
	    key_qsort_compare);
    extract_get_fname_tmp (zh, out_fname, zh->key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", out_fname);
        exit (1);
    }
    logf (LOG_LOG, "writing section %d", zh->key_file_no);
    prevcp = cp = (zh->key_buf)[zh->ptr_top - ptr_i];
    
    encode_key_init (&encode_info);
    encode_key_write (cp, &encode_info, outf);
    
    while (--ptr_i > 0)
    {
        cp = (zh->key_buf)[zh->ptr_top - ptr_i];
        if (strcmp (cp, prevcp))
        {
            encode_key_init (&encode_info);
            encode_key_write (cp, &encode_info, outf);
            prevcp = cp;
        }
        else
            encode_key_write (cp + strlen(cp), &encode_info, outf);
    }
#else
    qsort (key_buf + ptr_top-ptr_i, ptr_i, sizeof(char*), key_x_compare);
    extract_get_fname_tmp (out_fname, key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", out_fname);
        exit (1);
    }
    logf (LOG_LOG, "writing section %d", key_file_no);
    i = ptr_i;
    prevcp =  key_buf[ptr_top-i];
    while (1)
        if (!--i || strcmp (prevcp, key_buf[ptr_top-i]))
        {
            key_y_len = strlen(prevcp)+1;
#if 0
            logf (LOG_LOG, "key_y_len: %2d %02x %02x %s",
                      key_y_len, prevcp[0], prevcp[1], 2+prevcp);
#endif
            qsort (key_buf + ptr_top-ptr_i, ptr_i - i,
                                   sizeof(char*), key_y_compare);
            cp = key_buf[ptr_top-ptr_i];
            --key_y_len;
            encode_key_init (&encode_info);
            encode_key_write (cp, &encode_info, outf);
            while (--ptr_i > i)
            {
                cp = key_buf[ptr_top-ptr_i];
                encode_key_write (cp+key_y_len, &encode_info, outf);
            }
            if (!i)
                break;
            prevcp = key_buf[ptr_top-ptr_i];
        }
#endif
    if (fclose (outf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fclose %s", out_fname);
        exit (1);
    }
    logf (LOG_LOG, "finished section %d", zh->key_file_no);
    zh->ptr_i = 0;
    zh->key_buf_used = 0;
}

static void extract_flushRecordKeys (ZebraHandle zh, SYSNO sysno,
				     int cmd, struct recKeys *reckeys)
{
    unsigned char attrSet = (unsigned char) -1;
    unsigned short attrUse = (unsigned short) -1;
    int seqno = 0;
    int off = 0;
    ZebraExplainInfo zei = zh->service->zei;

    if (!zh->key_buf)
    {
	int mem = 8*1024*1024;
	zh->key_buf = (char**) xmalloc (mem);
	zh->ptr_top = mem/sizeof(char*);
	zh->ptr_i = 0;
	zh->key_buf_used = 0;
	zh->key_file_no = 0;
    }
    zebraExplain_recordCountIncrement (zei, cmd ? 1 : -1);
    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        struct it_key key;
        int lead, ch;
    
        lead = *src++;

        if (!(lead & 1))
        {
            memcpy (&attrSet, src, sizeof(attrSet));
            src += sizeof(attrSet);
        }
        if (!(lead & 2))
        {
            memcpy (&attrUse, src, sizeof(attrUse));
            src += sizeof(attrUse);
        }
        if (zh->key_buf_used + 1024 > (zh->ptr_top-zh->ptr_i)*sizeof(char*))
            extract_flushWriteKeys (zh);
        ++(zh->ptr_i);
        (zh->key_buf)[zh->ptr_top - zh->ptr_i] =
	    (char*)zh->key_buf + zh->key_buf_used;

        ch = zebraExplain_lookupSU (zei, attrSet, attrUse);
        if (ch < 0)
            ch = zebraExplain_addSU (zei, attrSet, attrUse);
        assert (ch > 0);
	zh->key_buf_used +=
	    key_SU_code (ch,((char*)zh->key_buf) + zh->key_buf_used);

        while (*src)
            ((char*)zh->key_buf) [(zh->key_buf_used)++] = *src++;
        src++;
        ((char*)(zh->key_buf))[(zh->key_buf_used)++] = '\0';
        ((char*)(zh->key_buf))[(zh->key_buf_used)++] = cmd;

        if (lead & 60)
            seqno += ((lead>>2) & 15)-1;
        else
        {
            memcpy (&seqno, src, sizeof(seqno));
            src += sizeof(seqno);
        }
        key.seqno = seqno;
        key.sysno = sysno;
        memcpy ((char*)zh->key_buf + zh->key_buf_used, &key, sizeof(key));
        (zh->key_buf_used) += sizeof(key);
        off = src - reckeys->buf;
    }
    assert (off == reckeys->buf_used);
}

static void extract_index (ZebraHandle zh)
{
    extract_flushWriteKeys (zh);
    zebra_index_merge (zh);
}

static int explain_extract (void *handle, Record rec, data1_node *n)
{
    ZebraHandle zh = (ZebraHandle) handle;
    struct recExtractCtrl extractCtrl;
    int i;

    if (zebraExplain_curDatabase (zh->service->zei,
				  rec->info[recInfo_databaseName]))
    {
	abort();
        if (zebraExplain_newDatabase (zh->service->zei,
				      rec->info[recInfo_databaseName], 0))
            abort ();
    }

    zh->keys.buf_used = 0;
    zh->keys.prevAttrUse = -1;
    zh->keys.prevAttrSet = -1;
    zh->keys.prevSeqNo = 0;
    zh->sortKeys = 0;
    
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->service->dh;
    for (i = 0; i<256; i++)
	extractCtrl.seqno[i] = 0;
    extractCtrl.zebra_maps = zh->service->zebra_maps;
    extractCtrl.flagShowRecords = 0;
    extractCtrl.handle = handle;
    
    grs_extract_tree(&extractCtrl, n);

    logf (LOG_LOG, "flush explain record, sysno=%d", rec->sysno);

    if (rec->size[recInfo_delKeys])
    {
	struct recKeys delkeys;
	struct sortKey *sortKeys = 0;

	delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];
	extract_flushSortKeys (zh, rec->sysno, 0, &sortKeys);
	extract_flushRecordKeys (zh, rec->sysno, 0, &delkeys);
    }
    extract_flushRecordKeys (zh, rec->sysno, 1, &zh->keys);
    extract_flushSortKeys (zh, rec->sysno, 1, &zh->sortKeys);

    xfree (rec->info[recInfo_delKeys]);
    rec->size[recInfo_delKeys] = zh->keys.buf_used;
    rec->info[recInfo_delKeys] = zh->keys.buf;
    zh->keys.buf = NULL;
    zh->keys.buf_max = 0;
    return 0;
}

static int extract_rec_in_mem (ZebraHandle zh, const char *recordType,
			       const char *buf, size_t buf_size,
			       const char *databaseName, int delete_flag,
			       int test_mode, int *sysno,
			       int store_keys, int store_data,
			       const char *match_criteria)
{
    RecordAttr *recordAttr;
    struct recExtractCtrl extractCtrl;
    int i, r;
    RecType recType;
    char subType[1024];
    void *clientData;
    const char *fname = "<no file>";
    Record rec;
    long recordOffset = 0;
    struct zebra_fetch_control fc;

    fc.fd = -1;
    fc.record_int_buf = buf;
    fc.record_int_len = buf_size;
    fc.record_int_pos = 0;
    fc.offset_end = 0;
    fc.record_offset = 0;

    extractCtrl.offset = 0;
    extractCtrl.readf = zebra_record_int_read;
    extractCtrl.seekf = zebra_record_int_seek;
    extractCtrl.tellf = zebra_record_int_tell;
    extractCtrl.endf = zebra_record_int_end;
    extractCtrl.fh = &fc;

    /* announce database */
    if (zebraExplain_curDatabase (zh->service->zei, databaseName))
    {
        if (zebraExplain_newDatabase (zh->service->zei, databaseName, 0))
	    return 0;
    }
    if (!(recType =
	  recType_byName (zh->service->recTypes, recordType, subType,
			  &clientData)))
    {
        logf (LOG_WARN, "No such record type: %s", recordType);
        return 0;
    }

    zh->keys.buf_used = 0;
    zh->keys.prevAttrUse = -1;
    zh->keys.prevAttrSet = -1;
    zh->keys.prevSeqNo = 0;
    zh->sortKeys = 0;

    extractCtrl.subType = subType;
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->service->dh;
    extractCtrl.handle = zh;
    extractCtrl.zebra_maps = zh->service->zebra_maps;
    extractCtrl.flagShowRecords = 0;
    for (i = 0; i<256; i++)
    {
	if (zebra_maps_is_positioned(zh->service->zebra_maps, i))
	    extractCtrl.seqno[i] = 1;
	else
	    extractCtrl.seqno[i] = 0;
    }

    r = (*recType->extract)(clientData, &extractCtrl);

    if (r == RECCTRL_EXTRACT_EOF)
	return 0;
    else if (r == RECCTRL_EXTRACT_ERROR)
    {
	/* error occured during extraction ... */
#if 1
	yaz_log (LOG_WARN, "extract error");
#else
	if (rGroup->flagRw &&
	    records_processed < rGroup->fileVerboseLimit)
	{
	    logf (LOG_WARN, "fail %s %s %ld", rGroup->recordType,
		  fname, (long) recordOffset);
	}
#endif
	return 0;
    }
    if (zh->keys.buf_used == 0)
    {
	/* the extraction process returned no information - the record
	   is probably empty - unless flagShowRecords is in use */
	if (test_mode)
	    return 1;
	logf (LOG_WARN, "No keys generated for record");
	logf (LOG_WARN, " The file is probably empty");
	return 1;
    }
    /* match criteria */

    if (! *sysno)
    {
        /* new record */
        if (delete_flag)
        {
	    logf (LOG_LOG, "delete %s %s %ld", recordType,
		  fname, (long) recordOffset);
            logf (LOG_WARN, "cannot delete record above (seems new)");
            return 1;
        }
	logf (LOG_LOG, "add %s %s %ld", recordType, fname,
	      (long) recordOffset);
        rec = rec_new (zh->service->records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zh->service->zei, rec);

#if 0
        if (matchStr)
        {
            dict_insert (matchDict, matchStr, sizeof(*sysno), sysno);
        }
#endif
        extract_flushRecordKeys (zh, *sysno, 1, &zh->keys);
	extract_flushSortKeys (zh, *sysno, 1, &zh->sortKeys);
    }
    else
    {
        /* record already exists */
        struct recKeys delkeys;

        rec = rec_get (zh->service->records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zh->service->zei, rec);

	if (recordAttr->runNumber ==
	    zebraExplain_runNumberIncrement (zh->service->zei, 0))
	{
	    logf (LOG_LOG, "skipped %s %s %ld", recordType,
		  fname, (long) recordOffset);
	    rec_rm (&rec);
	    return 1;
	}
        delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];
	extract_flushSortKeys (zh, *sysno, 0, &zh->sortKeys);
        extract_flushRecordKeys (zh, *sysno, 0, &delkeys);
        if (delete_flag)
        {
            /* record going to be deleted */
            if (!delkeys.buf_used)
            {
                logf (LOG_LOG, "delete %s %s %ld", recordType,
                      fname, (long) recordOffset);
                logf (LOG_WARN, "cannot delete file above, storeKeys false");
            }
            else
            {
		logf (LOG_LOG, "delete %s %s %ld", recordType,
		      fname, (long) recordOffset);
#if 0
                if (matchStr)
                    dict_delete (matchDict, matchStr);
#endif
                rec_del (zh->service->records, &rec);
            }
	    rec_rm (&rec);
            return 1;
        }
        else
        {
            /* record going to be updated */
            if (!delkeys.buf_used)
            {
                logf (LOG_LOG, "update %s %s %ld", recordType,
                      fname, (long) recordOffset);
                logf (LOG_WARN, "cannot update file above, storeKeys false");
            }
            else
            {
		logf (LOG_LOG, "update %s %s %ld", recordType,
		      fname, (long) recordOffset);
                extract_flushRecordKeys (zh, *sysno, 1, &zh->keys);
            }
        }
    }
    /* update file type */
    xfree (rec->info[recInfo_fileType]);
    rec->info[recInfo_fileType] =
        rec_strdup (recordType, &rec->size[recInfo_fileType]);

    /* update filename */
    xfree (rec->info[recInfo_filename]);
    rec->info[recInfo_filename] =
        rec_strdup (fname, &rec->size[recInfo_filename]);

    /* update delete keys */
    xfree (rec->info[recInfo_delKeys]);
    if (zh->keys.buf_used > 0 && store_keys == 1)
    {
        rec->size[recInfo_delKeys] = zh->keys.buf_used;
        rec->info[recInfo_delKeys] = zh->keys.buf;
        zh->keys.buf = NULL;
        zh->keys.buf_max = 0;
    }
    else
    {
        rec->info[recInfo_delKeys] = NULL;
        rec->size[recInfo_delKeys] = 0;
    }

    /* save file size of original record */
    zebraExplain_recordBytesIncrement (zh->service->zei,
				       - recordAttr->recordSize);
#if 0
    recordAttr->recordSize = fi->file_moffset - recordOffset;
    if (!recordAttr->recordSize)
	recordAttr->recordSize = fi->file_max - recordOffset;
#else
    recordAttr->recordSize = buf_size;
#endif
    zebraExplain_recordBytesIncrement (zh->service->zei,
				       recordAttr->recordSize);

    /* set run-number for this record */
    recordAttr->runNumber =
	zebraExplain_runNumberIncrement (zh->service->zei, 0);

    /* update store data */
    xfree (rec->info[recInfo_storeData]);
    if (store_data == 1)
    {
        rec->size[recInfo_storeData] = recordAttr->recordSize;
        rec->info[recInfo_storeData] = (char *)
	    xmalloc (recordAttr->recordSize);
#if 1
        memcpy (rec->info[recInfo_storeData], buf, recordAttr->recordSize);
#else
        if (lseek (fi->fd, recordOffset, SEEK_SET) < 0)
        {
            logf (LOG_ERRNO|LOG_FATAL, "seek to %ld in %s",
                  (long) recordOffset, fname);
            exit (1);
        }
        if (read (fi->fd, rec->info[recInfo_storeData], recordAttr->recordSize)
	    < recordAttr->recordSize)
        {
            logf (LOG_ERRNO|LOG_FATAL, "read %d bytes of %s",
                  recordAttr->recordSize, fname);
            exit (1);
        }
#endif
    }
    else
    {
        rec->info[recInfo_storeData] = NULL;
        rec->size[recInfo_storeData] = 0;
    }
    /* update database name */
    xfree (rec->info[recInfo_databaseName]);
    rec->info[recInfo_databaseName] =
        rec_strdup (databaseName, &rec->size[recInfo_databaseName]); 

    /* update offset */
    recordAttr->recordOffset = recordOffset;
    
    /* commit this record */
    rec_put (zh->service->records, &rec);

    return 0;
}
