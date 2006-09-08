/* $Id: extract.c,v 1.229 2006-09-08 14:40:52 adam Exp $
   Copyright (C) 1995-2006
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "index.h"
#include "orddict.h"
#include <direntz.h>
#include <charmap.h>

#define ENCODE_BUFLEN 768
struct encode_info {
    void *encode_handle;
    void *decode_handle;
    char buf[ENCODE_BUFLEN];
};

static int log_level = 0;
static int log_level_initialized = 0;

static void zebra_init_log_level()
{
    if (!log_level_initialized)
    {
        log_level = yaz_log_module_level("extract");
        log_level_initialized = 1;
    }
}

static void extract_flushRecordKeys (ZebraHandle zh, SYSNO sysno,
                                     int cmd, zebra_rec_keys_t reckeys,
                                     zint staticrank);
static void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
                                   int cmd, zebra_rec_keys_t skp);
static void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid);
static void extract_token_add (RecWord *p);

static void encode_key_init (struct encode_info *i);
static void encode_key_write (char *k, struct encode_info *i, FILE *outf);
static void encode_key_flush (struct encode_info *i, FILE *outf);

#define USE_SHELLSORT 0

#if USE_SHELLSORT
static void shellsort(void *ar, int r, size_t s,
                      int (*cmp)(const void *a, const void *b))
{
    char *a = ar;
    char v[100];
    int h, i, j, k;
    static const int incs[16] = { 1391376, 463792, 198768, 86961, 33936,
                                  13776, 4592, 1968, 861, 336, 
                                  112, 48, 21, 7, 3, 1 };
    for ( k = 0; k < 16; k++)
        for (h = incs[k], i = h; i < r; i++)
        { 
            memcpy (v, a+s*i, s);
            j = i;
            while (j > h && (*cmp)(a + s*(j-h), v) > 0)
            {
                memcpy (a + s*j, a + s*(j-h), s);
                j -= h;
            }
            memcpy (a+s*j, v, s);
        } 
}
#endif

static void logRecord (ZebraHandle zh)
{
    ++zh->records_processed;
    if (!(zh->records_processed % 1000))
    {
        yaz_log(YLOG_LOG, "Records: "ZINT_FORMAT" i/u/d "
                ZINT_FORMAT"/"ZINT_FORMAT"/"ZINT_FORMAT, 
                zh->records_processed, zh->records_inserted, 
                zh->records_updated, zh->records_deleted);
    }
}

static void extract_add_index_string (RecWord *p, 
                                      zinfo_index_category_t cat,
                                      const char *str, int length);

static void extract_set_store_data_prepare(struct recExtractCtrl *p);

static void extract_init(struct recExtractCtrl *p, RecWord *w)
{
    w->seqno = 1;
    w->index_name = "any";
    w->index_type = 'w';
    w->extractCtrl = p;
    w->record_id = 0;
    w->section_id = 0;
    w->segment = 0;
}

static void searchRecordKey(ZebraHandle zh,
			    zebra_rec_keys_t reckeys,
                            const char *index_name,
			    const char **ws, int ws_length)
{
    int i;
    int ch = -1;
    zinfo_index_category_t cat = zinfo_index_category_index;

    for (i = 0; i<ws_length; i++)
        ws[i] = NULL;

    if (ch < 0)
        ch = zebraExplain_lookup_attr_str(zh->reg->zei, cat, '0', index_name);
    if (ch < 0)
        ch = zebraExplain_lookup_attr_str(zh->reg->zei, cat, 'p', index_name);
    if (ch < 0)
        ch = zebraExplain_lookup_attr_str(zh->reg->zei, cat, 'w', index_name);

    if (ch < 0)
	return ;

    if (zebra_rec_keys_rewind(reckeys))
    {
	zint startSeq = -1;
	const char *str;
	size_t slen;
	struct it_key key;
	zint seqno;
	while (zebra_rec_keys_read(reckeys, &str, &slen, &key))
	{
	    assert(key.len <= IT_KEY_LEVEL_MAX && key.len > 2);

	    seqno = key.mem[key.len-1];
	    
	    if (key.mem[0] == ch)
	    {
		zint woff;
		
		if (startSeq == -1)
		    startSeq = seqno;
		woff = seqno - startSeq;
		if (woff >= 0 && woff < ws_length)
		    ws[woff] = str;
	    }
	}
    }
}

#define FILE_MATCH_BLANK "\t "

static char *fileMatchStr (ZebraHandle zh,
			   zebra_rec_keys_t reckeys,
                           const char *fname, const char *spec)
{
    static char dstBuf[2048];      /* static here ??? */
    char *dst = dstBuf;
    const char *s = spec;

    while (1)
    {
	for (; *s && strchr(FILE_MATCH_BLANK, *s); s++)
	    ;
        if (!*s)
            break;
        if (*s == '(')
        {
	    const char *ws[32];
	    char attset_str[64], attname_str[64];
	    int i;
            int first = 1;
	    
	    for (s++; strchr(FILE_MATCH_BLANK, *s); s++)
		;
	    for (i = 0; *s && *s != ',' && *s != ')' && 
		     !strchr(FILE_MATCH_BLANK, *s); s++)
		if (i+1 < sizeof(attset_str))
		    attset_str[i++] = *s;
	    attset_str[i] = '\0';
	    
	    for (; strchr(FILE_MATCH_BLANK, *s); s++)
		;
	    if (*s != ',')
                strcpy(attname_str, attset_str);
            else
	    {
		for (s++; strchr(FILE_MATCH_BLANK, *s); s++)
		    ;
		for (i = 0; *s && *s != ')' && 
			 !strchr(FILE_MATCH_BLANK, *s); s++)
		    if (i+1 < sizeof(attname_str))
			attname_str[i++] = *s;
		attname_str[i] = '\0';
	    }

            searchRecordKey (zh, reckeys, attname_str, ws, 32);

            if (*s != ')')
            {
                yaz_log (YLOG_WARN, "Missing ) in match criteria %s in group %s",
                      spec, zh->m_group ? zh->m_group : "none");
                return NULL;
            }
            s++;

            for (i = 0; i<32; i++)
                if (ws[i])
                {
                    if (first)
                    {
                        *dst++ = ' ';
                        first = 0;
                    }
                    strcpy (dst, ws[i]);
                    dst += strlen(ws[i]);
                }
            if (first)
            {
                yaz_log (YLOG_WARN, "Record didn't contain match"
                      " fields in (%s,%s)", attset_str, attname_str);
                return NULL;
            }
        }
        else if (*s == '$')
        {
            int spec_len;
            char special[64];
            const char *spec_src = NULL;
            const char *s1 = ++s;
            while (*s1 && !strchr(FILE_MATCH_BLANK, *s1))
                s1++;

            spec_len = s1 - s;
            if (spec_len > sizeof(special)-1)
                spec_len = sizeof(special)-1;
            memcpy (special, s, spec_len);
            special[spec_len] = '\0';
            s = s1;

            if (!strcmp (special, "group"))
                spec_src = zh->m_group;
            else if (!strcmp (special, "database"))
                spec_src = zh->basenames[0];
            else if (!strcmp (special, "filename")) {
                spec_src = fname;
	    }
            else if (!strcmp (special, "type"))
                spec_src = zh->m_record_type;
            else 
                spec_src = NULL;
            if (spec_src)
            {
                strcpy (dst, spec_src);
                dst += strlen (spec_src);
            }
        }
        else if (*s == '\"' || *s == '\'')
        {
            int stopMarker = *s++;
            char tmpString[64];
            int i = 0;

            while (*s && *s != stopMarker)
            {
                if (i+1 < sizeof(tmpString))
                    tmpString[i++] = *s++;
            }
            if (*s)
                s++;
            tmpString[i] = '\0';
            strcpy (dst, tmpString);
            dst += strlen (tmpString);
        }
        else
        {
            yaz_log (YLOG_WARN, "Syntax error in match criteria %s in group %s",
                  spec, zh->m_group ? zh->m_group : "none");
            return NULL;
        }
        *dst++ = 1;
    }
    if (dst == dstBuf)
    {
        yaz_log (YLOG_WARN, "No match criteria for record %s in group %s",
              fname, zh->m_group ? zh->m_group : "none");
        return NULL;
    }
    *dst = '\0';
    return dstBuf;
}

struct recordLogInfo {
    const char *fname;
    int recordOffset;
    struct recordGroup *rGroup;
};

static void init_extractCtrl(ZebraHandle zh, struct recExtractCtrl *ctrl)
{
    int i;
    for (i = 0; i<256; i++)
    {
	if (zebra_maps_is_positioned(zh->reg->zebra_maps, i))
	    ctrl->seqno[i] = 1;
	else
	    ctrl->seqno[i] = 0;
    }
    ctrl->flagShowRecords = !zh->m_flag_rw;
}

static void all_matches_add(struct recExtractCtrl *ctrl)
{
    RecWord word;
    extract_init(ctrl, &word);
    word.index_name = "_ALLRECORDS";
    word.index_type = 'w';
    word.seqno = 1;
    extract_add_index_string (&word, zinfo_index_category_alwaysmatches,
                              "", 0);
}

ZEBRA_RES zebra_extract_file(ZebraHandle zh, SYSNO *sysno, const char *fname, 
			     int deleteFlag)
{
    ZEBRA_RES r = ZEBRA_OK;
    int i, fd;
    char gprefix[128];
    char ext[128];
    char ext_res[128];
    struct file_read_info *fi = 0;
    const char *original_record_type = 0;
    RecType recType;
    void *recTypeClientData;
    struct ZebraRecStream stream, *streamp;

    zebra_init_log_level();

    if (!zh->m_group || !*zh->m_group)
        *gprefix = '\0';
    else
        sprintf (gprefix, "%s.", zh->m_group);
    
    yaz_log(log_level, "zebra_extract_file %s", fname);

    /* determine file extension */
    *ext = '\0';
    for (i = strlen(fname); --i >= 0; )
        if (fname[i] == '/')
            break;
        else if (fname[i] == '.')
        {
            strcpy (ext, fname+i+1);
            break;
        }
    /* determine file type - depending on extension */
    original_record_type = zh->m_record_type;
    if (!zh->m_record_type)
    {
        sprintf (ext_res, "%srecordType.%s", gprefix, ext);
        zh->m_record_type = res_get (zh->res, ext_res);
    }
    if (!zh->m_record_type)
    {
	if (zh->records_processed < zh->m_file_verbose_limit)
            yaz_log (YLOG_LOG, "? %s", fname);
        return 0;
    }
    /* determine match criteria */
    if (!zh->m_record_id)
    {
        sprintf (ext_res, "%srecordId.%s", gprefix, ext);
        zh->m_record_id = res_get (zh->res, ext_res);
    }

    if (!(recType =
	  recType_byName (zh->reg->recTypes, zh->res, zh->m_record_type,
			  &recTypeClientData)))
    {
        yaz_log(YLOG_WARN, "No such record type: %s", zh->m_record_type);
        return ZEBRA_FAIL;
    }

    switch(recType->version)
    {
    case 0:
	break;
    default:
	yaz_log(YLOG_WARN, "Bad filter version: %s", zh->m_record_type);
    }
    if (sysno && deleteFlag)
    {
        streamp = 0;
        fi = 0;
    }
    else
    {
        char full_rep[1024];

        if (zh->path_reg && !yaz_is_abspath (fname))
        {
            strcpy (full_rep, zh->path_reg);
            strcat (full_rep, "/");
            strcat (full_rep, fname);
        }
        else
            strcpy (full_rep, fname);
        
        if ((fd = open (full_rep, O_BINARY|O_RDONLY)) == -1)
        {
            yaz_log (YLOG_WARN|YLOG_ERRNO, "open %s", full_rep);
	    zh->m_record_type = original_record_type;
            return ZEBRA_FAIL;
        }
        streamp = &stream;
        zebra_create_stream_fd(streamp, fd, 0);
    }
    while(1)
    {
        r = zebra_extract_record_stream(zh, streamp,
                                        deleteFlag,
                                        0, /* tst_mode */
                                        zh->m_record_type,
                                        sysno,
                                        0, /*match_criteria */
                                        fname,
                                        1, /* force_update */
                                        1, /* allow_update */
                                        recType, recTypeClientData);
	if (r != ZEBRA_OK)
	{
	    break;
	}
	if (sysno)
	{
	    break;
	}
    }
    if (streamp)
        stream.destroy(streamp);
    zh->m_record_type = original_record_type;
    return r;
}

/*
  If sysno is provided, then it's used to identify the reocord.
  If not, and match_criteria is provided, then sysno is guessed
  If not, and a record is provided, then sysno is got from there
  
 */

ZEBRA_RES zebra_buffer_extract_record(ZebraHandle zh, 
                                      const char *buf, size_t buf_size,
                                      int delete_flag,
                                      int test_mode, 
                                      const char *recordType,
                                      SYSNO *sysno,
                                      const char *match_criteria,
                                      const char *fname,
                                      int force_update,
                                      int allow_update)
{
    struct ZebraRecStream stream;
    ZEBRA_RES res;
    void *clientData;
    RecType recType = 0;

    if (recordType && *recordType)
    {
        yaz_log(log_level, "Record type explicitly specified: %s", recordType);
        recType = recType_byName (zh->reg->recTypes, zh->res, recordType,
                                  &clientData);
    } 
    else
    {
        if (!(zh->m_record_type))
	{
            yaz_log (YLOG_WARN, "No such record type defined");
            return ZEBRA_FAIL;
        }
        yaz_log(log_level, "Get record type from rgroup: %s",
                zh->m_record_type);
        recType = recType_byName (zh->reg->recTypes, zh->res,
				  zh->m_record_type, &clientData);
        recordType = zh->m_record_type;
    }
    
    if (!recType)
    {
        yaz_log (YLOG_WARN, "No such record type: %s", recordType);
        return ZEBRA_FAIL;
    }



    zebra_create_stream_mem(&stream, buf, buf_size);

    res = zebra_extract_record_stream(zh, &stream,
                                      delete_flag,
                                      test_mode, 
                                      recordType,
                                      sysno,
                                      match_criteria,
                                      fname,
                                      force_update,
                                      allow_update,
                                      recType, clientData);
    stream.destroy(&stream);
    return res;
}


ZEBRA_RES zebra_extract_record_stream(ZebraHandle zh, 
                                      struct ZebraRecStream *stream,
                                      int delete_flag,
                                      int test_mode, 
                                      const char *recordType,
                                      SYSNO *sysno,
                                      const char *match_criteria,
                                      const char *fname,
                                      int force_update,
                                      int allow_update,
                                      RecType recType,
                                      void *recTypeClientData)

{
    SYSNO sysno0 = 0;
    RecordAttr *recordAttr;
    struct recExtractCtrl extractCtrl;
    int r;
    const char *matchStr = 0;
    Record rec;
    off_t start_offset = 0;
    const char *pr_fname = fname;  /* filename to print .. */
    int show_progress = zh->records_processed < zh->m_file_verbose_limit ? 1:0;

    zebra_init_log_level();

    if (!pr_fname)
	pr_fname = "<no file>";  /* make it printable if file is omitted */

    zebra_rec_keys_reset(zh->reg->keys);
    zebra_rec_keys_reset(zh->reg->sortKeys);

    if (zebraExplain_curDatabase (zh->reg->zei, zh->basenames[0]))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, zh->basenames[0], 
				      zh->m_explain_database))
            return ZEBRA_FAIL;
    }

    if (stream)
    {
        off_t null_offset = 0;
        extractCtrl.stream = stream;

        start_offset = stream->tellf(stream);

        extractCtrl.first_record = start_offset ? 0 : 1;
        
        stream->endf(stream, &null_offset);;

        extractCtrl.init = extract_init;
        extractCtrl.tokenAdd = extract_token_add;
        extractCtrl.schemaAdd = extract_schema_add;
        extractCtrl.dh = zh->reg->dh;
        extractCtrl.handle = zh;
        extractCtrl.match_criteria[0] = '\0';
        extractCtrl.staticrank = 0;

    
        init_extractCtrl(zh, &extractCtrl);
        
        extract_set_store_data_prepare(&extractCtrl);
        
        r = (*recType->extract)(recTypeClientData, &extractCtrl);
        
        if (r == RECCTRL_EXTRACT_EOF)
            return ZEBRA_FAIL;
        else if (r == RECCTRL_EXTRACT_ERROR_GENERIC)
        {
            /* error occured during extraction ... */
            yaz_log (YLOG_WARN, "extract error: generic");
            return ZEBRA_FAIL;
        }
        else if (r == RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER)
        {
            /* error occured during extraction ... */
            yaz_log (YLOG_WARN, "extract error: no such filter");
            return ZEBRA_FAIL;
        }
        
        all_matches_add(&extractCtrl);
        
        if (extractCtrl.match_criteria[0])
            match_criteria = extractCtrl.match_criteria;
    }
    if (!sysno) {

	sysno = &sysno0;

        if (match_criteria && *match_criteria) {
            matchStr = match_criteria;
        } else {
            if (zh->m_record_id && *zh->m_record_id) {
                matchStr = fileMatchStr (zh, zh->reg->keys, pr_fname, 
                                         zh->m_record_id);
		if (!matchStr)
                {
                    yaz_log (YLOG_WARN, "Bad match criteria (recordID)");
		    return ZEBRA_FAIL;
                }
            }
        }
        if (matchStr) 
	{
	    int db_ord = zebraExplain_get_database_ord(zh->reg->zei);
	    char *rinfo = dict_lookup_ord(zh->reg->matchDict, db_ord,
					  matchStr);
            if (rinfo)
	    {
		assert(*rinfo == sizeof(*sysno));
                memcpy (sysno, rinfo+1, sizeof(*sysno));
	    }
        }
    }
    if (zebra_rec_keys_empty(zh->reg->keys))
    {
	/* the extraction process returned no information - the record
	   is probably empty - unless flagShowRecords is in use */
	if (test_mode)
	    return ZEBRA_OK;
    }

    if (! *sysno)
    {
        /* new record */
        if (delete_flag)
        {
	    yaz_log (YLOG_LOG, "delete %s %s " ZINT_FORMAT, recordType,
			 pr_fname, (zint) start_offset);
            yaz_log (YLOG_WARN, "cannot delete record above (seems new)");
            return ZEBRA_FAIL;
        }
	if (show_progress)
	    yaz_log (YLOG_LOG, "add %s %s " ZINT_FORMAT, recordType, pr_fname,
		     (zint) start_offset);
        rec = rec_new (zh->reg->records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zh->reg->zei, rec);
	recordAttr->staticrank = extractCtrl.staticrank;

        if (matchStr)
        {
	    int db_ord = zebraExplain_get_database_ord(zh->reg->zei);
            dict_insert_ord(zh->reg->matchDict, db_ord, matchStr,
			    sizeof(*sysno), sysno);
        }


	extract_flushSortKeys (zh, *sysno, 1, zh->reg->sortKeys);
        extract_flushRecordKeys (zh, *sysno, 1, zh->reg->keys,
			 recordAttr->staticrank);
        zh->records_inserted++;
    } 
    else
    {
        /* record already exists */
	zebra_rec_keys_t delkeys = zebra_rec_keys_open();
	zebra_rec_keys_t sortKeys = zebra_rec_keys_open();
	if (!allow_update)
	{
	    yaz_log (YLOG_LOG, "skipped %s %s " ZINT_FORMAT, 
			 recordType, pr_fname, (zint) start_offset);
	    logRecord(zh);
	    return ZEBRA_FAIL;
	}

        rec = rec_get (zh->reg->records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zh->reg->zei, rec);

	zebra_rec_keys_set_buf(delkeys,
			       rec->info[recInfo_delKeys],
			       rec->size[recInfo_delKeys],
			       0);
	zebra_rec_keys_set_buf(sortKeys,
			       rec->info[recInfo_sortKeys],
			       rec->size[recInfo_sortKeys],
			       0);

	extract_flushSortKeys (zh, *sysno, 0, sortKeys);
        extract_flushRecordKeys (zh, *sysno, 0, delkeys,
				 recordAttr->staticrank);
        if (delete_flag)
        {
            /* record going to be deleted */
            if (zebra_rec_keys_empty(delkeys))
            {
		yaz_log(YLOG_LOG, "delete %s %s " ZINT_FORMAT, recordType,
                        pr_fname, (zint) start_offset);
		yaz_log(YLOG_WARN, "cannot delete file above, "
                        "storeKeys false (3)");
	    }
            else
            {
		if (show_progress)
		    yaz_log(YLOG_LOG, "delete %s %s " ZINT_FORMAT, recordType,
                            pr_fname, (zint) start_offset);
                zh->records_deleted++;
                if (matchStr)
		{
		    int db_ord = zebraExplain_get_database_ord(zh->reg->zei);
                    dict_delete_ord(zh->reg->matchDict, db_ord, matchStr);
		}
                rec_del (zh->reg->records, &rec);
            }
	    rec_rm (&rec);
            logRecord(zh);
            return ZEBRA_OK;
        }
        else
        {
	    if (show_progress)
		    yaz_log(YLOG_LOG, "update %s %s " ZINT_FORMAT, recordType,
                            pr_fname, (zint) ZINT_FORMAT);
	    recordAttr->staticrank = extractCtrl.staticrank;
            extract_flushSortKeys (zh, *sysno, 1, zh->reg->sortKeys);
            extract_flushRecordKeys (zh, *sysno, 1, zh->reg->keys, 
					 recordAttr->staticrank);
            zh->records_updated++;
        }
	zebra_rec_keys_close(delkeys);
	zebra_rec_keys_close(sortKeys);
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
    if (!zebra_rec_keys_empty(zh->reg->keys) && zh->m_store_keys == 1)
    {
	zebra_rec_keys_get_buf(zh->reg->keys,
			       &rec->info[recInfo_delKeys],
			       &rec->size[recInfo_delKeys]);
    }
    else
    {
        rec->info[recInfo_delKeys] = NULL;
        rec->size[recInfo_delKeys] = 0;
    }
    /* update sort keys */
    xfree (rec->info[recInfo_sortKeys]);

    zebra_rec_keys_get_buf(zh->reg->sortKeys,
			   &rec->info[recInfo_sortKeys],
			   &rec->size[recInfo_sortKeys]);

    /* save file size of original record */
    zebraExplain_recordBytesIncrement (zh->reg->zei,
				       - recordAttr->recordSize);
    if (stream)
    {
        off_t end_offset = stream->endf(stream, 0);

        if (!end_offset)
            end_offset = stream->tellf(stream);
        else
            stream->seekf(stream, end_offset);

        recordAttr->recordSize = end_offset - start_offset;
        zebraExplain_recordBytesIncrement(zh->reg->zei,
                                          recordAttr->recordSize);
    }

    /* set run-number for this record */
    recordAttr->runNumber =
	zebraExplain_runNumberIncrement (zh->reg->zei, 0);

    /* update store data */
    xfree (rec->info[recInfo_storeData]);

    /* update store data */
    if (zh->store_data_buf)
    {
        rec->size[recInfo_storeData] = zh->store_data_size;
        rec->info[recInfo_storeData] = zh->store_data_buf;
	zh->store_data_buf = 0;
    }
    else if (zh->m_store_data)
    {
        off_t cur_offset = stream->tellf(stream);

        rec->size[recInfo_storeData] = recordAttr->recordSize;
        rec->info[recInfo_storeData] = (char *)
	    xmalloc (recordAttr->recordSize);
        stream->seekf(stream, start_offset);
        stream->readf(stream, rec->info[recInfo_storeData],
                      recordAttr->recordSize);
        stream->seekf(stream, cur_offset);
    }
    else
    {
        rec->info[recInfo_storeData] = NULL;
        rec->size[recInfo_storeData] = 0;
    }
    /* update database name */
    xfree (rec->info[recInfo_databaseName]);
    rec->info[recInfo_databaseName] =
        rec_strdup (zh->basenames[0], &rec->size[recInfo_databaseName]); 

    /* update offset */
    recordAttr->recordOffset = start_offset;
    
    /* commit this record */
    rec_put (zh->reg->records, &rec);
    logRecord(zh);
    return ZEBRA_OK;
}

ZEBRA_RES zebra_extract_explain(void *handle, Record rec, data1_node *n)
{
    ZebraHandle zh = (ZebraHandle) handle;
    struct recExtractCtrl extractCtrl;

    if (zebraExplain_curDatabase (zh->reg->zei,
				  rec->info[recInfo_databaseName]))
    {
	abort();
        if (zebraExplain_newDatabase (zh->reg->zei,
				      rec->info[recInfo_databaseName], 0))
            abort ();
    }

    zebra_rec_keys_reset(zh->reg->keys);
    zebra_rec_keys_reset(zh->reg->sortKeys);

    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->reg->dh;

    init_extractCtrl(zh, &extractCtrl);

    extractCtrl.flagShowRecords = 0;
    extractCtrl.match_criteria[0] = '\0';
    extractCtrl.staticrank = 0;
    extractCtrl.handle = handle;
    extractCtrl.first_record = 1;
    
    extract_set_store_data_prepare(&extractCtrl);

    if (n)
	grs_extract_tree(&extractCtrl, n);

    if (rec->size[recInfo_delKeys])
    {
	zebra_rec_keys_t delkeys = zebra_rec_keys_open();
	
	zebra_rec_keys_t sortkeys = zebra_rec_keys_open();

	zebra_rec_keys_set_buf(delkeys, rec->info[recInfo_delKeys],
			       rec->size[recInfo_delKeys],
			       0);
	extract_flushRecordKeys (zh, rec->sysno, 0, delkeys, 0);
	zebra_rec_keys_close(delkeys);

	zebra_rec_keys_set_buf(sortkeys, rec->info[recInfo_sortKeys],
			       rec->size[recInfo_sortKeys],
			       0);

	extract_flushSortKeys (zh, rec->sysno, 0, sortkeys);
	zebra_rec_keys_close(sortkeys);
    }
    extract_flushRecordKeys (zh, rec->sysno, 1, zh->reg->keys, 0);
    extract_flushSortKeys (zh, rec->sysno, 1, zh->reg->sortKeys);

    xfree (rec->info[recInfo_delKeys]);
    zebra_rec_keys_get_buf(zh->reg->keys,
			   &rec->info[recInfo_delKeys],	
			   &rec->size[recInfo_delKeys]);

    xfree (rec->info[recInfo_sortKeys]);
    zebra_rec_keys_get_buf(zh->reg->sortKeys,
			   &rec->info[recInfo_sortKeys],
			   &rec->size[recInfo_sortKeys]);
    return ZEBRA_OK;
}

void extract_rec_keys_adjust(ZebraHandle zh, int is_insert,
                             zebra_rec_keys_t reckeys)
{
    ZebraExplainInfo zei = zh->reg->zei;
    struct ord_stat {
        int no;
        int ord;
        struct ord_stat *next;
    };

    if (zebra_rec_keys_rewind(reckeys))
    {
        struct ord_stat *ord_list = 0;
        struct ord_stat *p;
	size_t slen;
	const char *str;
	struct it_key key_in;
	while(zebra_rec_keys_read(reckeys, &str, &slen, &key_in))
        {
            int ord = CAST_ZINT_TO_INT(key_in.mem[0]);

            for (p = ord_list; p ; p = p->next)
                if (p->ord == ord)
                {
                    p->no++;
                    break;
                }
            if (!p)
            {
                p = xmalloc(sizeof(*p));
                p->no = 1;
                p->ord = ord;
                p->next = ord_list;
                ord_list = p;
            }
        }

        p = ord_list;
        while (p)
        {
            struct ord_stat *p1 = p;

            if (is_insert)
                zebraExplain_ord_adjust_occurrences(zei, p->ord, p->no, 1);
            else
                zebraExplain_ord_adjust_occurrences(zei, p->ord, - p->no, -1);
            p = p->next;
            xfree(p1);
        }
    }
}

void extract_flushRecordKeys (ZebraHandle zh, SYSNO sysno,
                              int cmd,
			      zebra_rec_keys_t reckeys,
			      zint staticrank)
{
    ZebraExplainInfo zei = zh->reg->zei;

    extract_rec_keys_adjust(zh, cmd, reckeys);

    if (!zh->reg->key_buf)
    {
	int mem= 1024*1024* atoi( res_get_def( zh->res, "memmax", "8"));
	if (mem <= 0)
	{
	    yaz_log(YLOG_WARN, "Invalid memory setting, using default 8 MB");
	    mem= 1024*1024*8;
	}
	/* FIXME: That "8" should be in a default settings include */
	/* not hard-coded here! -H */
	zh->reg->key_buf = (char**) xmalloc (mem);
	zh->reg->ptr_top = mem/sizeof(char*);
	zh->reg->ptr_i = 0;
	zh->reg->key_buf_used = 0;
	zh->reg->key_file_no = 0;
    }
    zebraExplain_recordCountIncrement (zei, cmd ? 1 : -1);

    if (zebra_rec_keys_rewind(reckeys))
    {
	size_t slen;
	const char *str;
	struct it_key key_in;
	while(zebra_rec_keys_read(reckeys, &str, &slen, &key_in))
	{
	    int ch = 0;
            int i, j = 0;
	    struct it_key key_out;

	    assert(key_in.len >= 2);
            assert(key_in.len <= IT_KEY_LEVEL_MAX);
	    
	    /* check for buffer overflow */
	    if (zh->reg->key_buf_used + 1024 > 
		(zh->reg->ptr_top -zh->reg->ptr_i)*sizeof(char*))
		extract_flushWriteKeys (zh, 0);
	    
	    ++(zh->reg->ptr_i);
	    assert(zh->reg->ptr_i > 0);
	    (zh->reg->key_buf)[zh->reg->ptr_top - zh->reg->ptr_i] =
		(char*)zh->reg->key_buf + zh->reg->key_buf_used;

            /* key_in.mem[0] ord/ch */
            /* key_in.mem[1] filter specified record ID */

	    /* encode the ordinal value (field/use/attribute) .. */
	    ch = CAST_ZINT_TO_INT(key_in.mem[0]);
	    zh->reg->key_buf_used +=
		key_SU_encode(ch, (char*)zh->reg->key_buf +
			      zh->reg->key_buf_used);

	    /* copy the 0-terminated stuff from str to output */
	    memcpy((char*)zh->reg->key_buf + zh->reg->key_buf_used, str, slen);
	    zh->reg->key_buf_used += slen;
	    ((char*)zh->reg->key_buf)[(zh->reg->key_buf_used)++] = '\0';

	    /* the delete/insert indicator */
	    ((char*)zh->reg->key_buf)[(zh->reg->key_buf_used)++] = cmd;

	    if (zh->m_staticrank) /* rank config enabled ? */
	    {
		if (staticrank < 0)
		{
		    yaz_log(YLOG_WARN, "staticrank = %ld. Setting to 0",
			    (long) staticrank);
		    staticrank = 0;
		}
                key_out.mem[j++] = staticrank;
	    }
	    
	    if (key_in.mem[1]) /* filter specified record ID */
		key_out.mem[j++] = key_in.mem[1];
	    else
		key_out.mem[j++] = sysno;
            for (i = 2; i < key_in.len; i++)
                key_out.mem[j++] = key_in.mem[i];
	    key_out.len = j;

	    memcpy((char*)zh->reg->key_buf + zh->reg->key_buf_used,
		   &key_out, sizeof(key_out));
	    (zh->reg->key_buf_used) += sizeof(key_out);
	}
    }
}

void extract_flushWriteKeys (ZebraHandle zh, int final)
        /* optimizing: if final=1, and no files written yet */
        /* push the keys directly to merge, sidestepping the */
        /* temp file altogether. Speeds small updates */
{
    FILE *outf;
    char out_fname[200];
    char *prevcp, *cp;
    struct encode_info encode_info;
    int ptr_i = zh->reg->ptr_i;
    int temp_policy;
#if SORT_EXTRA
    int i;
#endif
    if (!zh->reg->key_buf || ptr_i <= 0)
    {
        yaz_log(log_level, "  nothing to flush section=%d buf=%p i=%d",
               zh->reg->key_file_no, zh->reg->key_buf, ptr_i);
        return;
    }

    (zh->reg->key_file_no)++;
    yaz_log (YLOG_LOG, "sorting section %d", (zh->reg->key_file_no));
    yaz_log(log_level, "  sort_buff at %p n=%d",
                    zh->reg->key_buf + zh->reg->ptr_top - ptr_i,ptr_i);
#if !SORT_EXTRA
    qsort (zh->reg->key_buf + zh->reg->ptr_top - ptr_i, ptr_i,
               sizeof(char*), key_qsort_compare);

    /* zebra.cfg: tempfiles:  
       Y: always use temp files (old way) 
       A: use temp files, if more than one (auto) 
          = if this is both the last and the first 
       N: never bother with temp files (new) */

    temp_policy=toupper(res_get_def(zh->res,"tempfiles","auto")[0]);
    if (temp_policy != 'Y' && temp_policy != 'N' && temp_policy != 'A') {
        yaz_log (YLOG_WARN, "Illegal tempfiles setting '%c'. using 'Auto' ", 
                        temp_policy);
        temp_policy='A';
    }

    if (   ( temp_policy =='N' )   ||     /* always from memory */
         ( ( temp_policy =='A' ) &&       /* automatic */
             (zh->reg->key_file_no == 1) &&  /* this is first time */
             (final) ) )                     /* and last (=only) time */
    { /* go directly from memory */
        zh->reg->key_file_no =0; /* signal not to read files */
        zebra_index_merge(zh); 
        zh->reg->ptr_i = 0;
        zh->reg->key_buf_used = 0; 
        return; 
    }

    /* Not doing directly from memory, write into a temp file */
    extract_get_fname_tmp (zh, out_fname, zh->reg->key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fopen %s", out_fname);
        exit (1);
    }
    yaz_log (YLOG_LOG, "writing section %d", zh->reg->key_file_no);
    prevcp = cp = (zh->reg->key_buf)[zh->reg->ptr_top - ptr_i];
    
    encode_key_init (&encode_info);
    encode_key_write (cp, &encode_info, outf);
    
    while (--ptr_i > 0)
    {
        cp = (zh->reg->key_buf)[zh->reg->ptr_top - ptr_i];
        if (strcmp (cp, prevcp))
        {
            encode_key_flush ( &encode_info, outf);
            encode_key_init (&encode_info);
            encode_key_write (cp, &encode_info, outf);
            prevcp = cp;
        }
        else
            encode_key_write (cp + strlen(cp), &encode_info, outf);
    }
    encode_key_flush ( &encode_info, outf);
#else
    qsort (key_buf + ptr_top-ptr_i, ptr_i, sizeof(char*), key_x_compare);
    extract_get_fname_tmp (out_fname, key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fopen %s", out_fname);
        exit (1);
    }
    yaz_log (YLOG_LOG, "writing section %d", key_file_no);
    i = ptr_i;
    prevcp =  key_buf[ptr_top-i];
    while (1)
        if (!--i || strcmp (prevcp, key_buf[ptr_top-i]))
        {
            key_y_len = strlen(prevcp)+1;
#if 0
            yaz_log (YLOG_LOG, "key_y_len: %2d %02x %02x %s",
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
            encode_key_flush ( &encode_info, outf);
            if (!i)
                break;
            prevcp = key_buf[ptr_top-ptr_i];
        }
#endif
    if (fclose (outf))
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fclose %s", out_fname);
        exit (1);
    }
    yaz_log (YLOG_LOG, "finished section %d", zh->reg->key_file_no);
    zh->reg->ptr_i = 0;
    zh->reg->key_buf_used = 0;
}

ZEBRA_RES zebra_snippets_rec_keys(ZebraHandle zh,
				  zebra_rec_keys_t reckeys,
				  zebra_snippets *snippets)
{
    NMEM nmem = nmem_create();
    if (zebra_rec_keys_rewind(reckeys)) 
    {
	const char *str;
	size_t slen;
	struct it_key key;
	while (zebra_rec_keys_read(reckeys, &str, &slen, &key))
	{
	    char dst_buf[IT_MAX_WORD];
	    char *dst_term = dst_buf;
	    int ord;
            zint seqno;
	    int index_type;

	    assert(key.len <= IT_KEY_LEVEL_MAX && key.len > 2);
	    seqno = key.mem[key.len-1];
	    ord = CAST_ZINT_TO_INT(key.mem[0]);
	    
	    zebraExplain_lookup_ord(zh->reg->zei, ord, &index_type,
				    0/* db */, 0 /* string_index */);
	    assert(index_type);
	    zebra_term_untrans_iconv(zh, nmem, index_type,
				     &dst_term, str);
	    zebra_snippets_append(snippets, seqno, ord, dst_term);
	    nmem_reset(nmem);
	}
    }
    nmem_destroy(nmem);
    return ZEBRA_OK;
}

void print_rec_keys(ZebraHandle zh, zebra_rec_keys_t reckeys)
{
    yaz_log(YLOG_LOG, "print_rec_keys");
    if (zebra_rec_keys_rewind(reckeys))
    {
	const char *str;
	size_t slen;
	struct it_key key;
	while (zebra_rec_keys_read(reckeys, &str, &slen, &key))
	{
	    char dst_buf[IT_MAX_WORD];
	    zint seqno;
	    int index_type;
            int ord = CAST_ZINT_TO_INT(key.mem[0]);
	    const char *db = 0;
	    assert(key.len <= IT_KEY_LEVEL_MAX && key.len > 2);

	    zebraExplain_lookup_ord(zh->reg->zei, ord, &index_type, &db, 0);
	    
	    seqno = key.mem[key.len-1];
	    
	    zebra_term_untrans(zh, index_type, dst_buf, str);
	    
	    yaz_log(YLOG_LOG, "ord=%d seqno=" ZINT_FORMAT 
                    " term=%s", ord, seqno, dst_buf); 
	}
    }
}

static void extract_add_index_string(RecWord *p, zinfo_index_category_t cat,
                                     const char *str, int length)
{
    struct it_key key;
    ZebraHandle zh = p->extractCtrl->handle;
    ZebraExplainInfo zei = zh->reg->zei;
    int ch, i;

    ch = zebraExplain_lookup_attr_str(zei, cat, p->index_type, p->index_name);
    if (ch < 0)
        ch = zebraExplain_add_attr_str(zei, cat, p->index_type, p->index_name);

    i = 0;
    key.mem[i++] = ch;
    key.mem[i++] = p->record_id;
    key.mem[i++] = p->section_id;

    if (zh->m_segment_indexing)
        key.mem[i++] = p->segment;
    key.mem[i++] = p->seqno;
    key.len = i;

    zebra_rec_keys_write(zh->reg->keys, str, length, &key);
}

static void extract_add_sort_string(RecWord *p, const char *str, int length)
{
    struct it_key key;
    ZebraHandle zh = p->extractCtrl->handle;
    ZebraExplainInfo zei = zh->reg->zei;
    int ch;
    zinfo_index_category_t cat = zinfo_index_category_sort;

    ch = zebraExplain_lookup_attr_str(zei, cat, p->index_type, p->index_name);
    if (ch < 0)
        ch = zebraExplain_add_attr_str(zei, cat, p->index_type, p->index_name);
    key.len = 2;
    key.mem[0] = ch;
    key.mem[1] = p->record_id;

    zebra_rec_keys_write(zh->reg->sortKeys, str, length, &key);
}

static void extract_add_string(RecWord *p, const char *string, int length)
{
    ZebraHandle zh = p->extractCtrl->handle;
    assert (length > 0);

    if (!p->index_name)
        return;

    if (zebra_maps_is_sort(zh->reg->zebra_maps, p->index_type))
	extract_add_sort_string(p, string, length);
    else
    {
	extract_add_index_string(p, zinfo_index_category_index,
                                 string, length);
        if (zebra_maps_is_alwaysmatches(zh->reg->zebra_maps, p->index_type))
        {
            RecWord word;
            memcpy(&word, p, sizeof(word));

            word.seqno = 1;
            extract_add_index_string(
                &word, zinfo_index_category_alwaysmatches, "", 0);
        }
    }
}

static void extract_add_incomplete_field(RecWord *p)
{
    ZebraHandle zh = p->extractCtrl->handle;
    const char *b = p->term_buf;
    int remain = p->term_len;
    const char **map = 0;
    
    if (remain > 0)
	map = zebra_maps_input(zh->reg->zebra_maps, p->index_type, &b, remain, 0);

    if (map)
    {   
        if (zebra_maps_is_first_in_field(zh->reg->zebra_maps, p->index_type))
        {
             /* first in field marker */
            extract_add_string(p, FIRST_IN_FIELD_STR, FIRST_IN_FIELD_LEN);
            p->seqno++;
        }
    }
    while (map)
    {
	char buf[IT_MAX_WORD+1];
	int i, remain;

	/* Skip spaces */
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->term_len - (b - p->term_buf);
	    if (remain > 0)
		map = zebra_maps_input(zh->reg->zebra_maps, p->index_type, &b,
				       remain, 0);
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
	    remain = p->term_len - (b - p->term_buf);
	    if (remain > 0)
		map = zebra_maps_input(zh->reg->zebra_maps, p->index_type, &b, remain, 0);
	    else
		map = 0;
	}
	if (!i)
	    return;
	extract_add_string (p, buf, i);
        p->seqno++;
    }
}

static void extract_add_complete_field (RecWord *p)
{
    ZebraHandle zh = p->extractCtrl->handle;
    const char *b = p->term_buf;
    char buf[IT_MAX_WORD+1];
    const char **map = 0;
    int i = 0, remain = p->term_len;

    if (remain > 0)
	map = zebra_maps_input (zh->reg->zebra_maps, p->index_type, &b, remain, 1);

    while (remain > 0 && i < IT_MAX_WORD)
    {
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->term_len - (b - p->term_buf);

	    if (remain > 0)
	    {
		int first = i ? 0 : 1;  /* first position */
		map = zebra_maps_input(zh->reg->zebra_maps, p->index_type, &b, remain, first);
	    }
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

	    if (**map == *CHR_CUT)
	    {
		i = 0;
	    }
	    else
	    {
		if (i >= IT_MAX_WORD)
		    break;
		while (i < IT_MAX_WORD && *cp)
		    buf[i++] = *(cp++);
	    }
	    remain = p->term_len  - (b - p->term_buf);
	    if (remain > 0)
	    {
		map = zebra_maps_input (zh->reg->zebra_maps, p->index_type, &b,
					remain, 0);
	    }
	    else
		map = 0;
	}
    }
    if (!i)
	return;
    extract_add_string (p, buf, i);
}

static void extract_token_add(RecWord *p)
{
    ZebraHandle zh = p->extractCtrl->handle;
    WRBUF wrbuf;

    if (log_level)
    {
        yaz_log(log_level, "extract_token_add "
                "type=%c index=%s seqno=" ZINT_FORMAT " s=%.*s",
                p->index_type, p->index_name, 
                p->seqno, p->term_len, p->term_buf);
    }
    if ((wrbuf = zebra_replace(zh->reg->zebra_maps, p->index_type, 0,
			       p->term_buf, p->term_len)))
    {
	p->term_buf = wrbuf_buf(wrbuf);
	p->term_len = wrbuf_len(wrbuf);
    }
    if (zebra_maps_is_complete (zh->reg->zebra_maps, p->index_type))
	extract_add_complete_field (p);
    else
	extract_add_incomplete_field(p);
}

static void extract_set_store_data_cb(struct recExtractCtrl *p,
				      void *buf, size_t sz)
{
    ZebraHandle zh = (ZebraHandle) p->handle;

    xfree(zh->store_data_buf);
    zh->store_data_buf = 0;
    zh->store_data_size = 0;
    if (buf && sz)
    {
	zh->store_data_buf = xmalloc(sz);
	zh->store_data_size = sz;
	memcpy(zh->store_data_buf, buf, sz);
    }
}

static void extract_set_store_data_prepare(struct recExtractCtrl *p)
{
    ZebraHandle zh = (ZebraHandle) p->handle;
    xfree(zh->store_data_buf);
    zh->store_data_buf = 0;
    zh->store_data_size = 0;
    p->setStoreData = extract_set_store_data_cb;
}

static void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid)
{
    ZebraHandle zh = (ZebraHandle) p->handle;
    zebraExplain_addSchema (zh->reg->zei, oid);
}

void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
                            int cmd, zebra_rec_keys_t reckeys)
{
    if (zebra_rec_keys_rewind(reckeys))
    {
        SortIdx sortIdx = zh->reg->sortIdx;
	size_t slen;
	const char *str;
	struct it_key key_in;

        sortIdx_sysno (sortIdx, sysno);

	while (zebra_rec_keys_read(reckeys, &str, &slen, &key_in))
        {
            int ord = CAST_ZINT_TO_INT(key_in.mem[0]);
            
            sortIdx_type(sortIdx, ord);
            if (cmd == 1)
                sortIdx_add(sortIdx, str, slen);
            else
                sortIdx_add(sortIdx, "", 1);
        }
    }
}

static void encode_key_init(struct encode_info *i)
{
    i->encode_handle = iscz1_start();
    i->decode_handle = iscz1_start();
}

static void encode_key_write (char *k, struct encode_info *i, FILE *outf)
{
    struct it_key key;
    char *bp = i->buf, *bp0;
    const char *src = (char *) &key;

    /* copy term to output buf */
    while ((*bp++ = *k++))
        ;
    /* and copy & align key so we can mangle */
    memcpy (&key, k+1, sizeof(struct it_key));  /* *k is insert/delete */

#if 0
    /* debugging */
    key_logdump_txt(YLOG_LOG, &key, *k ? "i" : "d");
#endif
    assert(key.mem[0] >= 0);

    bp0 = bp++;
    iscz1_encode(i->encode_handle, &bp, &src);

    *bp0 = (*k * 128) + bp - bp0 - 1; /* length and insert/delete combined */
    if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fwrite");
        exit (1);
    }

#if 0
    /* debugging */
    if (1)
    {
	struct it_key key2;
	const char *src = bp0+1;
	char *dst = (char*) &key2;
	iscz1_decode(i->decode_handle, &dst, &src);

	key_logdump_txt(YLOG_LOG, &key2, *k ? "i" : "d");

	assert(key2.mem[1]);
    }
#endif
}

static void encode_key_flush (struct encode_info *i, FILE *outf)
{ 
    iscz1_stop(i->encode_handle);
    iscz1_stop(i->decode_handle);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

