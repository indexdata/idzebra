/* $Id: extract.c,v 1.134 2002-12-30 12:56:07 adam Exp $
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


#include <stdio.h>
#include <assert.h>
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
        logf (LOG_LOG, "Records: %7d i/u/d %d/%d/%d", 
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
}

static const char **searchRecordKey (ZebraHandle zh,
                                     struct recKeys *reckeys,
				     int attrSetS, int attrUseS)
{
    static const char *ws[32];
    int off = 0;
    int startSeq = -1;
    int i;
    int seqno = 0;
#if SU_SCHEME
    int chS, ch;
#else
    short attrUse;
    char attrSet;
#endif

    for (i = 0; i<32; i++)
        ws[i] = NULL;

#if SU_SCHEME
    chS = zebraExplain_lookupSU (zh->reg->zei, attrSetS, attrUseS);
    if (chS < 0)
	return ws;
#endif
    while (off < reckeys->buf_used)
    {

        const char *src = reckeys->buf + off;
	const char *wstart;
        int lead;
    
        lead = *src++;
#if SU_SCHEME
	if ((lead & 3)<3)
	{
	    memcpy (&ch, src, sizeof(ch));
	    src += sizeof(ch);
	}
#else
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
#endif
        wstart = src;
        while (*src++)
            ;
        if (lead & 60)
            seqno += ((lead>>2) & 15)-1;
        else
        {
            memcpy (&seqno, src, sizeof(seqno));
            src += sizeof(seqno);
        }
	if (
#if SU_SCHEME
	    ch == chS
#else
	    attrUseS == attrUse && attrSetS == attrSet
#endif
	    )
        {
            int woff;


            if (startSeq == -1)
                startSeq = seqno;
            woff = seqno - startSeq;
            if (woff >= 0 && woff < 31)
                ws[woff] = wstart;
        }

        off = src - reckeys->buf;
    }
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
                           struct recKeys *reckeys, struct recordGroup *rGroup,
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
                logf (LOG_WARN, "Missing ) in match criteria %s in group %s",
                      spec, rGroup->groupName ? rGroup->groupName : "none");
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
                logf (LOG_WARN, "Record didn't contain match"
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
                spec_src = rGroup->groupName;
            else if (!strcmp (special, "database"))
                spec_src = rGroup->databaseName;
            else if (!strcmp (special, "filename")) {
                spec_src = fname;
	    }
            else if (!strcmp (special, "type"))
                spec_src = rGroup->recordType;
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
            logf (LOG_WARN, "Syntax error in match criteria %s in group %s",
                  spec, rGroup->groupName ? rGroup->groupName : "none");
            return NULL;
        }
        *dst++ = 1;
    }
    if (dst == dstBuf)
    {
        logf (LOG_WARN, "No match criteria for record %s in group %s",
              fname, rGroup->groupName ? rGroup->groupName : "none");
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
     
static int recordExtract (ZebraHandle zh,
                          SYSNO *sysno, const char *fname,
                          struct recordGroup *rGroup, int deleteFlag,
                          struct file_read_info *fi,
			  RecType recType, char *subType, void *clientData)
{
    RecordAttr *recordAttr;
    int r;
    char *matchStr;
    SYSNO sysnotmp;
    Record rec;
    off_t recordOffset = 0;

    if (fi->fd != -1)
    {
	struct recExtractCtrl extractCtrl;

        /* we are going to read from a file, so prepare the extraction */
	int i;

	zh->reg->keys.buf_used = 0;
	zh->reg->keys.prevAttrUse = -1;
	zh->reg->keys.prevAttrSet = -1;
	zh->reg->keys.prevSeqNo = 0;
	zh->reg->sortKeys.buf_used = 0;
	zh->reg->sortKeys.buf_max = 0;
	zh->reg->sortKeys.buf = 0;
	
	recordOffset = fi->file_moffset;
	extractCtrl.offset = fi->file_moffset;
	extractCtrl.readf = file_read;
	extractCtrl.seekf = file_seek;
	extractCtrl.tellf = file_tell;
	extractCtrl.endf = file_end;
	extractCtrl.fh = fi;
	extractCtrl.subType = subType;
	extractCtrl.init = extract_init;
	extractCtrl.tokenAdd = extract_token_add;
	extractCtrl.schemaAdd = extract_schema_add;
	extractCtrl.dh = zh->reg->dh;
        extractCtrl.handle = zh;
	for (i = 0; i<256; i++)
	{
	    if (zebra_maps_is_positioned(zh->reg->zebra_maps, i))
		extractCtrl.seqno[i] = 1;
	    else
		extractCtrl.seqno[i] = 0;
	}
	extractCtrl.zebra_maps = zh->reg->zebra_maps;
	extractCtrl.flagShowRecords = !rGroup->flagRw;

        if (!rGroup->flagRw)
            printf ("File: %s " PRINTF_OFF_T "\n", fname, recordOffset);
        if (rGroup->flagRw)
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
            if (rGroup->flagRw &&
		zh->records_processed < rGroup->fileVerboseLimit)
            {
                logf (LOG_WARN, "fail %s %s " PRINTF_OFF_T, rGroup->recordType,
                      fname, recordOffset);
            }
            return 0;
        }
	else if (r == RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER)
	{
            /* error occured during extraction ... */
            if (rGroup->flagRw &&
		zh->records_processed < rGroup->fileVerboseLimit)
            {
                logf (LOG_WARN, "no filter for %s %s " 
                      PRINTF_OFF_T, rGroup->recordType,
                      fname, recordOffset);
            }
            return 0;
        }
        if (zh->reg->keys.buf_used == 0)
        {
            /* the extraction process returned no information - the record
               is probably empty - unless flagShowRecords is in use */
            if (!rGroup->flagRw)
                return 1;
	    
	    logf (LOG_WARN, "empty %s %s " PRINTF_OFF_T, rGroup->recordType,
		  fname, recordOffset);
            return 1;
        }
    }

    /* perform match if sysno not known and if match criteria is specified */
       
    matchStr = NULL;
    if (!sysno) 
    {
        sysnotmp = 0;
        sysno = &sysnotmp;
        if (rGroup->recordId && *rGroup->recordId)
        {
            char *rinfo;
        
            matchStr = fileMatchStr (zh, &zh->reg->keys, rGroup, fname, 
                                     rGroup->recordId);
            if (matchStr)
            {
                rinfo = dict_lookup (zh->reg->matchDict, matchStr);
                if (rinfo)
                    memcpy (sysno, rinfo+1, sizeof(*sysno));
            }
            else
            {
                logf (LOG_WARN, "Bad match criteria");
                return 0;
            }
        }
    }

    if (! *sysno)
    {
        /* new record */
        if (deleteFlag)
        {
	    logf (LOG_LOG, "delete %s %s " PRINTF_OFF_T, rGroup->recordType,
		  fname, recordOffset);
            logf (LOG_WARN, "cannot delete record above (seems new)");
            return 1;
        }
        if (zh->records_processed < rGroup->fileVerboseLimit)
            logf (LOG_LOG, "add %s %s " PRINTF_OFF_T, rGroup->recordType,
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

	if (recordAttr->runNumber ==
            zebraExplain_runNumberIncrement (zh->reg->zei, 0))
	{
            yaz_log (LOG_LOG, "run number = %d", recordAttr->runNumber);
	    yaz_log (LOG_LOG, "skipped %s %s " PRINTF_OFF_T,
                     rGroup->recordType, fname, recordOffset);
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
                logf (LOG_LOG, "delete %s %s " PRINTF_OFF_T,
                      rGroup->recordType, fname, recordOffset);
                logf (LOG_WARN, "cannot delete file above, storeKeys false");
            }
            else
            {
                if (zh->records_processed < rGroup->fileVerboseLimit)
                    logf (LOG_LOG, "delete %s %s " PRINTF_OFF_T,
                         rGroup->recordType, fname, recordOffset);
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
                logf (LOG_LOG, "update %s %s " PRINTF_OFF_T,
                      rGroup->recordType, fname, recordOffset);
                logf (LOG_WARN, "cannot update file above, storeKeys false");
            }
            else
            {
                if (zh->records_processed < rGroup->fileVerboseLimit)
                    logf (LOG_LOG, "update %s %s " PRINTF_OFF_T,
                        rGroup->recordType, fname, recordOffset);
                extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
                extract_flushRecordKeys (zh, *sysno, 1, &zh->reg->keys);
                zh->records_updated++;
            }
        }
    }
    /* update file type */
    xfree (rec->info[recInfo_fileType]);
    rec->info[recInfo_fileType] =
        rec_strdup (rGroup->recordType, &rec->size[recInfo_fileType]);

    /* update filename */
    xfree (rec->info[recInfo_filename]);
    rec->info[recInfo_filename] =
        rec_strdup (fname, &rec->size[recInfo_filename]);

    /* update delete keys */
    xfree (rec->info[recInfo_delKeys]);
    if (zh->reg->keys.buf_used > 0 && rGroup->flagStoreKeys == 1)
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
    if (rGroup->flagStoreData == 1)
    {
        rec->size[recInfo_storeData] = recordAttr->recordSize;
        rec->info[recInfo_storeData] = (char *)
	    xmalloc (recordAttr->recordSize);
        if (lseek (fi->fd, recordOffset, SEEK_SET) < 0)
        {
            logf (LOG_ERRNO|LOG_FATAL, "seek to " PRINTF_OFF_T " in %s",
                  recordOffset, fname);
            exit (1);
        }
        if (read (fi->fd, rec->info[recInfo_storeData], recordAttr->recordSize)
	    < recordAttr->recordSize)
        {
            logf (LOG_ERRNO|LOG_FATAL, "read %d bytes of %s",
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
        rec_strdup (rGroup->databaseName, &rec->size[recInfo_databaseName]); 

    /* update offset */
    recordAttr->recordOffset = recordOffset;
    
    /* commit this record */
    rec_put (zh->reg->records, &rec);
    logRecord (zh);
    return 1;
}

int fileExtract (ZebraHandle zh, SYSNO *sysno, const char *fname, 
                 const struct recordGroup *rGroupP, int deleteFlag)
{
    int r, i, fd;
    char gprefix[128];
    char ext[128];
    char ext_res[128];
    char subType[128];
    RecType recType;
    struct recordGroup rGroupM;
    struct recordGroup *rGroup = &rGroupM;
    struct file_read_info *fi;
    void *clientData;

    memcpy (rGroup, rGroupP, sizeof(*rGroupP));
   
    if (!rGroup->groupName || !*rGroup->groupName)
        *gprefix = '\0';
    else
        sprintf (gprefix, "%s.", rGroup->groupName);

    logf (LOG_DEBUG, "fileExtract %s", fname);

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
    if (!rGroup->recordType)
    {
        sprintf (ext_res, "%srecordType.%s", gprefix, ext);
        if (!(rGroup->recordType = res_get (zh->res, ext_res)))
        {
            sprintf (ext_res, "%srecordType", gprefix);
            rGroup->recordType = res_get (zh->res, ext_res);
        }
    }
    if (!rGroup->recordType)
    {
        if (zh->records_processed < rGroup->fileVerboseLimit)
            logf (LOG_LOG, "? %s", fname);
        return 0;
    }
    if (!*rGroup->recordType)
	return 0;
    if (!(recType =
	  recType_byName (zh->reg->recTypes, rGroup->recordType, subType,
			  &clientData)))
    {
        logf (LOG_WARN, "No such record type: %s", rGroup->recordType);
        return 0;
    }

    /* determine match criteria */
    if (!rGroup->recordId)
    {
        sprintf (ext_res, "%srecordId.%s", gprefix, ext);
        rGroup->recordId = res_get (zh->res, ext_res);
    }

    /* determine database name */
    if (!rGroup->databaseName)
    {
        sprintf (ext_res, "%sdatabase.%s", gprefix, ext);
        if (!(rGroup->databaseName = res_get (zh->res, ext_res)))
        {
            sprintf (ext_res, "%sdatabase", gprefix);
            rGroup->databaseName = res_get (zh->res, ext_res);
        }
    }
    if (!rGroup->databaseName)
        rGroup->databaseName = "Default";

    /* determine if explain database */
    
    sprintf (ext_res, "%sexplainDatabase", gprefix);
    rGroup->explainDatabase =
	atoi (res_get_def (zh->res, ext_res, "0"));

    /* announce database */
    if (zebraExplain_curDatabase (zh->reg->zei, rGroup->databaseName))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, rGroup->databaseName,
				      rGroup->explainDatabase))
	    return 0;
    }

    if (rGroup->flagStoreData == -1)
    {
        const char *sval;
        sprintf (ext_res, "%sstoreData.%s", gprefix, ext);
        if (!(sval = res_get (zh->res, ext_res)))
        {
            sprintf (ext_res, "%sstoreData", gprefix);
            sval = res_get (zh->res, ext_res);
        }
        if (sval)
            rGroup->flagStoreData = atoi (sval);
    }
    if (rGroup->flagStoreData == -1)
        rGroup->flagStoreData = 0;

    if (rGroup->flagStoreKeys == -1)
    {
        const char *sval;

        sprintf (ext_res, "%sstoreKeys.%s", gprefix, ext);
        sval = res_get (zh->res, ext_res);
	if (!sval)
        {
            sprintf (ext_res, "%sstoreKeys", gprefix);
            sval = res_get (zh->res, ext_res);
        }
	if (!sval)
	    sval = res_get (zh->res, "storeKeys");
        if (sval)
            rGroup->flagStoreKeys = atoi (sval);
    }
    if (rGroup->flagStoreKeys == -1)
        rGroup->flagStoreKeys = 0;

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
            logf (LOG_WARN|LOG_ERRNO, "open %s", full_rep);
            return 0;
        }
    }
    fi = file_read_start (fd);
    do
    {
        file_begin (fi);
        r = recordExtract (zh, sysno, fname, rGroup, deleteFlag, fi,
                           recType, subType, clientData);
    } while (r && !sysno && fi->file_more);
    file_read_stop (fi);
    if (fd != -1)
        close (fd);
    return r;
}
int extract_rec_in_mem (ZebraHandle zh, const char *recordType,
                        const char *buf, size_t buf_size,
                        const char *databaseName, int delete_flag,
                        int test_mode, int *sysno,
                        int store_keys, int store_data,
                        const char *match_criteria)
{
    struct recordGroup rGroup;
    rGroup.groupName = NULL;
    rGroup.databaseName = (char *)databaseName;
    rGroup.path = NULL;
    rGroup.recordId = NULL;
    rGroup.recordType = (char *)recordType;
    rGroup.flagStoreData = store_data;
    rGroup.flagStoreKeys = store_keys;
    rGroup.flagRw = 1;
    rGroup.databaseNamePath = 0;
    rGroup.explainDatabase = 0;
    rGroup.fileVerboseLimit = 100000;
    rGroup.followLinks = -1;
    return (bufferExtractRecord (zh,
				 buf, buf_size,
				 &rGroup,
				 delete_flag,
				 test_mode,
				 sysno,
				 match_criteria,
				 "<no file>"));
}
/*
  If sysno is provided, then it's used to identify the reocord.
  If not, and match_criteria is provided, then sysno is guessed
  If not, and a record is provided, then sysno is got from there
  
 */
int bufferExtractRecord (ZebraHandle zh, 
			 const char *buf, size_t buf_size,
			 struct recordGroup *rGroup, 
			 int delete_flag,
			 int test_mode, 
			 int *sysno,
			 const char *match_criteria,
			 const char *fname)

{
    RecordAttr *recordAttr;
    struct recExtractCtrl extractCtrl;
    int i, r;
    char *matchStr = 0;
    RecType recType;
    char subType[1024];
    void *clientData;
    SYSNO sysnotmp;
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
    if (zebraExplain_curDatabase (zh->reg->zei, rGroup->databaseName))
    {
        if (zebraExplain_newDatabase (zh->reg->zei, rGroup->databaseName, 0))
	    return 0;
    }

    if (!(rGroup->recordType)) {
        logf (LOG_WARN, "No such record type defined");
        return 0;
    }

    if (!(recType =
	  recType_byName (zh->reg->recTypes, rGroup->recordType, subType,
			  &clientData)))
    {
        logf (LOG_WARN, "No such record type: %s", rGroup->recordType);
        return 0;
    }
    zh->reg->keys.buf_used = 0;
    zh->reg->keys.prevAttrUse = -1;
    zh->reg->keys.prevAttrSet = -1;
    zh->reg->keys.prevSeqNo = 0;
    zh->reg->sortKeys.buf_used = 0;
    zh->reg->sortKeys.buf_max = 0;
    zh->reg->sortKeys.buf = 0;

    extractCtrl.subType = subType;
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->reg->dh;
    extractCtrl.handle = zh;
    extractCtrl.zebra_maps = zh->reg->zebra_maps;
    extractCtrl.flagShowRecords = 0;
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
	yaz_log (LOG_WARN, "extract error: generic");
	return 0;
    }
    else if (r == RECCTRL_EXTRACT_ERROR_NO_SUCH_FILTER)
    {
	/* error occured during extraction ... */
	yaz_log (LOG_WARN, "extract error: no such filter");
	return 0;
    }
    if (zh->reg->keys.buf_used == 0)
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
    matchStr = NULL;

    if (! *sysno) {
      char *rinfo;
      if (strlen(match_criteria) > 0) {
	matchStr = (char *)match_criteria;
      } else {
	if (rGroup->recordId && *rGroup->recordId) {
	  matchStr = fileMatchStr (zh, &zh->reg->keys, rGroup, fname, 
				   rGroup->recordId);
	}
      }
      if (matchStr) {
	rinfo = dict_lookup (zh->reg->matchDict, matchStr);
	if (rinfo)
	  memcpy (sysno, rinfo+1, sizeof(*sysno));
      } else {
	logf (LOG_WARN, "Bad match criteria (recordID)");
	return 0;
      }

    }

    if (! *sysno)
    {
        /* new record */
        if (delete_flag)
        {
	    logf (LOG_LOG, "delete %s %s %ld", rGroup->recordType,
		  fname, (long) recordOffset);
            logf (LOG_WARN, "cannot delete record above (seems new)");
            return 1;
        }
	logf (LOG_LOG, "add %s %s %ld", rGroup->recordType, fname,
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

        rec = rec_get (zh->reg->records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zh->reg->zei, rec);

	if (recordAttr->runNumber ==
	    zebraExplain_runNumberIncrement (zh->reg->zei, 0))
	{
	    logf (LOG_LOG, "skipped %s %s %ld", rGroup->recordType,
		  fname, (long) recordOffset);
	    extract_flushSortKeys (zh, *sysno, -1, &zh->reg->sortKeys);
	    rec_rm (&rec);
            logRecord(zh);
	    return 1;
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
                logf (LOG_LOG, "delete %s %s %ld", rGroup->recordType,
                      fname, (long) recordOffset);
                logf (LOG_WARN, "cannot delete file above, storeKeys false");
            }
            else
            {
		logf (LOG_LOG, "delete %s %s %ld", rGroup->recordType,
		      fname, (long) recordOffset);
                zh->records_deleted++;
                if (matchStr)
                    dict_delete (zh->reg->matchDict, matchStr);
                rec_del (zh->reg->records, &rec);
            }
	    rec_rm (&rec);
            logRecord(zh);
            return 1;
        }
        else
        {
            /* record going to be updated */
            if (!delkeys.buf_used)
            {
                logf (LOG_LOG, "update %s %s %ld", rGroup->recordType,
                      fname, (long) recordOffset);
                logf (LOG_WARN, "cannot update file above, storeKeys false");
            }
            else
            {
		logf (LOG_LOG, "update %s %s %ld", rGroup->recordType,
		      fname, (long) recordOffset);
                extract_flushSortKeys (zh, *sysno, 1, &zh->reg->sortKeys);
                extract_flushRecordKeys (zh, *sysno, 1, &zh->reg->keys);
                zh->records_updated++;
            }
        }
    }
    /* update file type */
    xfree (rec->info[recInfo_fileType]);
    rec->info[recInfo_fileType] =
        rec_strdup (rGroup->recordType, &rec->size[recInfo_fileType]);

    /* update filename */
    xfree (rec->info[recInfo_filename]);
    rec->info[recInfo_filename] =
        rec_strdup (fname, &rec->size[recInfo_filename]);

    /* update delete keys */
    xfree (rec->info[recInfo_delKeys]);
    if (zh->reg->keys.buf_used > 0 && rGroup->flagStoreKeys == 1)
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
    if (rGroup->flagStoreData == 1)
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
        rec_strdup (rGroup->databaseName, &rec->size[recInfo_databaseName]); 

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

    zh->reg->keys.buf_used = 0;
    zh->reg->keys.prevAttrUse = -1;
    zh->reg->keys.prevAttrSet = -1;
    zh->reg->keys.prevSeqNo = 0;
    zh->reg->sortKeys.buf_used = 0;
    
    extractCtrl.init = extract_init;
    extractCtrl.tokenAdd = extract_token_add;
    extractCtrl.schemaAdd = extract_schema_add;
    extractCtrl.dh = zh->reg->dh;
    for (i = 0; i<256; i++)
	extractCtrl.seqno[i] = 0;
    extractCtrl.zebra_maps = zh->reg->zebra_maps;
    extractCtrl.flagShowRecords = 0;
    extractCtrl.handle = handle;
    
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
#if SU_SCHEME
#else
    unsigned char attrSet = (unsigned char) -1;
    unsigned short attrUse = (unsigned short) -1;
#endif
    int seqno = 0;
    int off = 0;
    int ch = 0;
    ZebraExplainInfo zei = zh->reg->zei;

    if (!zh->reg->key_buf)
    {
	int mem= 1024*1024* atoi( res_get_def( zh->res, "memmax", "8"));
	if (mem <= 0)
	{
	    logf(LOG_WARN, "Invalid memory setting, using default 8 MB");
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
        int lead;
    
        lead = *src++;

#if SU_SCHEME
	if ((lead & 3) < 3)
	{
	    memcpy (&ch, src, sizeof(ch));
	    src += sizeof(ch);
	}
#else
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
#endif
        if (zh->reg->key_buf_used + 1024 > 
            (zh->reg->ptr_top -zh->reg->ptr_i)*sizeof(char*))
            extract_flushWriteKeys (zh);
        ++(zh->reg->ptr_i);
        (zh->reg->key_buf)[zh->reg->ptr_top - zh->reg->ptr_i] =
	    (char*)zh->reg->key_buf + zh->reg->key_buf_used;
#if SU_SCHEME
#else
        ch = zebraExplain_lookupSU (zei, attrSet, attrUse);
        if (ch < 0)
            ch = zebraExplain_addSU (zei, attrSet, attrUse);
#endif
        assert (ch > 0);
	zh->reg->key_buf_used +=
	    key_SU_encode (ch,((char*)zh->reg->key_buf) +
                           zh->reg->key_buf_used);

        while (*src)
            ((char*)zh->reg->key_buf) [(zh->reg->key_buf_used)++] = *src++;
        src++;
        ((char*)(zh->reg->key_buf))[(zh->reg->key_buf_used)++] = '\0';
        ((char*)(zh->reg->key_buf))[(zh->reg->key_buf_used)++] = cmd;

        if (lead & 60)
            seqno += ((lead>>2) & 15)-1;
        else
        {
            memcpy (&seqno, src, sizeof(seqno));
            src += sizeof(seqno);
        }
        key.seqno = seqno;
        key.sysno = sysno;
        memcpy ((char*)zh->reg->key_buf + zh->reg->key_buf_used, &key, sizeof(key));
        (zh->reg->key_buf_used) += sizeof(key);
        off = src - reckeys->buf;
    }
    assert (off == reckeys->buf_used);
}

void extract_flushWriteKeys (ZebraHandle zh)
{
    FILE *outf;
    char out_fname[200];
    char *prevcp, *cp;
    struct encode_info encode_info;
    int ptr_i = zh->reg->ptr_i;
#if SORT_EXTRA
    int i;
#endif
    if (!zh->reg->key_buf || ptr_i <= 0)
        return;

    (zh->reg->key_file_no)++;
    logf (LOG_LOG, "sorting section %d", (zh->reg->key_file_no));
#if !SORT_EXTRA
    qsort (zh->reg->key_buf + zh->reg->ptr_top - ptr_i, ptr_i,
               sizeof(char*), key_qsort_compare);
    extract_get_fname_tmp (zh, out_fname, zh->reg->key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", out_fname);
        exit (1);
    }
    logf (LOG_LOG, "writing section %d", zh->reg->key_file_no);
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
            encode_key_flush ( &encode_info, outf);
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
    logf (LOG_LOG, "finished section %d", zh->reg->key_file_no);
    zh->reg->ptr_i = 0;
    zh->reg->key_buf_used = 0;
}

void extract_add_index_string (RecWord *p, const char *string,
                               int length)
{
    char *dst;
    unsigned char attrSet;
    unsigned short attrUse;
    int lead = 0;
    int diff = 0;
    int *pseqno = &p->seqno;
    ZebraHandle zh = p->extractCtrl->handle;
    ZebraExplainInfo zei = zh->reg->zei;
    struct recKeys *keys = &zh->reg->keys;
    
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

#if SU_SCHEME
    if ((lead & 3) < 3)
    {
        int ch = zebraExplain_lookupSU (zei, attrSet, attrUse);
        if (ch < 0)
        {
            ch = zebraExplain_addSU (zei, attrSet, attrUse);
            yaz_log (LOG_DEBUG, "addSU set=%d use=%d SU=%d",
                     attrSet, attrUse, ch);
        }
	assert (ch > 0);
	memcpy (dst, &ch, sizeof(ch));
	dst += sizeof(ch);
    }
#else
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
#endif
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
}

static void extract_add_sort_string (RecWord *p, const char *string,
				     int length)
{
    ZebraHandle zh = p->extractCtrl->handle;
    struct sortKeys *sk = &zh->reg->sortKeys;
    size_t off = 0;

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
    memcpy (sk->buf + off, string, length);
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
        p->seqno++;
    }
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

void extract_token_add (RecWord *p)
{
    WRBUF wrbuf;

#if 0
    yaz_log (LOG_LOG, "reg_type=%c attrSet=%d attrUse=%d seqno=%d s=%.*s",
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
    size_t off = 0;

    sortIdx_sysno (sortIdx, sysno);

    while (off < sk->buf_used)
    {
        int set, use, slen, l;
        
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

#ifdef OLDENCODE
/* this is the old encode_key_write 
 * may be deleted once we are confident that the new works
 * HL 15-oct-2002
 */
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

void encode_key_flush (struct encode_info *i, FILE *outf)
{ /* dummy routine */
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
            logf (LOG_FATAL|LOG_ERRNO, "fwrite");
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
        logf (LOG_FATAL|LOG_ERRNO, "fwrite");
        exit (1);
    }
    i->keylen=0; /* ok, it's written, forget it */
    i->prevsys=0; /* forget the values too */
    i->prevseq=0;
}
#endif
