/* $Id: extract.c,v 1.169 2004-12-02 14:05:03 adam Exp $
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
#include <ctype.h>
#ifdef WIN32
#include <io.h>
#else
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

static void extract_init (struct recExtractCtrl *p, RecWord *w)
{
    w->zebra_maps = p->zebra_maps;
    w->seqno = 1;
    w->attrSet = VAL_BIB1;
    w->attrUse = 1016;
    w->reg_type = 'w';
    w->extractCtrl = p;
    w->record_id = 0;
    w->section_id = 0;
}

static const char **searchRecordKey (ZebraHandle zh,
                                     struct recKeys *reckeys,
				     int attrSetS, int attrUseS)
{
    static const char *ws[32];
    void *decode_handle = iscz1_start();
    int off = 0;
    int startSeq = -1;
    int seqno = 0;
    int i;

    for (i = 0; i<32; i++)
        ws[i] = NULL;

    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        struct it_key key;
	char *dst = (char*) &key;
	int attrSet, attrUse;

	iscz1_decode(decode_handle, &dst, &src);
	assert(key.len < 4 && key.len > 2);

	attrSet = (int) key.mem[0] >> 16;
	attrUse = (int) key.mem[0] & 65535;
	seqno = (int) key.mem[key.len-1];

	if (attrUseS == attrUse && attrSetS == attrSet)
        {
            int woff;

            if (startSeq == -1)
                startSeq = seqno;
            woff = seqno - startSeq;
            if (woff >= 0 && woff < 31)
                ws[woff] = src;
        }

        while (*src++)
	    ;
        off = src - reckeys->buf;
    }
    iscz1_stop(decode_handle);
    assert (off == reckeys->buf_used);
    return ws;
}

struct file_read_info {
    off_t file_max;	    /* maximum offset so far */
    off_t file_offset;      /* current offset */
    off_t file_moffset;     /* offset of rec/rec boundary */
    int file_more;
    int fd;
    char *sdrbuf;
    int sdrmax;
};

static struct file_read_info *file_read_start (int fd)
{
    struct file_read_info *fi = (struct file_read_info *)
	xmalloc (sizeof(*fi));

    fi->fd = fd;
    fi->file_max = 0;
    fi->file_moffset = 0;
    fi->sdrbuf = 0;
    fi->sdrmax = 0;
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
    if (p->sdrbuf)
	return offset;
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
    if (p->sdrbuf)
    {
	r = count;
	if (r > p->sdrmax - p->file_offset)
	    r = p->sdrmax - p->file_offset;
	if (r)
	    memcpy (buf, p->sdrbuf + p->file_offset, r);
    }
    else
	r = read (fd, buf, count);
    if (r > 0)
    {
        p->file_offset += r;
        if (p->file_offset > p->file_max)
            p->file_max = p->file_offset;
    }
    return r;
}

static void file_begin (void *handle)
{
    struct file_read_info *p = (struct file_read_info *) handle;

    p->file_offset = p->file_moffset;
    if (!p->sdrbuf && p->file_moffset)
        lseek (p->fd, p->file_moffset, SEEK_SET);
    p->file_more = 0;
}

static void file_end (void *handle, off_t offset)
{
    struct file_read_info *p = (struct file_read_info *) handle;

    assert (p->file_more == 0);
    p->file_more = 1;
    p->file_moffset = offset;
}

static char *fileMatchStr (ZebraHandle zh,
                           struct recKeys *reckeys,
                           const char *fname, const char *spec)
{
    static char dstBuf[2048];      /* static here ??? */
    char *dst = dstBuf;
    const char *s = spec;
    static const char **w;

    while (1)
    {
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        if (*s == '(')
        {
	    char attset_str[64], attname_str[64];
	    data1_attset *attset;
	    int i;
            char matchFlag[32];
            int attSet = 1, attUse = 1;
            int first = 1;

            s++;
	    for (i = 0; *s && *s != ',' && *s != ')'; s++)
		if (i < 63)
		    attset_str[i++] = *s;
	    attset_str[i] = '\0';

	    if (*s == ',')
	    {
		s++;
		for (i = 0; *s && *s != ')'; s++)
		    if (i < 63)
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
            w = searchRecordKey (zh, reckeys, attSet, attUse);
            assert (w);

            if (*s == ')')
            {
                for (i = 0; i<32; i++)
                    matchFlag[i] = 1;
            }
            else
            {
                yaz_log (YLOG_WARN, "Missing ) in match criteria %s in group %s",
                      spec, zh->m_group ? zh->m_group : "none");
                return NULL;
            }
            s++;

            for (i = 0; i<32; i++)
                if (matchFlag[i] && w[i])
                {
                    if (first)
                    {
                        *dst++ = ' ';
                        first = 0;
                    }
                    strcpy (dst, w[i]);
                    dst += strlen(w[i]);
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
            while (*s1 && *s1 != ' ' && *s1 != '\t')
                s1++;

            spec_len = s1 - s;
            if (spec_len > 63)
                spec_len = 63;
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
                if (i < 63)
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

void create_rec_keys_codec(struct recKeys *keys)
{
    keys->buf_used = 0;
    iscz1_reset(keys->codec_handle);
}
     
static int file_extract_record(ZebraHandle zh,
			       SYSNO *sysno, const char *fname,
			       int deleteFlag,
			       struct file_read_info *fi,
			       int force_update)
{
    RecordAttr *recordAttr;
    int r;
    const char *matchStr = 0;
    SYSNO sysnotmp;
    Record rec;
    off_t recordOffset = 0;
    RecType recType;
    void *clientData;
    
    if (!(recType =
	  recType_byName (zh->reg->recTypes, zh->res, zh->m_record_type,
			  &clientData)))
    {
        yaz_log (YLOG_WARN, "No such record type: %s", zh->m_record_type);
        return 0;
    }

    /* announce database */
    if (zebraExplain_curDatabase (zh->reg->zei, zh->basenames[0]))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, zh->basenames[0],
				      zh->m_explain_database))
	    return 0;
    }

    if (fi->fd != -1)
    {
	struct recExtractCtrl extractCtrl;

        /* we are going to read from a file, so prepare the extraction */
	int i;

	create_rec_keys_codec(&zh->reg->keys);

	zh->reg->sortKeys.buf_used = 0;
	
	recordOffset = fi->file_moffset;
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
        extractCtrl.handle = zh;
	for (i = 0; i<256; i++)
	{
	    if (zebra_maps_is_positioned(zh->reg->zebra_maps, i))
		extractCtrl.seqno[i] = 1;
	    else
		extractCtrl.seqno[i] = 0;
	}
	extractCtrl.zebra_maps = zh->reg->zebra_maps;
	extractCtrl.flagShowRecords = !zh->m_flag_rw;

        if (!zh->m_flag_rw)
            printf ("File: %s " PRINTF_OFF_T "\n", fname, recordOffset);
        if (zh->m_flag_rw)
        {
            char msg[512];
            sprintf (msg, "%s:" PRINTF_OFF_T , fname, recordOffset);
            yaz_log_init_prefix2 (msg);
        }

        r = (*recType->extract)(clientData, &extractCtrl);

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
        if (zh->reg->keys.buf_used == 0)
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
        
            matchStr = fileMatchStr (zh, &zh->reg->keys, fname, 
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

        if (matchStr)
        {
            dict_insert (zh->reg->matchDict, matchStr, sizeof(*sysno), sysno);
        }
	extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
        extract_flushRecordKeys (zh, *sysno, 1, &zh->reg->keys);

        zh->records_inserted++;
    }
    else
    {
        /* record already exists */
        struct recKeys delkeys;
        struct sortKeys sortKeys;

        rec = rec_get (zh->reg->records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zh->reg->zei, rec);

	if (!force_update && recordAttr->runNumber ==
            zebraExplain_runNumberIncrement (zh->reg->zei, 0))
	{
            yaz_log (YLOG_LOG, "run number = %d", recordAttr->runNumber);
	    yaz_log (YLOG_LOG, "skipped %s %s " PRINTF_OFF_T,
                     zh->m_record_type, fname, recordOffset);
	    extract_flushSortKeys (zh, *sysno, -1, &zh->reg->sortKeys);
	    rec_rm (&rec);
	    logRecord (zh);
	    return 1;
	}
        delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];

        sortKeys.buf_used = rec->size[recInfo_sortKeys];
        sortKeys.buf = rec->info[recInfo_sortKeys];

	extract_flushSortKeys (zh, *sysno, 0, &sortKeys);
        extract_flushRecordKeys (zh, *sysno, 0, &delkeys);
        if (deleteFlag)
        {
            /* record going to be deleted */
            if (!delkeys.buf_used)
            {
                yaz_log (YLOG_LOG, "delete %s %s " PRINTF_OFF_T,
                      zh->m_record_type, fname, recordOffset);
                yaz_log (YLOG_WARN, "cannot delete file above, storeKeys false");
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
            /* record going to be updated */
            if (!delkeys.buf_used)
            {
                yaz_log (YLOG_LOG, "update %s %s " PRINTF_OFF_T,
                      zh->m_record_type, fname, recordOffset);
                yaz_log (YLOG_WARN, "cannot update file above, storeKeys false");
            }
            else
            {
                if (zh->records_processed < zh->m_file_verbose_limit)
                    yaz_log (YLOG_LOG, "update %s %s " PRINTF_OFF_T,
                        zh->m_record_type, fname, recordOffset);
                extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
                extract_flushRecordKeys (zh, *sysno, 1, &zh->reg->keys);
                zh->records_updated++;
            }
        }
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
    if (zh->reg->keys.buf_used > 0 && zh->m_store_keys == 1)
    {
        rec->size[recInfo_delKeys] = zh->reg->keys.buf_used;
        rec->info[recInfo_delKeys] = zh->reg->keys.buf;
        zh->reg->keys.buf = NULL;
        zh->reg->keys.buf_max = 0;
    }
    else
    {
        rec->info[recInfo_delKeys] = NULL;
        rec->size[recInfo_delKeys] = 0;
    }

    /* update sort keys */
    xfree (rec->info[recInfo_sortKeys]);

    rec->size[recInfo_sortKeys] = zh->reg->sortKeys.buf_used;
    rec->info[recInfo_sortKeys] = zh->reg->sortKeys.buf;
    zh->reg->sortKeys.buf = NULL;
    zh->reg->sortKeys.buf_max = 0;

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
    if (zh->m_store_data)
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
        file_begin (fi);
        r = file_extract_record (zh, sysno, fname, deleteFlag, fi, 1);
    } while (r && !sysno && fi->file_more);
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
int buffer_extract_record (ZebraHandle zh, 
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
    RecordAttr *recordAttr;
    struct recExtractCtrl extractCtrl;
    int i, r;
    const char *matchStr = 0;
    RecType recType = NULL;
    void *clientData;
    Record rec;
    long recordOffset = 0;
    struct zebra_fetch_control fc;
    const char *pr_fname = fname;  /* filename to print .. */

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
    extractCtrl.fh = &fc;

    create_rec_keys_codec(&zh->reg->keys);

    zh->reg->sortKeys.buf_used = 0;

    if (zebraExplain_curDatabase (zh->reg->zei, zh->basenames[0]))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, zh->basenames[0], 
				      zh->m_explain_database))
            return 0;
    }
    
    if (recordType && *recordType) {
        yaz_log (YLOG_DEBUG, "Record type explicitly specified: %s", recordType);
        recType = recType_byName (zh->reg->recTypes, zh->res, recordType,
                                  &clientData);
    } else {
        if (!(zh->m_record_type)) {
            yaz_log (YLOG_WARN, "No such record type defined");
            return 0;
        }
        yaz_log (YLOG_DEBUG, "Get record type from rgroup: %s",zh->m_record_type);
        recType = recType_byName (zh->reg->recTypes, zh->res,
				  zh->m_record_type, &clientData);
        recordType = zh->m_record_type;
    }
    
    if (!recType) {
        yaz_log (YLOG_WARN, "No such record type: %s", zh->m_record_type);
        return 0;
    }
    
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->reg->dh;
    extractCtrl.handle = zh;
    extractCtrl.zebra_maps = zh->reg->zebra_maps;
    extractCtrl.flagShowRecords = 0;
    extractCtrl.match_criteria[0] = '\0';
    for (i = 0; i<256; i++)
    {
	if (zebra_maps_is_positioned(zh->reg->zebra_maps, i))
	    extractCtrl.seqno[i] = 1;
	else
	    extractCtrl.seqno[i] = 0;
    }

    r = (*recType->extract)(clientData, &extractCtrl);

    if (r == RECCTRL_EXTRACT_EOF)
	return 0;
    else if (r == RECCTRL_EXTRACT_ERROR_GENERIC)
    {
	/* error occured during extraction ... */
	yaz_log (YLOG_WARN, "extract error: generic");
	return 0;
    }
    else if (r == RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER)
    {
	/* error occured during extraction ... */
	yaz_log (YLOG_WARN, "extract error: no such filter");
	return 0;
    }
    if (zh->reg->keys.buf_used == 0)
    {
	/* the extraction process returned no information - the record
	   is probably empty - unless flagShowRecords is in use */
	if (test_mode)
	    return 1;
	yaz_log (YLOG_WARN, "No keys generated for record");
	yaz_log (YLOG_WARN, " The file is probably empty");
	return 1;
    }
    /* match criteria */
    matchStr = NULL;

    if (extractCtrl.match_criteria[0])
	match_criteria = extractCtrl.match_criteria;

    if (! *sysno) {
        char *rinfo;
        if (match_criteria && *match_criteria) {
            matchStr = match_criteria;
        } else {
            if (zh->m_record_id && *zh->m_record_id) {
                matchStr = fileMatchStr (zh, &zh->reg->keys, pr_fname, 
                                         zh->m_record_id);
		if (!matchStr)
                {
                    yaz_log (YLOG_WARN, "Bad match criteria (recordID)");
		    return 1;
                }
            }
        }
        if (matchStr) {
            rinfo = dict_lookup (zh->reg->matchDict, matchStr);
            if (rinfo)
	    {
		assert(*rinfo == sizeof(*sysno));
                memcpy (sysno, rinfo+1, sizeof(*sysno));
	    }
        }
    }

    if (! *sysno)
    {
        /* new record */
        if (delete_flag)
        {
	    yaz_log (YLOG_LOG, "delete %s %s %ld", recordType,
		  pr_fname, (long) recordOffset);
            yaz_log (YLOG_WARN, "cannot delete record above (seems new)");
            return 1;
        }
	yaz_log (YLOG_LOG, "add %s %s %ld", recordType, pr_fname,
	      (long) recordOffset);
        rec = rec_new (zh->reg->records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zh->reg->zei, rec);

        if (matchStr)
        {
            dict_insert (zh->reg->matchDict, matchStr,
                         sizeof(*sysno), sysno);
        }
	extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
        extract_flushRecordKeys (zh, *sysno, 1, &zh->reg->keys);

        zh->records_inserted++;
    } 
    else
    {
        /* record already exists */
        struct recKeys delkeys;
        struct sortKeys sortKeys;

	if (!allow_update) {
	      yaz_log (YLOG_LOG, "skipped %s %s %ld", 
		    recordType, pr_fname, (long) recordOffset);
	      logRecord(zh);
	      return -1;
	}

        rec = rec_get (zh->reg->records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zh->reg->zei, rec);
	
	if (!force_update) {
	    if (recordAttr->runNumber ==
		zebraExplain_runNumberIncrement (zh->reg->zei, 0))
	    {
		yaz_log (YLOG_LOG, "skipped %s %s %ld", recordType,
		      pr_fname, (long) recordOffset);
		extract_flushSortKeys (zh, *sysno, -1, &zh->reg->sortKeys);
		rec_rm (&rec);
		logRecord(zh);
		return -1;
	    }
	}

        delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];

        sortKeys.buf_used = rec->size[recInfo_sortKeys];
        sortKeys.buf = rec->info[recInfo_sortKeys];

	extract_flushSortKeys (zh, *sysno, 0, &sortKeys);
        extract_flushRecordKeys (zh, *sysno, 0, &delkeys);
        if (delete_flag)
        {
            /* record going to be deleted */
            if (!delkeys.buf_used)
            {
                yaz_log (YLOG_LOG, "delete %s %s %ld", recordType,
                      pr_fname, (long) recordOffset);
                yaz_log (YLOG_WARN, "cannot delete file above, storeKeys false");
            }
            else
            {
		yaz_log (YLOG_LOG, "delete %s %s %ld", recordType,
		      pr_fname, (long) recordOffset);
                zh->records_deleted++;
                if (matchStr)
                    dict_delete (zh->reg->matchDict, matchStr);
                rec_del (zh->reg->records, &rec);
            }
	    rec_rm (&rec);
            logRecord(zh);
            return 0;
        }
        else
        {
            /* record going to be updated */
            if (!delkeys.buf_used)
            {
                yaz_log (YLOG_LOG, "update %s %s %ld", recordType,
                      pr_fname, (long) recordOffset);
                yaz_log (YLOG_WARN, "cannot update file above, storeKeys false");
            }
            else
            {
		yaz_log (YLOG_LOG, "update %s %s %ld", recordType,
		      pr_fname, (long) recordOffset);
                extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
                extract_flushRecordKeys (zh, *sysno, 1, &zh->reg->keys);
                zh->records_updated++;
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
    if (zh->reg->keys.buf_used > 0 && zh->m_store_keys == 1)
    {
        rec->size[recInfo_delKeys] = zh->reg->keys.buf_used;
        rec->info[recInfo_delKeys] = zh->reg->keys.buf;
        zh->reg->keys.buf = NULL;
        zh->reg->keys.buf_max = 0;
    }
    else
    {
        rec->info[recInfo_delKeys] = NULL;
        rec->size[recInfo_delKeys] = 0;
    }

    /* update sort keys */
    xfree (rec->info[recInfo_sortKeys]);

    rec->size[recInfo_sortKeys] = zh->reg->sortKeys.buf_used;
    rec->info[recInfo_sortKeys] = zh->reg->sortKeys.buf;
    zh->reg->sortKeys.buf = NULL;
    zh->reg->sortKeys.buf_max = 0;

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
    if (zh->m_store_data)
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
    return 0;
}

int explain_extract (void *handle, Record rec, data1_node *n)
{
    ZebraHandle zh = (ZebraHandle) handle;
    struct recExtractCtrl extractCtrl;
    int i;

    if (zebraExplain_curDatabase (zh->reg->zei,
				  rec->info[recInfo_databaseName]))
    {
	abort();
        if (zebraExplain_newDatabase (zh->reg->zei,
				      rec->info[recInfo_databaseName], 0))
            abort ();
    }

    create_rec_keys_codec(&zh->reg->keys);

    zh->reg->sortKeys.buf_used = 0;
    
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->reg->dh;
    for (i = 0; i<256; i++)
	extractCtrl.seqno[i] = 0;
    extractCtrl.zebra_maps = zh->reg->zebra_maps;
    extractCtrl.flagShowRecords = 0;
    extractCtrl.match_criteria[0] = '\0';
    extractCtrl.handle = handle;

    if (n)
	grs_extract_tree(&extractCtrl, n);

    if (rec->size[recInfo_delKeys])
    {
	struct recKeys delkeys;
	struct sortKeys sortkeys;

	delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];

	sortkeys.buf_used = rec->size[recInfo_sortKeys];
	sortkeys.buf = rec->info[recInfo_sortKeys];

	extract_flushSortKeys (zh, rec->sysno, 0, &sortkeys);
	extract_flushRecordKeys (zh, rec->sysno, 0, &delkeys);
    }
    extract_flushRecordKeys (zh, rec->sysno, 1, &zh->reg->keys);
    extract_flushSortKeys (zh, rec->sysno, 1, &zh->reg->sortKeys);

    xfree (rec->info[recInfo_delKeys]);
    rec->size[recInfo_delKeys] = zh->reg->keys.buf_used;
    rec->info[recInfo_delKeys] = zh->reg->keys.buf;
    zh->reg->keys.buf = NULL;
    zh->reg->keys.buf_max = 0;

    xfree (rec->info[recInfo_sortKeys]);
    rec->size[recInfo_sortKeys] = zh->reg->sortKeys.buf_used;
    rec->info[recInfo_sortKeys] = zh->reg->sortKeys.buf;
    zh->reg->sortKeys.buf = NULL;
    zh->reg->sortKeys.buf_max = 0;

    return 0;
}

void extract_flushRecordKeys (ZebraHandle zh, SYSNO sysno,
                              int cmd, struct recKeys *reckeys)
{
    void *decode_handle = iscz1_start();
    int off = 0;
    int ch = 0;
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

    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        struct it_key key;
	char *dst = (char*) &key;
	int attrSet, attrUse;

	iscz1_decode(decode_handle, &dst, &src);
	assert(key.len == 4);

	attrSet = (int) key.mem[0] >> 16;
	attrUse = (int) key.mem[0] & 65535;

        if (zh->reg->key_buf_used + 1024 > 
            (zh->reg->ptr_top -zh->reg->ptr_i)*sizeof(char*))
            extract_flushWriteKeys (zh,0);
        ++(zh->reg->ptr_i);
        assert(zh->reg->ptr_i > 0);
        (zh->reg->key_buf)[zh->reg->ptr_top - zh->reg->ptr_i] =
	    (char*)zh->reg->key_buf + zh->reg->key_buf_used;

        ch = zebraExplain_lookupSU (zei, attrSet, attrUse);
        if (ch < 0)
            ch = zebraExplain_addSU (zei, attrSet, attrUse);

        assert (ch > 0);
	zh->reg->key_buf_used +=
	    key_SU_encode (ch,((char*)zh->reg->key_buf) +
                           zh->reg->key_buf_used);
        while (*src)
            ((char*)zh->reg->key_buf) [(zh->reg->key_buf_used)++] = *src++;
        src++;
        ((char*)(zh->reg->key_buf))[(zh->reg->key_buf_used)++] = '\0';
        ((char*)(zh->reg->key_buf))[(zh->reg->key_buf_used)++] = cmd;

        key.len = 3;
	if (key.mem[1]) /* filter specify record ID */
	    key.mem[0] = key.mem[1];
	else
	    key.mem[0] = sysno;
	key.mem[1] = key.mem[2];  /* section_id */
	key.mem[2] = key.mem[3];  /* sequence .. */

        memcpy ((char*)zh->reg->key_buf + zh->reg->key_buf_used,
		&key, sizeof(key));
        (zh->reg->key_buf_used) += sizeof(key);
        off = src - reckeys->buf;
    }
    assert (off == reckeys->buf_used);
    iscz1_stop(decode_handle);
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

void extract_add_it_key (ZebraHandle zh,
			 int reg_type,
			 const char *str, int slen, struct it_key *key)
{
    char *dst;
    struct recKeys *keys = &zh->reg->keys;
    const char *src = (char*) key;
    
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

    iscz1_encode(keys->codec_handle, &dst, &src);

    *dst++ = reg_type;
    memcpy (dst, str, slen);
    dst += slen;
    *dst++ = '\0';
    keys->buf_used = dst - keys->buf;
}

void extract_add_index_string (RecWord *p, const char *str, int length)
{
    struct it_key key;
    key.len = 4;
    key.mem[0] = p->attrSet * 65536 + p->attrUse;
    key.mem[1] = p->record_id;
    key.mem[2] = p->section_id;
    key.mem[3] = p->seqno;

#if 1
    /* just for debugging .. */
    yaz_log(YLOG_LOG, "add: set=%d use=%d "
	    "record_id=%lld section_id=%lld seqno=%lld",
	    p->attrSet, p->attrUse, p->record_id, p->section_id, p->seqno);
#endif

    extract_add_it_key(p->extractCtrl->handle,  p->reg_type, str,
		       length, &key);
}

static void extract_add_sort_string (RecWord *p, const char *str,
				     int length)
{
    ZebraHandle zh = p->extractCtrl->handle;
    struct sortKeys *sk = &zh->reg->sortKeys;
    int off = 0;

    while (off < sk->buf_used)
    {
        int set, use, slen;

        off += key_SU_decode(&set, sk->buf + off);
        off += key_SU_decode(&use, sk->buf + off);
        off += key_SU_decode(&slen, sk->buf + off);
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

void extract_add_string (RecWord *p, const char *string, int length)
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
    
    yaz_log(YLOG_DEBUG, "Incomplete field, w='%.*s'", p->length, p->string);

    if (remain > 0)
	map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain, 0);

    while (map)
    {
	char buf[IT_MAX_WORD+1];
	int i, remain;

	/* Skip spaces */
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->length - (b - p->string);
	    if (remain > 0)
		map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain, 0);
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
		map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain, 0);
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
    const char *b = p->string;
    char buf[IT_MAX_WORD+1];
    const char **map = 0;
    int i = 0, remain = p->length;
    int first; /* first position */

    yaz_log(YLOG_DEBUG, "Complete field, w='%.*s'", p->length, p->string);

    if (remain > 0)
	map = zebra_maps_input (p->zebra_maps, p->reg_type, &b, remain, 1);

    while (remain > 0 && i < IT_MAX_WORD)
    {
	while (map && *map && **map == *CHR_SPACE)
	{
	    remain = p->length - (b - p->string);

	    if (remain > 0)
	    {
		first = i ? 0 : 1;
		map = zebra_maps_input(p->zebra_maps, p->reg_type, &b, remain, first);
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
	    remain = p->length  - (b - p->string);
	    if (remain > 0)
	    {
		map = zebra_maps_input (p->zebra_maps, p->reg_type, &b,
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

void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid)
{
    ZebraHandle zh = (ZebraHandle) (p->handle);
    zebraExplain_addSchema (zh->reg->zei, oid);
}

void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
                            int cmd, struct sortKeys *sk)
{
    SortIdx sortIdx = zh->reg->sortIdx;
    int off = 0;

    sortIdx_sysno (sortIdx, sysno);

    while (off < sk->buf_used)
    {
        int set, use, slen;
        
        off += key_SU_decode(&set, sk->buf + off);
        off += key_SU_decode(&use, sk->buf + off);
        off += key_SU_decode(&slen, sk->buf + off);
        
        sortIdx_type(sortIdx, use);
        if (cmd == 1)
            sortIdx_add(sortIdx, sk->buf + off, slen);
        else
            sortIdx_add(sortIdx, "", 1);
        off += slen;
    }
}

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
