/* $Id: extract.c,v 1.201 2006-02-08 13:45:44 adam Exp $
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
#include <ctype.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "index.h"
#include <direntz.h>
#include <charmap.h>

#if _FILE_OFFSET_BITS == 64
#define PRINTF_OFF_T "%Ld"
#else
#define PRINTF_OFF_T "%ld"
#endif

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
        yaz_log (YLOG_LOG, "Records: "ZINT_FORMAT" i/u/d "
			ZINT_FORMAT"/"ZINT_FORMAT"/"ZINT_FORMAT, 
              zh->records_processed, zh->records_inserted, zh->records_updated,
              zh->records_deleted);
    }
}

static void extract_set_store_data_prepare(struct recExtractCtrl *p);

static void extract_init (struct recExtractCtrl *p, RecWord *w)
{
    w->zebra_maps = p->zebra_maps;
    w->seqno = 1;
#if NATTR
#else
    w->attrSet = VAL_BIB1;
    w->attrUse = 1016;
#endif
    w->index_name = 0;
    w->index_type = 'w';
    w->extractCtrl = p;
    w->record_id = 0;
    w->section_id = 0;
}

static void searchRecordKey(ZebraHandle zh,
			    zebra_rec_keys_t reckeys,
			    int attrSetS, int attrUseS,
			    const char **ws, int ws_length)
{
    int i;
    int ch;

    for (i = 0; i<ws_length; i++)
        ws[i] = NULL;

    ch = zebraExplain_lookup_attr_su_any_index(zh->reg->zei,
					       attrSetS, attrUseS);
    if (ch < 0)
	return ;

    if (zebra_rec_keys_rewind(reckeys))
    {
	int startSeq = -1;
	const char *str;
	size_t slen;
	struct it_key key;
	zint seqno;
	while (zebra_rec_keys_read(reckeys, &str, &slen, &key))
	{
	    assert(key.len <= 4 && key.len > 2);

	    seqno = key.mem[key.len-1];
	    
	    if (key.mem[0] == ch)
	    {
		int woff;
		
		if (startSeq == -1)
		    startSeq = seqno;
		woff = seqno - startSeq;
		if (woff >= 0 && woff < ws_length)
		    ws[woff] = str;
	    }
	}
    }
}

struct file_read_info {
    off_t file_max;	    /* maximum offset so far */
    off_t file_offset;      /* current offset */
    off_t file_moffset;     /* offset of rec/rec boundary */
    int file_more;
    int fd;
};

static struct file_read_info *file_read_start (int fd)
{
    struct file_read_info *fi = (struct file_read_info *)
	xmalloc (sizeof(*fi));

    fi->fd = fd;
    fi->file_max = 0;
    fi->file_moffset = 0;
    fi->file_offset = 0;
    fi->file_more = 0;
    return fi;
}

static void file_read_stop (struct file_read_info *fi)
{
    xfree (fi);
}

static off_t file_seek (void *handle, off_t offset)
{
    struct file_read_info *p = (struct file_read_info *) handle;
    p->file_offset = offset;
    return lseek (p->fd, offset, SEEK_SET);
}

static off_t file_tell (void *handle)
{
    struct file_read_info *p = (struct file_read_info *) handle;
    return p->file_offset;
}

static int file_read (void *handle, char *buf, size_t count)
{
    struct file_read_info *p = (struct file_read_info *) handle;
    int fd = p->fd;
    int r;
    r = read (fd, buf, count);
    if (r > 0)
    {
        p->file_offset += r;
        if (p->file_offset > p->file_max)
            p->file_max = p->file_offset;
    }
    return r;
}

static void file_end (void *handle, off_t offset)
{
    struct file_read_info *p = (struct file_read_info *) handle;

    if (offset != p->file_moffset)
    {
	p->file_moffset = offset;
	p->file_more = 1;
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
	    data1_attset *attset;
	    int i;
            int attSet = 1, attUse = 1;
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
	    if (*s == ',')
	    {
		for (s++; strchr(FILE_MATCH_BLANK, *s); s++)
		    ;
		for (i = 0; *s && *s != ')' && 
			 !strchr(FILE_MATCH_BLANK, *s); s++)
		    if (i+1 < sizeof(attname_str))
			attname_str[i++] = *s;
		attname_str[i] = '\0';
	    }
	    
	    if ((attset = data1_get_attset (zh->reg->dh, attset_str)))
	    {
		data1_att *att;
		attSet = attset->reference;
		att = data1_getattbyname(zh->reg->dh, attset, attname_str);
		if (att)
		    attUse = att->value;
		else
		    attUse = atoi (attname_str);
	    }
            searchRecordKey (zh, reckeys, attSet, attUse, ws, 32);

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
    ctrl->zebra_maps = zh->reg->zebra_maps;
    ctrl->flagShowRecords = !zh->m_flag_rw;
}

static int file_extract_record(ZebraHandle zh,
			       SYSNO *sysno, const char *fname,
			       int deleteFlag,
			       struct file_read_info *fi,
			       int force_update,
			       RecType recType,
			       void *recTypeClientData)
{
    RecordAttr *recordAttr;
    int r;
    const char *matchStr = 0;
    SYSNO sysnotmp;
    Record rec;
    off_t recordOffset = 0;
    struct recExtractCtrl extractCtrl;
    
    /* announce database */
    if (zebraExplain_curDatabase (zh->reg->zei, zh->basenames[0]))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, zh->basenames[0],
				      zh->m_explain_database))
	    return 0;
    }

    if (fi->fd != -1)
    {
        /* we are going to read from a file, so prepare the extraction */
	zebra_rec_keys_reset(zh->reg->keys);

#if NATTR
	zebra_rec_keys_reset(zh->reg->sortKeys);
#else
	zh->reg->sortKeys.buf_used = 0;
#endif
	recordOffset = fi->file_moffset;
        extractCtrl.handle = zh;
	extractCtrl.offset = fi->file_moffset;
	extractCtrl.readf = file_read;
	extractCtrl.seekf = file_seek;
	extractCtrl.tellf = file_tell;
	extractCtrl.endf = file_end;
	extractCtrl.fh = fi;
	extractCtrl.init = extract_init;
	extractCtrl.tokenAdd = extract_token_add;
	extractCtrl.schemaAdd = extract_schema_add;
	extractCtrl.dh = zh->reg->dh;
	extractCtrl.match_criteria[0] = '\0';
	extractCtrl.staticrank = 0;
	
	extractCtrl.first_record = fi->file_offset ? 0 : 1;

	extract_set_store_data_prepare(&extractCtrl);

	init_extractCtrl(zh, &extractCtrl);

        if (!zh->m_flag_rw)
            printf ("File: %s " PRINTF_OFF_T "\n", fname, recordOffset);
        if (zh->m_flag_rw)
        {
            char msg[512];
            sprintf (msg, "%s:" PRINTF_OFF_T , fname, recordOffset);
            yaz_log_init_prefix2 (msg);
        }

        r = (*recType->extract)(recTypeClientData, &extractCtrl);

        yaz_log_init_prefix2 (0);
	if (r == RECCTRL_EXTRACT_EOF)
	    return 0;
	else if (r == RECCTRL_EXTRACT_ERROR_GENERIC)
	{
            /* error occured during extraction ... */
            if (zh->m_flag_rw &&
		zh->records_processed < zh->m_file_verbose_limit)
            {
                yaz_log (YLOG_WARN, "fail %s %s " PRINTF_OFF_T, zh->m_record_type,
                      fname, recordOffset);
            }
            return 0;
        }
	else if (r == RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER)
	{
            /* error occured during extraction ... */
            if (zh->m_flag_rw &&
		zh->records_processed < zh->m_file_verbose_limit)
            {
                yaz_log (YLOG_WARN, "no filter for %s %s " 
                      PRINTF_OFF_T, zh->m_record_type,
                      fname, recordOffset);
            }
            return 0;
        }
        if (extractCtrl.match_criteria[0])
            matchStr = extractCtrl.match_criteria;	
    }

    /* perform match if sysno not known and if match criteria is specified */
    if (!sysno) 
    {
        sysnotmp = 0;
        sysno = &sysnotmp;

        if (matchStr == 0 && zh->m_record_id && *zh->m_record_id)
        {
            matchStr = fileMatchStr (zh, zh->reg->keys, fname, 
				     zh->m_record_id);
	    if (!matchStr)
	    {
		yaz_log(YLOG_WARN, "Bad match criteria");
		return 0;
	    }
	}
	if (matchStr)
	{
            char *rinfo = dict_lookup (zh->reg->matchDict, matchStr);
	    if (rinfo)
	    {
		assert(*rinfo == sizeof(*sysno));
		memcpy (sysno, rinfo+1, sizeof(*sysno));
	    }
	}
    }
    if (! *sysno && zebra_rec_keys_empty(zh->reg->keys)	)
    {
         /* the extraction process returned no information - the record
            is probably empty - unless flagShowRecords is in use */
         if (!zh->m_flag_rw)
             return 1;
  
         if (zh->records_processed < zh->m_file_verbose_limit)
	     yaz_log (YLOG_WARN, "empty %s %s " PRINTF_OFF_T, zh->m_record_type,
	    fname, recordOffset);
         return 1;
    }

    if (! *sysno)
    {
        /* new record */
        if (deleteFlag)
        {
	    yaz_log (YLOG_LOG, "delete %s %s " PRINTF_OFF_T, zh->m_record_type,
		  fname, recordOffset);
            yaz_log (YLOG_WARN, "cannot delete record above (seems new)");
            return 1;
        }
        if (zh->records_processed < zh->m_file_verbose_limit)
            yaz_log (YLOG_LOG, "add %s %s " PRINTF_OFF_T, zh->m_record_type,
                  fname, recordOffset);
        rec = rec_new (zh->reg->records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zh->reg->zei, rec);
	recordAttr->staticrank = extractCtrl.staticrank;

        if (matchStr)
        {
            dict_insert (zh->reg->matchDict, matchStr, sizeof(*sysno), sysno);
        }
#if NATTR
	extract_flushSortKeys (zh, *sysno, 1, zh->reg->sortKeys);
#else
	extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
#endif
        extract_flushRecordKeys (zh, *sysno, 1, zh->reg->keys,
				 recordAttr->staticrank);
        zh->records_inserted++;
    }
    else
    {
        /* record already exists */
	zebra_rec_keys_t delkeys = zebra_rec_keys_open();

#if NATTR
	zebra_rec_keys_t sortKeys = zebra_rec_keys_open();
#else
        struct sortKeys sortKeys;
#endif

        rec = rec_get (zh->reg->records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zh->reg->zei, rec);

	zebra_rec_keys_set_buf(delkeys,
			       rec->info[recInfo_delKeys],
			       rec->size[recInfo_delKeys],
			       0);

#if NATTR
	zebra_rec_keys_set_buf(sortKeys,
			       rec->info[recInfo_sortKeys],
			       rec->size[recInfo_sortKeys],
			       0);
	extract_flushSortKeys (zh, *sysno, 0, sortKeys);
#else
        sortKeys.buf_used = rec->size[recInfo_sortKeys];
        sortKeys.buf = rec->info[recInfo_sortKeys];
	extract_flushSortKeys (zh, *sysno, 0, &sortKeys);
#endif

        extract_flushRecordKeys (zh, *sysno, 0, delkeys,
				 recordAttr->staticrank); /* old values */  
        if (deleteFlag)
        {
            /* record going to be deleted */
            if (zebra_rec_keys_empty(delkeys))
            {
                yaz_log (YLOG_LOG, "delete %s %s " PRINTF_OFF_T,
			 zh->m_record_type, fname, recordOffset);
                yaz_log (YLOG_WARN, "cannot delete file above, storeKeys false (1)");
            }
            else
            {
                if (zh->records_processed < zh->m_file_verbose_limit)
                    yaz_log (YLOG_LOG, "delete %s %s " PRINTF_OFF_T,
			     zh->m_record_type, fname, recordOffset);
                zh->records_deleted++;
                if (matchStr)
                    dict_delete (zh->reg->matchDict, matchStr);
                rec_del (zh->reg->records, &rec);
            }
	    rec_rm (&rec);
            logRecord (zh);
            return 1;
        }
        else
        {
	    /* flush new keys for sort&search etc */
            if (zh->records_processed < zh->m_file_verbose_limit)
                yaz_log (YLOG_LOG, "update %s %s " PRINTF_OFF_T,
                      zh->m_record_type, fname, recordOffset);
	    recordAttr->staticrank = extractCtrl.staticrank;
#if NATTR
            extract_flushSortKeys (zh, *sysno, 1, zh->reg->sortKeys);
#else
            extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
#endif
            extract_flushRecordKeys (zh, *sysno, 1, zh->reg->keys,
					 recordAttr->staticrank);
            zh->records_updated++;
        }
	zebra_rec_keys_close(delkeys);
#if NATTR
	zebra_rec_keys_close(sortKeys);
#endif
    }
    /* update file type */
    xfree (rec->info[recInfo_fileType]);
    rec->info[recInfo_fileType] =
        rec_strdup (zh->m_record_type, &rec->size[recInfo_fileType]);

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

#if NATTR
    zebra_rec_keys_get_buf(zh->reg->sortKeys,
			   &rec->info[recInfo_sortKeys],
			   &rec->size[recInfo_sortKeys]);
#else
    rec->size[recInfo_sortKeys] = zh->reg->sortKeys.buf_used;
    rec->info[recInfo_sortKeys] = zh->reg->sortKeys.buf;
    zh->reg->sortKeys.buf = NULL;
    zh->reg->sortKeys.buf_max = 0;
#endif

    /* save file size of original record */
    zebraExplain_recordBytesIncrement (zh->reg->zei,
                                       - recordAttr->recordSize);
    recordAttr->recordSize = fi->file_moffset - recordOffset;
    if (!recordAttr->recordSize)
	recordAttr->recordSize = fi->file_max - recordOffset;
    zebraExplain_recordBytesIncrement (zh->reg->zei,
                                       recordAttr->recordSize);

    /* set run-number for this record */
    recordAttr->runNumber = zebraExplain_runNumberIncrement (zh->reg->zei,
                                                             0);

    /* update store data */
    xfree (rec->info[recInfo_storeData]);
    if (zh->store_data_buf)
    {
        rec->size[recInfo_storeData] = zh->store_data_size;
        rec->info[recInfo_storeData] = zh->store_data_buf;
	zh->store_data_buf = 0;
    }
    else if (zh->m_store_data)
    {
        rec->size[recInfo_storeData] = recordAttr->recordSize;
        rec->info[recInfo_storeData] = (char *)
	    xmalloc (recordAttr->recordSize);
        if (lseek (fi->fd, recordOffset, SEEK_SET) < 0)
        {
            yaz_log (YLOG_ERRNO|YLOG_FATAL, "seek to " PRINTF_OFF_T " in %s",
                  recordOffset, fname);
            exit (1);
        }
        if (read (fi->fd, rec->info[recInfo_storeData], recordAttr->recordSize)
	    < recordAttr->recordSize)
        {
            yaz_log (YLOG_ERRNO|YLOG_FATAL, "read %d bytes of %s",
                  recordAttr->recordSize, fname);
            exit (1);
        }
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
    recordAttr->recordOffset = recordOffset;
    
    /* commit this record */
    rec_put (zh->reg->records, &rec);
    logRecord (zh);
    return 1;
}

int fileExtract (ZebraHandle zh, SYSNO *sysno, const char *fname, 
		 int deleteFlag)
{
    int r, i, fd;
    char gprefix[128];
    char ext[128];
    char ext_res[128];
    struct file_read_info *fi;
    const char *original_record_type = 0;
    RecType recType;
    void *recTypeClientData;

    if (!zh->m_group || !*zh->m_group)
        *gprefix = '\0';
    else
        sprintf (gprefix, "%s.", zh->m_group);
    
    yaz_log (YLOG_DEBUG, "fileExtract %s", fname);

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
        return 0;
    }

    switch(recType->version)
    {
    case 0:
	break;
    default:
	yaz_log(YLOG_WARN, "Bad filter version: %s", zh->m_record_type);
    }
    if (sysno && deleteFlag)
        fd = -1;
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
            return 0;
        }
    }
    fi = file_read_start (fd);
    do
    {
	fi->file_moffset = fi->file_offset;
	fi->file_more = 0;  /* file_end not called (yet) */
        r = file_extract_record (zh, sysno, fname, deleteFlag, fi, 1,
				 recType, recTypeClientData);
	if (fi->file_more)
	{   /* file_end has been called so reset offset .. */
	    fi->file_offset = fi->file_moffset;
	    lseek(fi->fd, fi->file_moffset, SEEK_SET);
	}
    }
    while (r && !sysno);
    file_read_stop (fi);
    if (fd != -1)
        close (fd);
    zh->m_record_type = original_record_type;
    return r;
}

/*
  If sysno is provided, then it's used to identify the reocord.
  If not, and match_criteria is provided, then sysno is guessed
  If not, and a record is provided, then sysno is got from there
  
 */
ZEBRA_RES buffer_extract_record(ZebraHandle zh, 
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
    SYSNO sysno0 = 0;
    RecordAttr *recordAttr;
    struct recExtractCtrl extractCtrl;
    int r;
    const char *matchStr = 0;
    RecType recType = NULL;
    void *clientData;
    Record rec;
    long recordOffset = 0;
    struct zebra_fetch_control fc;
    const char *pr_fname = fname;  /* filename to print .. */
    int show_progress = zh->records_processed < zh->m_file_verbose_limit ? 1:0;

    if (!pr_fname)
	pr_fname = "<no file>";  /* make it printable if file is omitted */

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
    extractCtrl.first_record = 1;
    extractCtrl.fh = &fc;

    zebra_rec_keys_reset(zh->reg->keys);

#if NATTR
    zebra_rec_keys_reset(zh->reg->sortKeys);
#else
    zh->reg->sortKeys.buf_used = 0;
#endif
    if (zebraExplain_curDatabase (zh->reg->zei, zh->basenames[0]))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, zh->basenames[0], 
				      zh->m_explain_database))
            return ZEBRA_FAIL;
    }
    
    if (recordType && *recordType)
    {
        yaz_log (YLOG_DEBUG, "Record type explicitly specified: %s", recordType);
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
        yaz_log (YLOG_DEBUG, "Get record type from rgroup: %s",zh->m_record_type);
        recType = recType_byName (zh->reg->recTypes, zh->res,
				  zh->m_record_type, &clientData);
        recordType = zh->m_record_type;
    }
    
    if (!recType)
    {
        yaz_log (YLOG_WARN, "No such record type: %s", zh->m_record_type);
        return ZEBRA_FAIL;
    }
    
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->reg->dh;
    extractCtrl.handle = zh;
    extractCtrl.match_criteria[0] = '\0';
    extractCtrl.staticrank = 0;
    
    init_extractCtrl(zh, &extractCtrl);

    extract_set_store_data_prepare(&extractCtrl);

    r = (*recType->extract)(clientData, &extractCtrl);

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

    if (extractCtrl.match_criteria[0])
	match_criteria = extractCtrl.match_criteria;

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
        if (matchStr) {
	    char *rinfo = dict_lookup (zh->reg->matchDict, matchStr);
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
	    yaz_log (YLOG_LOG, "delete %s %s %ld", recordType,
			 pr_fname, (long) recordOffset);
            yaz_log (YLOG_WARN, "cannot delete record above (seems new)");
            return ZEBRA_FAIL;
        }
	if (show_progress)
	    yaz_log (YLOG_LOG, "add %s %s %ld", recordType, pr_fname,
		     (long) recordOffset);
        rec = rec_new (zh->reg->records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zh->reg->zei, rec);
	recordAttr->staticrank = extractCtrl.staticrank;

        if (matchStr)
        {
            dict_insert (zh->reg->matchDict, matchStr,
                         sizeof(*sysno), sysno);
        }
#if NATTR
	extract_flushSortKeys (zh, *sysno, 1, zh->reg->sortKeys);
#else
	extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
#endif
	
#if 0
	print_rec_keys(zh, zh->reg->keys);
#endif
        extract_flushRecordKeys (zh, *sysno, 1, zh->reg->keys,
			 recordAttr->staticrank);
        zh->records_inserted++;
    } 
    else
    {
        /* record already exists */
	zebra_rec_keys_t delkeys = zebra_rec_keys_open();
#if NATTR
	zebra_rec_keys_t sortKeys = zebra_rec_keys_open();
#else
        struct sortKeys sortKeys;
#endif

	if (!allow_update)
	{
	    yaz_log (YLOG_LOG, "skipped %s %s %ld", 
			 recordType, pr_fname, (long) recordOffset);
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
#if NATTR
	zebra_rec_keys_set_buf(sortKeys,
			       rec->info[recInfo_sortKeys],
			       rec->size[recInfo_sortKeys],
			       0);
#else
        sortKeys.buf_used = rec->size[recInfo_sortKeys];
        sortKeys.buf = rec->info[recInfo_sortKeys];
#endif

#if NATTR
	extract_flushSortKeys (zh, *sysno, 0, sortKeys);
#else
	extract_flushSortKeys (zh, *sysno, 0, &sortKeys);
#endif
        extract_flushRecordKeys (zh, *sysno, 0, delkeys,
				 recordAttr->staticrank);
        if (delete_flag)
        {
            /* record going to be deleted */
            if (zebra_rec_keys_empty(delkeys))
            {
		yaz_log (YLOG_LOG, "delete %s %s %ld", recordType,
		     pr_fname, (long) recordOffset);
		yaz_log (YLOG_WARN, "cannot delete file above, "
			     "storeKeys false (3)");
	    }
            else
            {
		if (show_progress)
		    yaz_log (YLOG_LOG, "delete %s %s %ld", recordType,
			     pr_fname, (long) recordOffset);
                zh->records_deleted++;
                if (matchStr)
                    dict_delete (zh->reg->matchDict, matchStr);
                rec_del (zh->reg->records, &rec);
            }
	    rec_rm (&rec);
            logRecord(zh);
            return ZEBRA_OK;
        }
        else
        {
	    if (show_progress)
		    yaz_log (YLOG_LOG, "update %s %s %ld", recordType,
			     pr_fname, (long) recordOffset);
	    recordAttr->staticrank = extractCtrl.staticrank;
#if NATTR
            extract_flushSortKeys (zh, *sysno, 1, zh->reg->sortKeys);
#else
            extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
#endif
            extract_flushRecordKeys (zh, *sysno, 1, zh->reg->keys, 
					 recordAttr->staticrank);
            zh->records_updated++;
        }
	zebra_rec_keys_close(delkeys);
#if NATTR
	zebra_rec_keys_close(sortKeys);
#endif
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

#if NATTR
    zebra_rec_keys_get_buf(zh->reg->sortKeys,
			   &rec->info[recInfo_sortKeys],
			   &rec->size[recInfo_sortKeys]);
#else
    rec->size[recInfo_sortKeys] = zh->reg->sortKeys.buf_used;
    rec->info[recInfo_sortKeys] = zh->reg->sortKeys.buf;
    zh->reg->sortKeys.buf = NULL;
    zh->reg->sortKeys.buf_max = 0;
#endif

    /* save file size of original record */
    zebraExplain_recordBytesIncrement (zh->reg->zei,
				       - recordAttr->recordSize);
#if 0
    recordAttr->recordSize = fi->file_moffset - recordOffset;
    if (!recordAttr->recordSize)
	recordAttr->recordSize = fi->file_max - recordOffset;
#else
    recordAttr->recordSize = buf_size;
#endif
    zebraExplain_recordBytesIncrement (zh->reg->zei,
				       recordAttr->recordSize);

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
        rec->size[recInfo_storeData] = recordAttr->recordSize;
        rec->info[recInfo_storeData] = (char *)
	    xmalloc (recordAttr->recordSize);
        memcpy (rec->info[recInfo_storeData], buf, recordAttr->recordSize);
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
    recordAttr->recordOffset = recordOffset;
    
    /* commit this record */
    rec_put (zh->reg->records, &rec);
    logRecord(zh);
    return ZEBRA_OK;
}

int explain_extract (void *handle, Record rec, data1_node *n)
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
    
#if NATTR
    zebra_rec_keys_reset(zh->reg->sortKeys);
#else
    zh->reg->sortKeys.buf_used = 0;
#endif
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
	
#if NATTR
	zebra_rec_keys_t sortkeys = zebra_rec_keys_open();
#else
	struct sortKeys sortkeys;
#endif

	zebra_rec_keys_set_buf(delkeys, rec->info[recInfo_delKeys],
			       rec->size[recInfo_delKeys],
			       0);
	extract_flushRecordKeys (zh, rec->sysno, 0, delkeys, 0);
	zebra_rec_keys_close(delkeys);
#if NATTR
	zebra_rec_keys_set_buf(sortkeys, rec->info[recInfo_sortKeys],
			       rec->size[recInfo_sortKeys],
			       0);

	extract_flushSortKeys (zh, rec->sysno, 0, sortkeys);
	zebra_rec_keys_close(sortkeys);
#else
	sortkeys.buf_used = rec->size[recInfo_sortKeys];
	sortkeys.buf = rec->info[recInfo_sortKeys];
	extract_flushSortKeys (zh, rec->sysno, 0, &sortkeys);
#endif
    }
    extract_flushRecordKeys (zh, rec->sysno, 1, zh->reg->keys, 0);
#if NATTR
    extract_flushSortKeys (zh, rec->sysno, 1, zh->reg->sortKeys);
#else
    extract_flushSortKeys (zh, rec->sysno, 1, &zh->reg->sortKeys);
#endif

    xfree (rec->info[recInfo_delKeys]);
    zebra_rec_keys_get_buf(zh->reg->keys,
			   &rec->info[recInfo_delKeys],	
			   &rec->size[recInfo_delKeys]);

    xfree (rec->info[recInfo_sortKeys]);
#if NATTR
    zebra_rec_keys_get_buf(zh->reg->sortKeys,
			   &rec->info[recInfo_sortKeys],
			   &rec->size[recInfo_sortKeys]);
#else
    rec->size[recInfo_sortKeys] = zh->reg->sortKeys.buf_used;
    rec->info[recInfo_sortKeys] = zh->reg->sortKeys.buf;
    zh->reg->sortKeys.buf = NULL;
    zh->reg->sortKeys.buf_max = 0;
#endif

    return 0;
}

void extract_flushRecordKeys (ZebraHandle zh, SYSNO sysno,
                              int cmd,
			      zebra_rec_keys_t reckeys,
			      zint staticrank)
{
    ZebraExplainInfo zei = zh->reg->zei;

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
	    struct it_key key_out;
	    zint *keyp = key_out.mem;

	    assert(key_in.len == 4);
	    
	    /* check for buffer overflow */
	    if (zh->reg->key_buf_used + 1024 > 
		(zh->reg->ptr_top -zh->reg->ptr_i)*sizeof(char*))
		extract_flushWriteKeys (zh, 0);
	    
	    ++(zh->reg->ptr_i);
	    assert(zh->reg->ptr_i > 0);
	    (zh->reg->key_buf)[zh->reg->ptr_top - zh->reg->ptr_i] =
		(char*)zh->reg->key_buf + zh->reg->key_buf_used;

	    /* encode the ordinal value (field/use/attribute) .. */
	    ch = (int) key_in.mem[0];
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
		*keyp++ = staticrank;
		key_out.len = 4;
	    }
	    else
		key_out.len = 3;
	    
	    if (key_in.mem[1]) /* filter specified record ID */
		*keyp++ = key_in.mem[1];
	    else
		*keyp++ = sysno;
	    *keyp++ = key_in.mem[2];  /* section_id */
	    *keyp++ = key_in.mem[3];  /* sequence .. */
	    
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
        yaz_log (YLOG_DEBUG, "  nothing to flush section=%d buf=%p i=%d",
               zh->reg->key_file_no, zh->reg->key_buf, ptr_i);
        yaz_log (YLOG_DEBUG, "  buf=%p ",
               zh->reg->key_buf);
        yaz_log (YLOG_DEBUG, "  ptr=%d ",zh->reg->ptr_i);
        yaz_log (YLOG_DEBUG, "  reg=%p ",zh->reg);
               
        return;
    }

    (zh->reg->key_file_no)++;
    yaz_log (YLOG_LOG, "sorting section %d", (zh->reg->key_file_no));
    yaz_log (YLOG_DEBUG, "  sort_buff at %p n=%d",
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
	    int ord, seqno;
	    int index_type;
	    assert(key.len <= 4 && key.len > 2);
	    seqno = (int) key.mem[key.len-1];
	    ord = key.mem[0];
	    
	    zebraExplain_lookup_ord(zh->reg->zei, ord, &index_type,
				    0/* db */, 0/* set */, 0/* use */);
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
	    int seqno;
	    int index_type;
	    const char *db = 0;
	    assert(key.len <= 4 && key.len > 2);

	    zebraExplain_lookup_ord(zh->reg->zei,
				    key.mem[0], &index_type, &db, 0, 0);
	    
	    seqno = (int) key.mem[key.len-1];
	    
	    zebra_term_untrans(zh, index_type, dst_buf, str);
	    
	    yaz_log(YLOG_LOG, "ord=" ZINT_FORMAT " seqno=%d term=%s",
		    key.mem[0], seqno, dst_buf); 
	}
    }
}

void extract_add_index_string (RecWord *p, const char *str, int length)
{
    struct it_key key;

    ZebraHandle zh = p->extractCtrl->handle;
    ZebraExplainInfo zei = zh->reg->zei;
    int ch;

    if (p->index_name)
    {
	ch = zebraExplain_lookup_attr_str(zei, p->index_type, p->index_name);
	if (ch < 0)
	    ch = zebraExplain_add_attr_str(zei, p->index_type, p->index_name);
    }
    else
    {
#if NATTR
	return;
#else
	ch = zebraExplain_lookup_attr_su(zei, p->index_type, 
					 p->attrSet, p->attrUse);
	if (ch < 0)
	    ch = zebraExplain_add_attr_su(zei, p->index_type,
					  p->attrSet, p->attrUse);
#endif
    }
    key.len = 4;
    key.mem[0] = ch;
    key.mem[1] = p->record_id;
    key.mem[2] = p->section_id;
    key.mem[3] = p->seqno;

#if 0
    if (1)
    {
	char strz[80];
	int i;

	strz[0] = 0;
	for (i = 0; i<length && i < 20; i++)
	    sprintf(strz+strlen(strz), "%02X", str[i] & 0xff);
	/* just for debugging .. */
	yaz_log(YLOG_LOG, "add: set=%d use=%d "
		"record_id=%lld section_id=%lld seqno=%lld %s",
		p->attrSet, p->attrUse, p->record_id, p->section_id, p->seqno,
		strz);
    }
#endif

    zebra_rec_keys_write(zh->reg->keys, str, length, &key);
}

#if NATTR
static void extract_add_sort_string (RecWord *p, const char *str, int length)
{
    struct it_key key;

    ZebraHandle zh = p->extractCtrl->handle;
    ZebraExplainInfo zei = zh->reg->zei;
    int ch;

    if (p->index_name)
    {
	ch = zebraExplain_lookup_attr_str(zei, p->index_type, p->index_name);
	if (ch < 0)
	    ch = zebraExplain_add_attr_str(zei, p->index_type, p->index_name);
    }
    else
    {
	return;
    }
    key.len = 4;
    key.mem[0] = ch;
    key.mem[1] = p->record_id;
    key.mem[2] = p->section_id;
    key.mem[3] = p->seqno;

    zebra_rec_keys_write(zh->reg->sortKeys, str, length, &key);
}
#else
static void extract_add_sort_string (RecWord *p, const char *str, int length)
{
    ZebraHandle zh = p->extractCtrl->handle;
    struct sortKeys *sk = &zh->reg->sortKeys;
    int off = 0;

    while (off < sk->buf_used)
    {
        int set, use, slen;

        off += key_SU_decode(&set, (unsigned char *) sk->buf + off);
        off += key_SU_decode(&use, (unsigned char *) sk->buf + off);
        off += key_SU_decode(&slen, (unsigned char *) sk->buf + off);
        off += slen;
        if (p->attrSet == set && p->attrUse == use)
            return;
    }
    assert (off == sk->buf_used);
    
    if (sk->buf_used + IT_MAX_WORD > sk->buf_max)
    {
        char *b;
        
        b = (char *) xmalloc (sk->buf_max += 128000);
        if (sk->buf_used > 0)
            memcpy (b, sk->buf, sk->buf_used);
        xfree (sk->buf);
        sk->buf = b;
    }
    off += key_SU_encode(p->attrSet, sk->buf + off);
    off += key_SU_encode(p->attrUse, sk->buf + off);
    off += key_SU_encode(length, sk->buf + off);
    memcpy (sk->buf + off, str, length);
    sk->buf_used = off + length;
}
#endif

void extract_add_string (RecWord *p, const char *string, int length)
{
    assert (length > 0);
    if (zebra_maps_is_sort (p->zebra_maps, p->index_type))
	extract_add_sort_string (p, string, length);
    else
	extract_add_index_string (p, string, length);
}

static void extract_add_incomplete_field (RecWord *p)
{
    const char *b = p->term_buf;
    int remain = p->term_len;
    const char **map = 0;
    
    yaz_log(YLOG_DEBUG, "Incomplete field, w='%.*s'", p->term_len, p->term_buf);

    if (remain > 0)
	map = zebra_maps_input(p->zebra_maps, p->index_type, &b, remain, 0);

    while (map)
    {
	char buf[IT_MAX_WORD+1];
	int i, remain;

	/* Skip spaces */
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->term_len - (b - p->term_buf);
	    if (remain > 0)
		map = zebra_maps_input(p->zebra_maps, p->index_type, &b,
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
		map = zebra_maps_input(p->zebra_maps, p->index_type, &b, remain, 0);
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
    const char *b = p->term_buf;
    char buf[IT_MAX_WORD+1];
    const char **map = 0;
    int i = 0, remain = p->term_len;

    yaz_log(YLOG_DEBUG, "Complete field, w='%.*s'",
	    p->term_len, p->term_buf);

    if (remain > 0)
	map = zebra_maps_input (p->zebra_maps, p->index_type, &b, remain, 1);

    while (remain > 0 && i < IT_MAX_WORD)
    {
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->term_len - (b - p->term_buf);

	    if (remain > 0)
	    {
		int first = i ? 0 : 1;  /* first position */
		map = zebra_maps_input(p->zebra_maps, p->index_type, &b, remain, first);
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
		yaz_log(YLOG_DEBUG, "Adding string to index '%d'", **map);
		while (i < IT_MAX_WORD && *cp)
		    buf[i++] = *(cp++);
	    }
	    remain = p->term_len  - (b - p->term_buf);
	    if (remain > 0)
	    {
		map = zebra_maps_input (p->zebra_maps, p->index_type, &b,
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

void extract_token_add (RecWord *p)
{
    WRBUF wrbuf;
#if 0
    yaz_log (YLOG_LOG, "token_add "
	     "reg_type=%c attrSet=%d attrUse=%d seqno=%d s=%.*s",
             p->reg_type, p->attrSet, p->attrUse, p->seqno, p->length,
             p->string);
#endif
    if ((wrbuf = zebra_replace(p->zebra_maps, p->index_type, 0,
			       p->term_buf, p->term_len)))
    {
	p->term_buf = wrbuf_buf(wrbuf);
	p->term_len = wrbuf_len(wrbuf);
    }
    if (zebra_maps_is_complete (p->zebra_maps, p->index_type))
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

void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid)
{
    ZebraHandle zh = (ZebraHandle) p->handle;
    zebraExplain_addSchema (zh->reg->zei, oid);
}

#if NATTR
#error not done yet with zebra_rec_keys_t
void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
                            int cmd, struct recKeys *reckeys)
{
    SortIdx sortIdx = zh->reg->sortIdx;
    void *decode_handle = iscz1_start();
    int off = 0;
    int ch = 0;

    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        struct it_key key;
	char *dst = (char*) &key;

	iscz1_decode(decode_handle, &dst, &src);
	assert(key.len == 4);

	ch = (int) key.mem[0];  /* ordinal for field/use/attribute */

        sortIdx_type(sortIdx, ch);
        if (cmd == 1)
            sortIdx_add(sortIdx, src, strlen(src));
        else
            sortIdx_add(sortIdx, "", 1);
	
	src += strlen(src);
        src++;

        off = src - reckeys->buf;
    }
    assert (off == reckeys->buf_used);
    iscz1_stop(decode_handle);
}
#else
void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
                            int cmd, struct sortKeys *sk)
{
    SortIdx sortIdx = zh->reg->sortIdx;
    int off = 0;

    sortIdx_sysno (sortIdx, sysno);

    while (off < sk->buf_used)
    {
        int set, use, slen;
        
        off += key_SU_decode(&set, (unsigned char *) sk->buf + off);
        off += key_SU_decode(&use, (unsigned char *) sk->buf + off);
        off += key_SU_decode(&slen, (unsigned char *) sk->buf + off);
        
        sortIdx_type(sortIdx, use);
        if (cmd == 1)
            sortIdx_add(sortIdx, sk->buf + off, slen);
        else
            sortIdx_add(sortIdx, "", 1);
        off += slen;
    }
}
#endif

void encode_key_init (struct encode_info *i)
{
    i->sysno = 0;
    i->seqno = 0;
    i->cmd = -1;
    i->prevsys=0;
    i->prevseq=0;
    i->prevcmd=-1;
    i->keylen=0;
    i->encode_handle = iscz1_start();
}

#define OLDENCODE 1

#ifdef OLDENCODE
/* this is the old encode_key_write 
 * may be deleted once we are confident that the new works
 * HL 15-oct-2002
 */
void encode_key_write (char *k, struct encode_info *i, FILE *outf)
{
    struct it_key key;
    char *bp = i->buf, *bp0;
    const char *src = (char *) &key;

    /* copy term to output buf */
    while ((*bp++ = *k++))
        ;
    /* and copy & align key so we can mangle */
    memcpy (&key, k+1, sizeof(struct it_key));  /* *k is insert/delete */

    bp0 = bp++;
    iscz1_encode(i->encode_handle, &bp, &src);
    *bp0 = (*k * 128) + bp - bp0 - 1; /* length and insert/delete combined */
    if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fwrite");
        exit (1);
    }
}

void encode_key_flush (struct encode_info *i, FILE *outf)
{ /* dummy routine */
    iscz1_stop(i->encode_handle);
}

#else

/* new encode_key_write
 * The idea is to buffer one more key, and compare them
 * If we are going to delete and insert the same key, 
 * we may as well not bother. Should make a difference in 
 * updates with small modifications (appending to a mbox)
 */
void encode_key_write (char *k, struct encode_info *i, FILE *outf)
{
    struct it_key key;
    char *bp; 

    if (*k)  /* first time for new key */
    {
        bp = i->buf;
        while ((*bp++ = *k++))
            ;
	i->keylen= bp - i->buf -1;    
	assert(i->keylen+1+sizeof(struct it_key) < ENCODE_BUFLEN);
    }
    else
    {
	bp=i->buf + i->keylen;
	*bp++=0;
	k++;
    }

    memcpy (&key, k+1, sizeof(struct it_key));
    if (0==i->prevsys) /* no previous filter, fill up */
    {
        i->prevsys=key.sysno;
	i->prevseq=key.seqno;
	i->prevcmd=*k;
    }
    else if ( (i->prevsys==key.sysno) &&
              (i->prevseq==key.seqno) &&
	      (i->prevcmd!=*k) )
    { /* same numbers, diff cmd, they cancel out */
        i->prevsys=0;
    }
    else 
    { /* different stuff, write previous, move buf */
        bp = encode_key_int ( (i->prevsys - i->sysno) * 2 + i->prevcmd, bp);
	if (i->sysno != i->prevsys)
	{
	    i->sysno = i->prevsys;
	    i->seqno = 0;
        }
        else if (!i->seqno && !i->prevseq && i->cmd == i->prevcmd)
	{
	    return; /* ??? Filters some sort of duplicates away */
	            /* ??? Can this ever happen   -H 15oct02 */
	}
        bp = encode_key_int (i->prevseq - i->seqno, bp);
        i->seqno = i->prevseq;
        i->cmd = i->prevcmd;
        if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
        {
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "fwrite");
            exit (1);
        }
        i->keylen=0; /* ok, it's written, forget it */
	i->prevsys=key.sysno;
	i->prevseq=key.seqno;
	i->prevcmd=*k;
    }
}

void encode_key_flush (struct encode_info *i, FILE *outf)
{ /* flush the last key from i */
    char *bp =i->buf + i->keylen;
    if (0==i->prevsys)
    {
        return; /* nothing to flush */
    }
    *bp++=0;
    bp = encode_key_int ( (i->prevsys - i->sysno) * 2 + i->prevcmd, bp);
    if (i->sysno != i->prevsys)
    {
        i->sysno = i->prevsys;
        i->seqno = 0;
    }
    else if (!i->seqno && !i->prevseq && i->cmd == i->prevcmd)
    {
        return; /* ??? Filters some sort of duplicates away */
                /* ??? Can this ever happen   -H 15oct02 */
    }
    bp = encode_key_int (i->prevseq - i->seqno, bp);
    i->seqno = i->prevseq;
    i->cmd = i->prevcmd;
    if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fwrite");
        exit (1);
    }
    i->keylen=0; /* ok, it's written, forget it */
    i->prevsys=0; /* forget the values too */
    i->prevseq=0;
}
#endif
