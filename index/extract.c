/*
 * Copyright (C) 1994-2002, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: extract.c,v 1.110 2002-02-20 17:30:01 adam Exp $
 */
#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include <recctrl.h>
#include <charmap.h>
#include <sortidx.h>
#include "index.h"
#include "zserver.h"
#include "zinfo.h"

#if _FILE_OFFSET_BITS == 64
#define PRINTF_OFF_T "%Ld"
#else
#define PRINTF_OFF_T "%ld"
#endif

#ifndef ZEBRASDR
#define ZEBRASDR 0
#endif

#if ZEBRASDR
#include "zebrasdr.h"
#endif

static int records_inserted = 0;
static int records_updated = 0;
static int records_deleted = 0;
static int records_processed = 0;

static void logRecord (int showFlag)
{
    if (!showFlag)
        ++records_processed;
    if (showFlag || !(records_processed % 1000))
    {
        logf (LOG_LOG, "Records: %7d i/u/d %d/%d/%d", 
              records_processed, records_inserted, records_updated,
              records_deleted);
    }
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
    chS = zebraExplain_lookupSU (zh->service->zei, attrSetS, attrUseS);
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
	    
	    if ((attset = data1_get_attset (zh->service->dh, attset_str)))
	    {
		data1_att *att;
		attSet = attset->reference;
		att = data1_getattbyname(zh->service->dh, attset, attname_str);
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
            else if (!strcmp (special, "filename"))
                spec_src = fname;
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
     
static void recordLogPreamble (int level, const char *msg, void *info)
{
    struct recordLogInfo *p = (struct recordLogInfo *) info;
    FILE *outf = yaz_log_file ();

    if (level & LOG_LOG)
        return ;
    fprintf (outf, "File %s, offset %d, type %s\n",
             p->rGroup->recordType, p->recordOffset, p->fname);
    log_event_start (NULL, NULL);
}


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
    struct recordLogInfo logInfo;
    off_t recordOffset = 0;

    if (fi->fd != -1)
    {
	struct recExtractCtrl extractCtrl;

        /* we are going to read from a file, so prepare the extraction */
	int i;

	zh->keys.buf_used = 0;
	zh->keys.prevAttrUse = -1;
	zh->keys.prevAttrSet = -1;
	zh->keys.prevSeqNo = 0;
	
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
	extractCtrl.dh = zh->service->dh;
        extractCtrl.handle = zh;
	for (i = 0; i<256; i++)
	{
	    if (zebra_maps_is_positioned(zh->service->zebra_maps, i))
		extractCtrl.seqno[i] = 1;
	    else
		extractCtrl.seqno[i] = 0;
	}
	extractCtrl.zebra_maps = zh->service->zebra_maps;
	extractCtrl.flagShowRecords = !rGroup->flagRw;

        if (!rGroup->flagRw)
            printf ("File: %s " PRINTF_OFF_T "\n", fname, recordOffset);

        logInfo.fname = fname;
        logInfo.recordOffset = recordOffset;
        logInfo.rGroup = rGroup;
        log_event_start (recordLogPreamble, &logInfo);

        r = (*recType->extract)(clientData, &extractCtrl);

        log_event_start (NULL, NULL);

	if (r == RECCTRL_EXTRACT_EOF)
	    return 0;
	else if (r == RECCTRL_EXTRACT_ERROR)
	{
            /* error occured during extraction ... */
            if (rGroup->flagRw &&
		records_processed < rGroup->fileVerboseLimit)
            {
                logf (LOG_WARN, "fail %s %s " PRINTF_OFF_T, rGroup->recordType,
                      fname, recordOffset);
            }
            return 0;
        }
        if (zh->keys.buf_used == 0)
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
        
            matchStr = fileMatchStr (zh, &zh->keys, rGroup, fname, 
                                     rGroup->recordId);
            if (matchStr)
            {
                rinfo = dict_lookup (zh->service->matchDict, matchStr);
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
        if (records_processed < rGroup->fileVerboseLimit)
            logf (LOG_LOG, "add %s %s " PRINTF_OFF_T, rGroup->recordType,
                  fname, recordOffset);
        rec = rec_new (zh->service->records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zh->service->zei, rec);

        if (matchStr)
        {
            dict_insert (zh->service->matchDict, matchStr, sizeof(*sysno), sysno);
        }
        extract_flushRecordKeys (zh, *sysno, 1, &zh->keys);
	extract_flushSortKeys (zh, *sysno, 1, &zh->sortKeys);

        records_inserted++;
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
	    logf (LOG_LOG, "skipped %s %s " PRINTF_OFF_T, rGroup->recordType,
		  fname, recordOffset);
	    extract_flushSortKeys (zh, *sysno, -1, &zh->sortKeys);
	    rec_rm (&rec);
	    logRecord (0);
	    return 1;
	}
        delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];
	extract_flushSortKeys (zh, *sysno, 0, &zh->sortKeys);
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
                if (records_processed < rGroup->fileVerboseLimit)
                    logf (LOG_LOG, "delete %s %s " PRINTF_OFF_T,
                         rGroup->recordType, fname, recordOffset);
                records_deleted++;
                if (matchStr)
                    dict_delete (zh->service->matchDict, matchStr);
                rec_del (zh->service->records, &rec);
            }
	    rec_rm (&rec);
            logRecord (0);
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
                if (records_processed < rGroup->fileVerboseLimit)
                    logf (LOG_LOG, "update %s %s " PRINTF_OFF_T,
                        rGroup->recordType, fname, recordOffset);
                extract_flushRecordKeys (zh, *sysno, 1, &zh->keys);
                records_updated++;
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
    if (zh->keys.buf_used > 0 && rGroup->flagStoreKeys == 1)
    {
#if 1
        rec->size[recInfo_delKeys] = zh->keys.buf_used;
        rec->info[recInfo_delKeys] = zh->keys.buf;
        zh->keys.buf = NULL;
        zh->keys.buf_max = 0;
#else
        rec->info[recInfo_delKeys] = xmalloc (reckeys.buf_used);
        rec->size[recInfo_delKeys] = reckeys.buf_used;
        memcpy (rec->info[recInfo_delKeys], reckeys.buf,
                rec->size[recInfo_delKeys]);
#endif
    }
    else
    {
        rec->info[recInfo_delKeys] = NULL;
        rec->size[recInfo_delKeys] = 0;
    }

    /* save file size of original record */
    zebraExplain_recordBytesIncrement (zh->service->zei,
                                       - recordAttr->recordSize);
    recordAttr->recordSize = fi->file_moffset - recordOffset;
    if (!recordAttr->recordSize)
	recordAttr->recordSize = fi->file_max - recordOffset;
    zebraExplain_recordBytesIncrement (zh->service->zei,
                                       recordAttr->recordSize);

    /* set run-number for this record */
    recordAttr->runNumber = zebraExplain_runNumberIncrement (zh->service->zei,
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
    rec_put (zh->service->records, &rec);
    logRecord (0);
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
        if (!(rGroup->recordType = res_get (zh->service->res, ext_res)))
        {
            sprintf (ext_res, "%srecordType", gprefix);
            rGroup->recordType = res_get (zh->service->res, ext_res);
        }
    }
    if (!rGroup->recordType)
    {
        if (records_processed < rGroup->fileVerboseLimit)
            logf (LOG_LOG, "? %s", fname);
        return 0;
    }
    if (!*rGroup->recordType)
	return 0;
    if (!(recType =
	  recType_byName (zh->service->recTypes, rGroup->recordType, subType,
			  &clientData)))
    {
        logf (LOG_WARN, "No such record type: %s", rGroup->recordType);
        return 0;
    }

    /* determine match criteria */
    if (!rGroup->recordId)
    {
        sprintf (ext_res, "%srecordId.%s", gprefix, ext);
        rGroup->recordId = res_get (zh->service->res, ext_res);
    }

    /* determine database name */
    if (!rGroup->databaseName)
    {
        sprintf (ext_res, "%sdatabase.%s", gprefix, ext);
        if (!(rGroup->databaseName = res_get (zh->service->res, ext_res)))
        {
            sprintf (ext_res, "%sdatabase", gprefix);
            rGroup->databaseName = res_get (zh->service->res, ext_res);
        }
    }
    if (!rGroup->databaseName)
        rGroup->databaseName = "Default";

    /* determine if explain database */
    
    sprintf (ext_res, "%sexplainDatabase", gprefix);
    rGroup->explainDatabase =
	atoi (res_get_def (zh->service->res, ext_res, "0"));

    /* announce database */
    if (zebraExplain_curDatabase (zh->service->zei, rGroup->databaseName))
    {
        if (zebraExplain_newDatabase (zh->service->zei, rGroup->databaseName,
				      rGroup->explainDatabase))
	    return 0;
    }

    if (rGroup->flagStoreData == -1)
    {
        const char *sval;
        sprintf (ext_res, "%sstoreData.%s", gprefix, ext);
        if (!(sval = res_get (zh->service->res, ext_res)))
        {
            sprintf (ext_res, "%sstoreData", gprefix);
            sval = res_get (zh->service->res, ext_res);
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
        sval = res_get (zh->service->res, ext_res);
	if (!sval)
        {
            sprintf (ext_res, "%sstoreKeys", gprefix);
            sval = res_get (zh->service->res, ext_res);
        }
	if (!sval)
	    sval = res_get (zh->service->res, "storeKeys");
        if (sval)
            rGroup->flagStoreKeys = atoi (sval);
    }
    if (rGroup->flagStoreKeys == -1)
        rGroup->flagStoreKeys = 0;

    if (sysno && deleteFlag)
        fd = -1;
    else
    {
        if ((fd = open (fname, O_BINARY|O_RDONLY)) == -1)
        {
            logf (LOG_WARN|LOG_ERRNO, "open %s", fname);
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
    RecordAttr *recordAttr;
    struct recExtractCtrl extractCtrl;
    int i, r;
    char *matchStr;
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

        if (matchStr)
        {
            dict_insert (zh->service->matchDict, matchStr,
                         sizeof(*sysno), sysno);
        }
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

int explain_extract (void *handle, Record rec, data1_node *n)
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
        if (zh->key_buf_used + 1024 > (zh->ptr_top-zh->ptr_i)*sizeof(char*))
            extract_flushWriteKeys (zh);
        ++(zh->ptr_i);
        (zh->key_buf)[zh->ptr_top - zh->ptr_i] =
	    (char*)zh->key_buf + zh->key_buf_used;
#if SU_SCHEME
#else
        ch = zebraExplain_lookupSU (zei, attrSet, attrUse);
        if (ch < 0)
            ch = zebraExplain_addSU (zei, attrSet, attrUse);
#endif
        assert (ch > 0);
	zh->key_buf_used +=
	    key_SU_encode (ch,((char*)zh->key_buf) + zh->key_buf_used);

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

void extract_flushWriteKeys (ZebraHandle zh)
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

void extract_add_index_string (RecWord *p, const char *string,
                               int length)
{
    char *dst;
    unsigned char attrSet;
    unsigned short attrUse;
    int lead = 0;
    int diff = 0;
    int *pseqno = &p->seqnos[p->reg_type];
    ZebraHandle zh = p->extractCtrl->handle;
    ZebraExplainInfo zei = zh->service->zei;
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

#if SU_SCHEME
    if ((lead & 3) < 3)
    {
        int ch = zebraExplain_lookupSU (zei, attrSet, attrUse);
        if (ch < 0)
        {
            ch = zebraExplain_addSU (zei, attrSet, attrUse);
            yaz_log (LOG_LOG, "addSU set=%d use=%d SU=%d",
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

void extract_token_add (RecWord *p)
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

void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid)
{
    ZebraHandle zh = (ZebraHandle) (p->handle);
    zebraExplain_addSchema (zh->service->zei, oid);
}

void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
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

