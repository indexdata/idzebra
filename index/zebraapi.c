/* $Id: zebraapi.c,v 1.76 2002-10-30 14:35:09 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
#include "index.h"
#include <charmap.h>

static Res zebra_open_res (ZebraHandle zh);
static void zebra_close_res (ZebraHandle zh);

static void zebra_chdir (ZebraService zh)
{
    const char *dir = res_get (zh->global_res, "chdir");
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
    zebraExplain_flush (zh->reg->zei, zh);
    
    extract_flushWriteKeys (zh);
    zebra_index_merge (zh);
}

static struct zebra_register *zebra_register_open (ZebraService zs, 
                                                   const char *name,
                                                   int rw, int useshadow,
                                                   Res res,
                                                   const char *reg_path);
static void zebra_register_close (ZebraService zs, struct zebra_register *reg);

ZebraHandle zebra_open (ZebraService zs)
{
    ZebraHandle zh;
    const char *default_encoding;

    if (!zs)
        return 0;

    zh = (ZebraHandle) xmalloc (sizeof(*zh));
    yaz_log (LOG_DEBUG, "zebra_open zs=%p returns %p", zs, zh);

    zh->service = zs;
    zh->reg = 0;          /* no register attached yet */
    zh->sets = 0;
    zh->destroyed = 0;
    zh->errCode = 0;
    zh->errString = 0;
    zh->res = 0; 

    zh->reg_name = xstrdup ("");
    zh->path_reg = 0;
    zh->num_basenames = 0;
    zh->basenames = 0;

    zh->trans_no = 0;

    zh->lock_normal = 0;
    zh->lock_shadow = 0;

    zh->admin_databaseName = 0;

    zh->shadow_enable = 1;

    default_encoding = res_get_def(zs->global_res, "encoding", "ISO-8859-1");
    zh->record_encoding = xstrdup (default_encoding);

    zh->iconv_to_utf8 =
        yaz_iconv_open ("UTF-8", default_encoding);
    if (zh->iconv_to_utf8 == 0)
        yaz_log (LOG_WARN, "iconv: %s to UTF-8 unsupported",
           default_encoding);
    zh->iconv_from_utf8 =
        yaz_iconv_open (default_encoding, "UTF-8");
    if (zh->iconv_to_utf8 == 0)
        yaz_log (LOG_WARN, "iconv: UTF-8 to %s unsupported",
           default_encoding);

    zebra_mutex_cond_lock (&zs->session_lock);

    zh->next = zs->sessions;
    zs->sessions = zh;

    zebra_mutex_cond_unlock (&zs->session_lock);

    return zh;
}

ZebraService zebra_start (const char *configName)
{
    Res res;

    yaz_log (LOG_LOG, "zebra_start %s", configName);

    if ((res = res_open (configName, 0)))
    {
        ZebraService zh = xmalloc (sizeof(*zh));

	yaz_log (LOG_DEBUG, "Read resources `%s'", configName);
        
        zh->global_res = res;
        zh->configName = xstrdup(configName);
        zh->sessions = 0;
        
        zebra_chdir (zh);
        
        zebra_mutex_cond_init (&zh->session_lock);
        if (!res_get (zh->global_res, "passwd"))
            zh->passwd_db = NULL;
        else
        {
            zh->passwd_db = passwd_db_open ();
            if (!zh->passwd_db)
                logf (LOG_WARN|LOG_ERRNO, "passwd_db_open failed");
            else
                passwd_db_file (zh->passwd_db,
                                res_get (zh->global_res, "passwd"));
        }
        zh->path_root = res_get (zh->global_res, "root");
        return zh;
    }
    return 0;
}

static
struct zebra_register *zebra_register_open (ZebraService zs, const char *name,
                                            int rw, int useshadow, Res res,
                                            const char *reg_path)
{
    struct zebra_register *reg;
    int record_compression = REC_COMPRESS_NONE;
    char *recordCompression = 0;

    reg = xmalloc (sizeof(*reg));

    assert (name);
    reg->name = xstrdup (name);

    reg->seqno = 0;
    reg->last_val = 0;

    assert (res);

    yaz_log (LOG_DEBUG, "zebra_register_open rw = %d useshadow=%d p=%p",
             rw, useshadow, reg);

    reg->dh = data1_createx (DATA1_FLAG_XML);
    if (!reg->dh)
        return 0;
    reg->bfs = bfs_create (res_get (res, "register"), reg_path);
    if (!reg->bfs)
    {
        data1_destroy(reg->dh);
        return 0;
    }
    if (useshadow)
        bf_cache (reg->bfs, res_get (res, "shadow"));
    data1_set_tabpath (reg->dh, res_get_def(res, "profilePath",
                                            DEFAULT_PROFILE_PATH));
    data1_set_tabroot (reg->dh, reg_path);
    reg->recTypes = recTypes_init (reg->dh);
    recTypes_default_handlers (reg->recTypes);

    reg->zebra_maps = zebra_maps_open (res, reg_path);
    reg->rank_classes = NULL;

    reg->key_buf = 0;

    reg->keys.buf_max = 0;
    reg->keys.buf = 0;

    reg->records = 0;
    reg->dict = 0;
    reg->sortIdx = 0;
    reg->isams = 0;
    reg->matchDict = 0;
    reg->isam = 0;
    reg->isamc = 0;
    reg->isamd = 0;
    reg->isamb = 0;
    reg->zei = 0;
    reg->matchDict = 0;
    
    zebraRankInstall (reg, rank1_class);

    recordCompression = res_get_def (res, "recordCompression", "none");
    if (!strcmp (recordCompression, "none"))
	record_compression = REC_COMPRESS_NONE;
    if (!strcmp (recordCompression, "bzip2"))
	record_compression = REC_COMPRESS_BZIP2;

    if (!(reg->records = rec_open (reg->bfs, rw, record_compression)))
    {
	logf (LOG_WARN, "rec_open");
	return 0;
    }
    if (rw)
    {
        reg->matchDict = dict_open (reg->bfs, GMATCH_DICT, 20, 1, 0);
    }
    if (!(reg->dict = dict_open (reg->bfs, FNAME_DICT, 40, rw, 0)))
    {
	logf (LOG_WARN, "dict_open");
	return 0;
    }
    if (!(reg->sortIdx = sortIdx_open (reg->bfs, rw)))
    {
	logf (LOG_WARN, "sortIdx_open");
	return 0;
    }
    if (res_get_match (res, "isam", "s", ISAM_DEFAULT))
    {
	struct ISAMS_M_s isams_m;
	if (!(reg->isams = isams_open (reg->bfs, FNAME_ISAMS, rw,
				      key_isams_m(res, &isams_m))))
	{
	    logf (LOG_WARN, "isams_open");
	    return 0;
	}
    }
    if (res_get_match (res, "isam", "i", ISAM_DEFAULT))
    {
	if (!(reg->isam = is_open (reg->bfs, FNAME_ISAM, key_compare, rw,
				  sizeof (struct it_key), res)))
	{
	    logf (LOG_WARN, "is_open");
	    return 0;
	}
    }
    if (res_get_match (res, "isam", "c", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
	if (!(reg->isamc = isc_open (reg->bfs, FNAME_ISAMC,
				    rw, key_isamc_m(res, &isamc_m))))
	{
	    logf (LOG_WARN, "isc_open");
	    return 0;
	}
    }
    if (res_get_match (res, "isam", "d", ISAM_DEFAULT))
    {
	struct ISAMD_M_s isamd_m;
	
	if (!(reg->isamd = isamd_open (reg->bfs, FNAME_ISAMD,
				      rw, key_isamd_m(res, &isamd_m))))
	{
	    logf (LOG_WARN, "isamd_open");
	    return 0;
	}
    }
    if (res_get_match (res, "isam", "b", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
	
	if (!(reg->isamb = isamb_open (reg->bfs, "isamb",
                                       rw, key_isamc_m(res, &isamc_m), 0)))
	{
	    logf (LOG_WARN, "isamb_open");
	    return 0;
	}
    }
    if (res_get_match (res, "isam", "bc", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
	
	if (!(reg->isamb = isamb_open (reg->bfs, "isamb",
                                       rw, key_isamc_m(res, &isamc_m), 1)))
	{
	    logf (LOG_WARN, "isamb_open");
	    return 0;
	}
    }
    if (res_get_match (res, "isam", "null", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
	
	if (!(reg->isamb = isamb_open (reg->bfs, "isamb",
                                       rw, key_isamc_m(res, &isamc_m), -1)))
	{
	    logf (LOG_WARN, "isamb_open");
	    return 0;
	}
    }
    reg->zei = zebraExplain_open (reg->records, reg->dh,
                                  res, rw, reg,
                                  explain_extract);
    if (!reg->zei)
    {
	logf (LOG_WARN, "Cannot obtain EXPLAIN information");
	return 0;
    }
    reg->active = 2;
    yaz_log (LOG_DEBUG, "zebra_register_open ok p=%p", reg);
    return reg;
}

void zebra_admin_shutdown (ZebraHandle zh)
{
    zebra_mutex_cond_lock (&zh->service->session_lock);
    zh->service->stop_flag = 1;
    zebra_mutex_cond_unlock (&zh->service->session_lock);
}

void zebra_admin_start (ZebraHandle zh)
{
    ZebraService zs = zh->service;
    zh->errCode = 0;
    zebra_mutex_cond_lock (&zs->session_lock);
    zebra_mutex_cond_unlock (&zs->session_lock);
}

static void zebra_register_close (ZebraService zs, struct zebra_register *reg)
{
    yaz_log(LOG_DEBUG, "zebra_register_close p=%p", reg);
    reg->stop_flag = 0;
    zebra_chdir (zs);
    if (reg->records)
    {
        zebraExplain_close (reg->zei);
        dict_close (reg->dict);
        if (reg->matchDict)
            dict_close (reg->matchDict);
	sortIdx_close (reg->sortIdx);
	if (reg->isams)
	    isams_close (reg->isams);
        if (reg->isam)
            is_close (reg->isam);
        if (reg->isamc)
            isc_close (reg->isamc);
        if (reg->isamd)
            isamd_close (reg->isamd);
        if (reg->isamb)
            isamb_close (reg->isamb);
        rec_close (&reg->records);
    }

    recTypes_destroy (reg->recTypes);
    zebra_maps_close (reg->zebra_maps);
    zebraRankDestroy (reg);
    bfs_destroy (reg->bfs);
    data1_destroy (reg->dh);

    xfree (reg->key_buf);
    xfree (reg->name);
    xfree (reg);
    yaz_log(LOG_DEBUG, "zebra_register_close 2");
}

void zebra_stop(ZebraService zs)
{
    if (!zs)
	return ;
    yaz_log (LOG_LOG, "zebra_stop");

    while (zs->sessions)
    {
        zebra_close (zs->sessions);
    }
        
    zebra_mutex_cond_destroy (&zs->session_lock);

    if (zs->passwd_db)
	passwd_db_close (zs->passwd_db);

    res_close (zs->global_res);
    xfree (zs->configName);
    xfree (zs->path_root);
    xfree (zs);
}

void zebra_close (ZebraHandle zh)
{
    ZebraService zs;
    struct zebra_session **sp;

    if (!zh)
        return;

    zs = zh->service;
    yaz_log (LOG_DEBUG, "zebra_close zh=%p", zh);
    if (!zh)
	return ;
    resultSetDestroy (zh, -1, 0, 0);


    if (zh->reg)
        zebra_register_close (zh->service, zh->reg);
    zebra_close_res (zh);

    xfree (zh->record_encoding);

    if (zh->iconv_to_utf8 != 0)
        yaz_iconv_close (zh->iconv_to_utf8);
    if (zh->iconv_from_utf8 != 0)
        yaz_iconv_close (zh->iconv_from_utf8);

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
    xfree (zh->reg_name);
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

static Res zebra_open_res (ZebraHandle zh)
{
    Res res = 0;
    char fname[512];
    if (zh->path_reg)
    {
        sprintf (fname, "%.200s/zebra.cfg", zh->path_reg);
        res = res_open (fname, zh->service->global_res);
        if (!res)
            res = zh->service->global_res;
    }
    else if (*zh->reg_name == 0)
    {
        res = zh->service->global_res;
    }
    else
    {
        yaz_log (LOG_WARN, "no register root specified");
        return 0;  /* no path for register - fail! */
    }
    return res;
}

static void zebra_close_res (ZebraHandle zh)
{
    if (zh->res != zh->service->global_res)
        res_close (zh->res);
    zh->res = 0;
}

static int zebra_select_register (ZebraHandle zh, const char *new_reg)
{
    if (zh->res && strcmp (zh->reg_name, new_reg) == 0)
        return 0;
    if (!zh->res)
    {
        assert (zh->reg == 0);
        assert (*zh->reg_name == 0);
    }
    else
    {
        if (zh->reg)
        {
            resultSetInvalidate (zh);
            zebra_register_close (zh->service, zh->reg);
            zh->reg = 0;
        }
        zebra_close_res(zh);
    }
    xfree (zh->reg_name);
    zh->reg_name = xstrdup (new_reg);

    xfree (zh->path_reg);
    zh->path_reg = 0;
    if (zh->service->path_root)
    {
        zh->path_reg = xmalloc (strlen(zh->service->path_root) + 
                                strlen(zh->reg_name) + 3);
        strcpy (zh->path_reg, zh->service->path_root);
        if (*zh->reg_name)
        {
            strcat (zh->path_reg, "/");
            strcat (zh->path_reg, zh->reg_name);
        }
    }
    zh->res = zebra_open_res (zh);
    
    if (zh->lock_normal)
        zebra_lock_destroy (zh->lock_normal);
    zh->lock_normal = 0;

    if (zh->lock_shadow)
        zebra_lock_destroy (zh->lock_shadow);
    zh->lock_shadow = 0;

    if (zh->res)
    {
        char fname[512];
        const char *lock_area  =res_get (zh->res, "lockDir");
        
        if (!lock_area && zh->path_reg)
            res_put (zh->res, "lockDir", zh->path_reg);
        sprintf (fname, "norm.%s.LCK", zh->reg_name);
        zh->lock_normal =
            zebra_lock_create (res_get(zh->res, "lockDir"), fname, 0);
        
        sprintf (fname, "shadow.%s.LCK", zh->reg_name);
        zh->lock_shadow =
            zebra_lock_create (res_get(zh->res, "lockDir"), fname, 0);

    }
    return 1;
}

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

    res_trav (zh->service->global_res, "mapdb", &info, map_basenames_func);
    
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

int zebra_select_database (ZebraHandle zh, const char *basename)
{
    return zebra_select_databases (zh, 1, &basename);
}

int zebra_select_databases (ZebraHandle zh, int num_bases,
                            const char **basenames)
{
    int i;
    const char *cp;
    int len = 0;
    char *new_reg = 0;
    
    if (num_bases < 1)
    {
        zh->errCode = 23;
        return -1;
    }
    for (i = 0; i < zh->num_basenames; i++)
        xfree (zh->basenames[i]);
    xfree (zh->basenames);
    
    zh->num_basenames = num_bases;
    zh->basenames = xmalloc (zh->num_basenames * sizeof(*zh->basenames));
    for (i = 0; i < zh->num_basenames; i++)
        zh->basenames[i] = xstrdup (basenames[i]);

    cp = strrchr(basenames[0], '/');
    if (cp)
    {
        len = cp - basenames[0];
        new_reg = xmalloc (len + 1);
        memcpy (new_reg, basenames[0], len);
        new_reg[len] = '\0';
    }
    else
        new_reg = xstrdup ("");
    for (i = 1; i<num_bases; i++)
    {
        const char *cp1;

        cp1 = strrchr (basenames[i], '/');
        if (cp)
        {
            if (!cp1)
            {
                zh->errCode = 23;
                return -1;
            }
            if (len != cp1 - basenames[i] ||
                memcmp (basenames[i], new_reg, len))
            {
                zh->errCode = 23;
                return -1;
            }
        }
        else
        {
            if (cp1)
            {
                zh->errCode = 23;
                return -1;
            }
        }
    }
    zebra_select_register (zh, new_reg);
    xfree (new_reg);
    if (!zh->res)
    {
        zh->errCode = 109;
        return -1;
    }
    if (!zh->lock_normal || !zh->lock_shadow)
    {
        zh->errCode = 2;
	return -1;
    }
    return 0;
}

void zebra_search_rpn (ZebraHandle zh, ODR decode, ODR stream,
		       Z_RPNQuery *query, const char *setname, int *hits)
{
    zh->hits = 0;
    *hits = 0;

    if (zebra_begin_read (zh))
	return;
    resultSetAddRPN (zh, decode, stream, query, 
                     zh->num_basenames, zh->basenames, setname);

    zebra_end_read (zh);

    *hits = zh->hits;
}

void zebra_records_retrieve (ZebraHandle zh, ODR stream,
			     const char *setname, Z_RecordComposition *comp,
			     oid_value input_format, int num_recs,
			     ZebraRetrievalRecord *recs)
{
    ZebraPosSet poset;
    int i, *pos_array;

    if (!zh->res)
    {
        zh->errCode = 30;
        zh->errString = odr_strdup (stream, setname);
        return;
    }
    
    zh->errCode = 0;

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
                zh->errString = odr_strdup (stream, num_str);
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
		 int *position, int *num_entries, ZebraScanEntry **entries,
		 int *is_partial)
{
    if (zebra_begin_read (zh))
    {
	*entries = 0;
	*num_entries = 0;
	return;
    }
    rpn_scan (zh, stream, zapt, attributeset,
	      zh->num_basenames, zh->basenames, position,
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

int zebra_auth (ZebraHandle zh, const char *user, const char *pass)
{
    ZebraService zs = zh->service;
    if (!zs->passwd_db || !passwd_db_auth (zs->passwd_db, user, pass))
    {
        logf(LOG_APP,"AUTHOK:%s", user?user:"ANONYMOUS");
	return 0;
    }

    logf(LOG_APP,"AUTHFAIL:%s", user?user:"ANONYMOUS");
    return 1;
}

void zebra_admin_import_begin (ZebraHandle zh, const char *database,
                               const char *record_type)
{
    if (zebra_select_database(zh, database))
        return;
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

int zebra_admin_exchange_record (ZebraHandle zh,
                                 const char *database,
                                 const char *rec_buf,
                                 size_t rec_len,
                                 const char *recid_buf, size_t recid_len,
                                 int action)
{
    int sysno = 0;
    char *rinfo = 0;
    char recid_z[256];

    if (!recid_buf || recid_len <= 0 || recid_len >= sizeof(recid_z))
        return -1;
    memcpy (recid_z, recid_buf, recid_len);
    recid_z[recid_len] = 0;

    rinfo = dict_lookup (zh->reg->matchDict, recid_z);
    if (rinfo)
    {
        if (action == 1)  /* fail if insert */
            return -1;
        memcpy (&sysno, rinfo+1, sizeof(sysno));
    }
    else
    {
        if (action == 2 || action == 3) /* fail if delete or update */
            return -1;
    }
    extract_rec_in_mem (zh, "grs.sgml", rec_buf, rec_len, database,
                        action == 3 ? 1 : 0 /* delete flag */,
                        0, &sysno, 1, 1, 0);
    if (action == 1)
    {
        dict_insert (zh->reg->matchDict, recid_z, sizeof(sysno), &sysno);
    }
    else if (action == 3)
    {
        dict_delete (zh->reg->matchDict, recid_z);
    }
    return 0;
}

void zebra_admin_create (ZebraHandle zh, const char *database)
{
    ZebraService zs;

    if (zebra_select_database (zh, database))
        return;
    zebra_begin_trans (zh);

    zs = zh->service;
    /* announce database */
    if (zebraExplain_newDatabase (zh->reg->zei, database, 0 
                                  /* explainDatabase */))
    {
	zh->errCode = 224;
	zh->errString = "database already exist";
    }
    zebra_end_trans (zh);
}

int zebra_string_norm (ZebraHandle zh, unsigned reg_id,
		       const char *input_str, int input_len,
		       char *output_str, int output_len)
{
    WRBUF wrbuf;
    if (!zh->reg->zebra_maps)
	return -1;
    wrbuf = zebra_replace(zh->reg->zebra_maps, reg_id, "",
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


void zebra_set_state (ZebraHandle zh, int val, int seqno)
{
    char state_fname[256];
    char *fname;
    long p = getpid();
    FILE *f;

    sprintf (state_fname, "state.%s.LCK", zh->reg_name);
    fname = zebra_mk_fname (res_get(zh->res, "lockDir"), state_fname);
    f = fopen (fname, "w");

    yaz_log (LOG_DEBUG, "%c %d %ld", val, seqno, p);
    fprintf (f, "%c %d %ld\n", val, seqno, p);
    fclose (f);
    xfree (fname);
}

void zebra_get_state (ZebraHandle zh, char *val, int *seqno)
{
    char state_fname[256];
    char *fname;
    FILE *f;

    sprintf (state_fname, "state.%s.LCK", zh->reg_name);
    fname = zebra_mk_fname (res_get(zh->res, "lockDir"), state_fname);
    f = fopen (fname, "r");
    *val = 'o';
    *seqno = 0;

    if (f)
    {
        fscanf (f, "%c %d", val, seqno);
        fclose (f);
    }
    xfree (fname);
}

int zebra_begin_read (ZebraHandle zh)
{
    int dirty = 0;
    char val;
    int seqno;

    assert (zh->res);

    (zh->trans_no)++;

    if (zh->trans_no != 1)
    {
        zebra_flush_reg (zh);
        return 0;
    }
#if HAVE_SYS_TIMES_H
    times (&zh->tms1);
#endif
    if (!zh->res)
    {
        (zh->trans_no)--;
        zh->errCode = 109;
        return -1;
    }
    if (!zh->lock_normal || !zh->lock_shadow)
    {
        (zh->trans_no)--;
        zh->errCode = 2;
	return -1;
    }
    zebra_get_state (zh, &val, &seqno);
    if (val == 'd')
        val = 'o';

    if (!zh->reg)
        dirty = 1;
    else if (seqno != zh->reg->seqno)
    {
        yaz_log (LOG_LOG, "reopen seqno cur/old %d/%d",
                 seqno, zh->reg->seqno);
        dirty = 1;
    }
    else if (zh->reg->last_val != val)
    {
        yaz_log (LOG_LOG, "reopen last cur/old %d/%d",
                 val, zh->reg->last_val);
        dirty = 1;
    }
    if (!dirty)
        return 0;

    if (val == 'c')
        zebra_lock_r (zh->lock_shadow);
    else
        zebra_lock_r (zh->lock_normal);
    
    if (zh->reg)
        zebra_register_close (zh->service, zh->reg);
    zh->reg = zebra_register_open (zh->service, zh->reg_name,
                                   0, val == 'c' ? 1 : 0,
                                   zh->res, zh->path_reg);
    if (!zh->reg)
    {
        zh->errCode = 109;
        return -1;
    }
    zh->reg->last_val = val;
    zh->reg->seqno = seqno;

    return 0;
}

void zebra_end_read (ZebraHandle zh)
{
    (zh->trans_no)--;

    if (zh->trans_no != 0)
        return;

#if HAVE_SYS_TIMES_H
    times (&zh->tms2);
    logf (LOG_LOG, "user/system: %ld/%ld",
                    (long) (zh->tms2.tms_utime - zh->tms1.tms_utime),
                    (long) (zh->tms2.tms_stime - zh->tms1.tms_stime));

#endif

    zebra_unlock (zh->lock_normal);
    zebra_unlock (zh->lock_shadow);
}

void zebra_begin_trans (ZebraHandle zh)
{
    int pass;
    int seqno = 0;
    char val = '?';
    const char *rval = 0;

    assert (zh->res);

    (zh->trans_no++);
    if (zh->trans_no != 1)
    {
        return;
    }
    
    yaz_log (LOG_LOG, "zebra_begin_trans");

    zh->records_inserted = 0;
    zh->records_updated = 0;
    zh->records_deleted = 0;
    zh->records_processed = 0;

#if HAVE_SYS_TIMES_H
    times (&zh->tms1);
#endif
    
    /* lock */
    if (zh->shadow_enable)
        rval = res_get (zh->res, "shadow");

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
        
        zebra_get_state (zh, &val, &seqno);
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
                BFiles bfs = bfs_create (res_get (zh->res, "shadow"),
                                         zh->path_reg);
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
    zebra_set_state (zh, 'd', seqno);

    zh->reg = zebra_register_open (zh->service, zh->reg_name,
                                   1, rval ? 1 : 0, zh->res,
                                   zh->path_reg);

    zh->reg->seqno = seqno;
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
    rval = res_get (zh->res, "shadow");

    zebraExplain_runNumberIncrement (zh->reg->zei, 1);

    zebra_flush_reg (zh);

    zebra_register_close (zh->service, zh->reg);
    zh->reg = 0;

    
    yaz_log (LOG_LOG, "Records: %7d i/u/d %d/%d/%d", 
             zh->records_processed, zh->records_inserted,
             zh->records_updated, zh->records_deleted);

    zebra_get_state (zh, &val, &seqno);
    if (val != 'd')
    {
        BFiles bfs = bfs_create (rval, zh->path_reg);
        yaz_log (LOG_LOG, "deleting shadow stuff val=%c", val);
        bf_commitClean (bfs, rval);
        bfs_destroy (bfs);
    }
    if (!rval)
        seqno++;
    zebra_set_state (zh, 'o', seqno);

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
    logf (LOG_LOG, "updating %s", zh->rGroup.path);
    repositoryUpdate (zh);    
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

int zebra_commit (ZebraHandle zh)
{
    int seqno;
    char val;
    const char *rval;
    BFiles bfs;

    if (!zh->res)
    {
        zh->errCode = 109;
        return -1;
    }
    rval = res_get (zh->res, "shadow");    
    if (!rval)
    {
        logf (LOG_WARN, "Cannot perform commit");
        logf (LOG_WARN, "No shadow area defined");
        return 0;
    }

    zebra_lock_w (zh->lock_normal);
    zebra_lock_r (zh->lock_shadow);

    bfs = bfs_create (res_get (zh->res, "register"), zh->path_reg);

    zebra_get_state (zh, &val, &seqno);

    if (rval && *rval)
        bf_cache (bfs, rval);
    if (bf_commitExists (bfs))
    {
        zebra_set_state (zh, 'c', seqno);

        logf (LOG_LOG, "commit start");
        bf_commitExec (bfs);
#ifndef WIN32
        sync ();
#endif
        logf (LOG_LOG, "commit clean");
        bf_commitClean (bfs, rval);
        seqno++;
        zebra_set_state (zh, 'o', seqno);
    }
    else
    {
        logf (LOG_LOG, "nothing to commit");
    }
    bfs_destroy (bfs);

    zebra_unlock (zh->lock_shadow);
    zebra_unlock (zh->lock_normal);
    return 0;
}

int zebra_init (ZebraHandle zh)
{
    const char *rval;
    BFiles bfs = 0;

    if (!zh->res)
    {
        zh->errCode = 109;
        return -1;
    }
    rval = res_get (zh->res, "shadow");

    bfs = bfs_create (res_get (zh->service->global_res, "register"),
                      zh->path_reg);
    if (rval && *rval)
        bf_cache (bfs, rval);
    
    bf_reset (bfs);
    bfs_destroy (bfs);
    zebra_set_state (zh, 'o', 0);
    return 0;
}

int zebra_compact (ZebraHandle zh)
{
    BFiles bfs;
    if (!zh->res)
    {
        zh->errCode = 109;
        return -1;
    }
    bfs = bfs_create (res_get (zh->res, "register"), zh->path_reg);
    inv_compact (bfs);
    bfs_destroy (bfs);
    return 0;
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

void zebra_set_group (ZebraHandle zh, struct recordGroup *rg)
{
    memcpy (&zh->rGroup, rg, sizeof(*rg));
}

void zebra_result (ZebraHandle zh, int *code, char **addinfo)
{
    *code = zh->errCode;
    *addinfo = zh->errString;
}

void zebra_shadow_enable (ZebraHandle zh, int value)
{
    zh->shadow_enable = value;
}

int zebra_record_encoding (ZebraHandle zh, const char *encoding)
{
    xfree (zh->record_encoding);
    zh->record_encoding = xstrdup (encoding);
    return 0;
}

void zebra_set_resource(ZebraHandle zh, const char *name, const char *value)
{
    res_put(zh->res, name, value);
}

const char *zebra_get_resource(ZebraHandle zh,
		                const char *name, const char *defaultvalue)
{
    return res_get_def( zh->res, name, (char *)defaultvalue);
}
