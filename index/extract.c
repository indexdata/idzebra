/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: extract.c,v $
 * Revision 1.48  1996-02-01 20:53:26  adam
 * The temporary per-record keys are compacted a little, and duplication
 * of the per-records keys are avoided when they are saved in the record
 * information buffer.
 *
 * Revision 1.47  1996/01/17  14:57:48  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int.
 *
 * Revision 1.46  1995/12/15  14:57:16  adam
 * Bug fix.
 *
 * Revision 1.45  1995/12/15  12:37:41  adam
 * In addRecordKeyAny: Writes key only when attrSet != -1.
 *
 * Revision 1.44  1995/12/12  16:00:54  adam
 * System call sync(2) used after update/commit.
 * Locking (based on fcntl) uses F_EXLCK and F_SHLCK instead of F_WRLCK
 * and F_RDLCK.
 *
 * Revision 1.43  1995/12/11  09:12:46  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.42  1995/12/07  17:38:46  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.41  1995/12/06  16:06:42  adam
 * Better diagnostics. Work on 'real' dictionary deletion.
 *
 * Revision 1.40  1995/12/05  16:57:40  adam
 * More work on regular patterns.
 *
 * Revision 1.39  1995/12/05  13:20:18  adam
 * Bug fix: file_read sometimes returned early EOF.
 *
 * Revision 1.38  1995/12/04  17:59:21  adam
 * More work on regular expression conversion.
 *
 * Revision 1.37  1995/12/04  14:22:27  adam
 * Extra arg to recType_byName.
 * Started work on new regular expression parsed input to
 * structured records.
 *
 * Revision 1.36  1995/11/30  08:34:29  adam
 * Started work on commit facility.
 * Changed a few malloc/free to xmalloc/xfree.
 *
 * Revision 1.35  1995/11/28  14:26:21  adam
 * Bug fix: recordId with constant wasn't right.
 * Bug fix: recordId dictionary entry wasn't deleted when needed.
 *
 * Revision 1.34  1995/11/28  09:09:38  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.33  1995/11/27  09:56:20  adam
 * Record info elements better enumerated. Internal store of records.
 *
 * Revision 1.32  1995/11/25  10:24:05  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.31  1995/11/24  11:31:35  adam
 * Commands add & del read filenames from stdin if source directory is
 * empty.
 * Match criteria supports 'constant' strings.
 *
 * Revision 1.30  1995/11/22  17:19:16  adam
 * Record management uses the bfile system.
 *
 * Revision 1.29  1995/11/21  15:01:14  adam
 * New general match criteria implemented.
 * New feature: document groups.
 *
 * Revision 1.28  1995/11/21  09:20:30  adam
 * Yet more work on record match.
 *
 * Revision 1.27  1995/11/20  16:59:45  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.26  1995/11/20  11:56:24  adam
 * Work on new traversal.
 *
 * Revision 1.25  1995/11/16  15:34:54  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.24  1995/11/15  19:13:08  adam
 * Work on record management.
 *
 * Revision 1.23  1995/10/27  14:00:10  adam
 * Implemented detection of database availability.
 *
 * Revision 1.22  1995/10/17  18:02:07  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.21  1995/10/10  12:24:38  adam
 * Temporary sort files are compressed.
 *
 * Revision 1.20  1995/10/06  13:52:05  adam
 * Bug fixes. Handler may abort further scanning.
 *
 * Revision 1.19  1995/10/04  12:55:16  adam
 * Bug fix in ranked search. Use=Any keys inserted.
 *
 * Revision 1.18  1995/10/04  09:37:08  quinn
 * Fixed bug.
 *
 * Revision 1.17  1995/10/03  14:28:57  adam
 * Buffered read in extract works.
 *
 * Revision 1.16  1995/10/03  14:28:45  adam
 * Work on more effecient read handler in extract.
 *
 * Revision 1.15  1995/10/02  15:42:53  adam
 * Extract uses file descriptors instead of FILE pointers.
 *
 * Revision 1.14  1995/10/02  15:29:13  adam
 * More logging in file_extract.
 *
 * Revision 1.13  1995/09/29  14:01:39  adam
 * Bug fixes.
 *
 * Revision 1.12  1995/09/28  14:22:56  adam
 * Sort uses smaller temporary files.
 *
 * Revision 1.11  1995/09/28  12:10:31  adam
 * Bug fixes. Field prefix used in queries.
 *
 * Revision 1.10  1995/09/28  09:19:41  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.9  1995/09/27  12:22:28  adam
 * More work on extract in record control.
 * Field name is not in isam keys but in prefix in dictionary words.
 *
 * Revision 1.8  1995/09/14  07:48:22  adam
 * Record control management.
 *
 * Revision 1.7  1995/09/11  13:09:32  adam
 * More work on relevance feedback.
 *
 * Revision 1.6  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.5  1995/09/06  16:11:16  adam
 * Option: only one word key per file.
 *
 * Revision 1.4  1995/09/05  15:28:39  adam
 * More work on search engine.
 *
 * Revision 1.3  1995/09/04  12:33:41  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.2  1995/09/04  09:10:34  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 * Revision 1.1  1995/09/01  14:06:35  adam
 * Split of work into more files.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include <alexutil.h>
#include <recctrl.h>
#include "index.h"

#include "recindex.h"

static Dict matchDict;

static Records records = NULL;

static char **key_buf;
static size_t ptr_top;
static size_t ptr_i;
static size_t key_buf_used;
static int key_file_no;

static int records_inserted = 0;
static int records_updated = 0;
static int records_deleted = 0;

void key_open (int mem)
{
    if (mem < 50000)
        mem = 50000;
    key_buf = xmalloc (mem);
    ptr_top = mem/sizeof(char*);
    ptr_i = 0;

    key_buf_used = 0;
    key_file_no = 0;

    if (!(matchDict = dict_open (GMATCH_DICT, 50, 1)))
    {
        logf (LOG_FATAL, "dict_open fail of %s", GMATCH_DICT);
        exit (1);
    }
    assert (!records);
    records = rec_open (1);
}

struct encode_info {
    int  sysno;
    int  seqno;
    char buf[512];
};

void encode_key_init (struct encode_info *i)
{
    i->sysno = 0;
    i->seqno = 0;
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
    bp = encode_key_int (key.seqno - i->seqno, bp);
    i->seqno = key.seqno;
    if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "fwrite");
        exit (1);
    }
}

void key_flush (void)
{
    FILE *outf;
    char out_fname[200];
    char *prevcp, *cp;
    struct encode_info encode_info;
    
    if (ptr_i <= 0)
        return;

    key_file_no++;
    logf (LOG_LOG, "sorting section %d", key_file_no);
    qsort (key_buf + ptr_top-ptr_i, ptr_i, sizeof(char*), key_qsort_compare);
    sprintf (out_fname, TEMP_FNAME, key_file_no);

    if (!(outf = fopen (out_fname, "w")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen (4) %s", out_fname);
        exit (1);
    }
    logf (LOG_LOG, "writing section %d", key_file_no);
    prevcp = cp = key_buf[ptr_top-ptr_i];
    
    encode_key_init (&encode_info);
    encode_key_write (cp, &encode_info, outf);
    while (--ptr_i > 0)
    {
        cp = key_buf[ptr_top-ptr_i];
        if (strcmp (cp, prevcp))
        {
            encode_key_init (&encode_info);
            encode_key_write (cp, &encode_info, outf);
            prevcp = cp;
        }
        else
            encode_key_write (cp + strlen(cp), &encode_info, outf);
    }
    if (fclose (outf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fclose %s", out_fname);
        exit (1);
    }
    logf (LOG_LOG, "finished section %d", key_file_no);
    ptr_i = 0;
    key_buf_used = 0;
}

int key_close (void)
{
    key_flush ();
    xfree (key_buf);
    rec_close (&records);
    dict_close (matchDict);

    logf (LOG_LOG, "Records inserted %6d", records_inserted);
    logf (LOG_LOG, "Records updated  %6d", records_updated);
    logf (LOG_LOG, "Records deleted  %6d", records_deleted);
    return key_file_no;
}

static void wordInit (RecWord *p)
{
    p->attrSet = 1;
    p->attrUse = 1016;
    p->which = Word_String;
}

struct recKeys {
    int buf_used;
    int buf_max;
    char *buf;
    char prevAttrSet;
    short prevAttrUse;
} reckeys;

static void addRecordKey (const RecWord *p)
{
    char *dst;
    char attrSet;
    short attrUse;
    size_t i;
    int lead = 0;

    if (reckeys.buf_used+1024 > reckeys.buf_max)
    {
        char *b;

        b = xmalloc (reckeys.buf_max += 128000);
        if (reckeys.buf_used > 0)
            memcpy (b, reckeys.buf, reckeys.buf_used);
        xfree (reckeys.buf);
        reckeys.buf = b;
    }
    dst = reckeys.buf + reckeys.buf_used;

    attrSet = p->attrSet;
    if (reckeys.buf_used > 0 && reckeys.prevAttrSet == attrSet)
        lead |= 1;
    else
        reckeys.prevAttrSet = attrSet;
    attrUse = p->attrUse;
    if (reckeys.buf_used > 0 && reckeys.prevAttrUse == attrUse)
        lead |= 2;
    else
        reckeys.prevAttrUse = attrUse;

    switch (p->which)
    {
    case Word_String:
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
        for (i = 0; p->u.string[i]; i++)
            *dst++ = p->u.string[i];
        *dst++ = '\0';

        memcpy (dst, &p->seqno, sizeof(p->seqno));
        dst += sizeof(p->seqno);

        break;
    default:
        return;
    }
    reckeys.buf_used = dst - reckeys.buf;
}

static void flushRecordKeys (SYSNO sysno, int cmd, struct recKeys *reckeys, 
                             const char *databaseName)
{
    char attrSet = -1;
    short attrUse = -1;
    int off = 0;
    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        struct it_key key;
        int lead;
    
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
        if (key_buf_used + 1024 > (ptr_top-ptr_i)*sizeof(char*))
            key_flush ();
        ++ptr_i;
        key_buf[ptr_top-ptr_i] = (char*)key_buf + key_buf_used;
        key_buf_used += index_word_prefix ((char*)key_buf + key_buf_used,
                                           attrSet, attrUse, databaseName);
        while (*src)
            ((char*)key_buf) [key_buf_used++] = index_char_cvt (*src++);
        src++;
        ((char*)key_buf) [key_buf_used++] = '\0';
        
        ((char*) key_buf)[key_buf_used++] = cmd;

        memcpy (&key.seqno, src, sizeof(key.seqno));
        src += sizeof(key.seqno);
        key.sysno = sysno;
        memcpy ((char*)key_buf + key_buf_used, &key, sizeof(key));
        key_buf_used += sizeof(key);
        off = src - reckeys->buf;
    }
    assert (off == reckeys->buf_used);
}

static const char **searchRecordKey (struct recKeys *reckeys,
                               int attrSetS, int attrUseS)
{
    static const char *ws[32];
    int off = 0;
    int startSeq = -1;
    int i;

    for (i = 0; i<32; i++)
        ws[i] = NULL;
    
    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        char attrSet;
        short attrUse;
        int seqno;
        const char *wstart;
        
        memcpy (&attrSet, src, sizeof(attrSet));
        src += sizeof(attrSet);

        memcpy (&attrUse, src, sizeof(attrUse));
        src += sizeof(attrUse);

        wstart = src;
        while (*src++)
            ;

        memcpy (&seqno, src, sizeof(seqno));
        src += sizeof(seqno);

#if 0
        logf (LOG_LOG, "(%d,%d) %d %s", attrSet, attrUse, seqno, wstart);
#endif
        if (attrUseS == attrUse && attrSetS == attrSet)
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

static void addRecordKeyAny (const RecWord *p)
{
    if (p->attrSet != 1 || p->attrUse != 1016)
    {
        RecWord w;

        memcpy (&w, p, sizeof(w));
        w.attrSet = 1;
        w.attrUse = 1016;
        addRecordKey (&w);
    }
    if (p->attrSet != -1)
        addRecordKey (p);
}

#define FILE_READ_BUFSIZE 4096
struct file_read_info {
    int file_noread;
    int fd;
#if FILE_READ_BUFSIZE
    char *file_buf;
    int file_offset;
    int file_bufsize;
#endif
};

static struct file_read_info *file_read_start (int fd)
{
    struct file_read_info *fi = xmalloc (sizeof(*fi));

    fi->fd = fd;
    fi->file_noread = 0;
#if FILE_READ_BUFSIZE
    fi->file_offset = 0;
    fi->file_buf = xmalloc (FILE_READ_BUFSIZE);
    fi->file_bufsize = read (fd, fi->file_buf, FILE_READ_BUFSIZE);
#endif
    return fi;
}

static void file_read_stop (struct file_read_info *fi)
{
    assert (fi);
#if FILE_READ_BUFSIZE
    xfree (fi->file_buf);
    fi->file_buf = NULL;
#endif
    xfree (fi);
}

static int file_read (void *handle, char *buf, size_t count)
{
    struct file_read_info *p = handle;
    int fd = p->fd;
#if FILE_READ_BUFSIZE
    int l = p->file_bufsize - p->file_offset;

    if (count > l)
    {
        int r;
        if (l > 0)
            memcpy (buf, p->file_buf + p->file_offset, l);
        count = count-l;
        if (count > FILE_READ_BUFSIZE)
        {
            if ((r = read (fd, buf + l, count)) == -1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "read");
                exit (1);
            }
            p->file_bufsize = 0;
            p->file_offset = 0;
            p->file_noread += l+r;
            return l+r;
        }
        p->file_bufsize = r = read (fd, p->file_buf, FILE_READ_BUFSIZE);
        if (r == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "read");
            exit (1);
        }
        else if (r <= count)
        {
            p->file_offset = r;
            memcpy (buf + l, p->file_buf, r);
            p->file_noread += l+r;
            return l+r;
        }
        else
        {
            p->file_offset = count;
            memcpy (buf + l, p->file_buf, count - l);
            p->file_noread += count;
            return count;
        }
    }
    memcpy (buf, p->file_buf + p->file_offset, count);
    p->file_offset += count;
    p->file_noread += count;
    return count;
#else
    int r;
    r = read (fd, buf, count);
    if (r > 0)
        p->file_noread += r;
    return r;
#endif
}

static int atois (const char **s)
{
    int val = 0, c;
    while ( (c=**s) >= '0' && c <= '9')
    {
        val = val*10 + c - '0';
        ++(*s);
    }
    return val;
}

static char *fileMatchStr (struct recKeys *reckeys, struct recordGroup *rGroup,
                           const char *fname,
                           const char *spec)
{
    static char dstBuf[2048];
    char *dst = dstBuf;
    const char *s = spec;
    static const char **w;
    int i;

    while (1)
    {
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        if (*s == '(')
        {
            char matchFlag[32];
            int attrSet, attrUse;
            int first = 1;

            s++;
            attrSet = atois (&s);
            if (*s != ',')
            {
                logf (LOG_WARN, "Missing , in match criteria %s in group %s",
                      spec, rGroup->groupName ? rGroup->groupName : "none");
                return NULL;
            }
            s++;
            attrUse = atois (&s);
            w = searchRecordKey (reckeys, attrSet, attrUse);
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
                logf (LOG_WARN, "Record in file %s didn't contain match"
                      " fields in (%d,%d)", fname, attrSet, attrUse);
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
    return dstBuf;
}

static int recordExtract (SYSNO *sysno, const char *fname,
                          struct recordGroup *rGroup, int deleteFlag,
                          struct file_read_info *fi, RecType recType,
                          char *subType)
{
    struct recExtractCtrl extractCtrl;
    int r;
    char *matchStr;
    SYSNO sysnotmp;
    Record rec;

    if (fi->fd != -1)
    {
        extractCtrl.fh = fi;
        /* extract keys */
        extractCtrl.subType = subType;
        extractCtrl.init = wordInit;
        extractCtrl.add = addRecordKeyAny;

        reckeys.buf_used = 0;
        reckeys.prevAttrUse = -1;
        reckeys.prevAttrSet = -1;
        extractCtrl.readf = file_read;
        r = (*recType->extract)(&extractCtrl);
  
        if (r)      
        {
            logf (LOG_WARN, "Couldn't extract file %s, code %d", fname, r);
            return 0;
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
        
            matchStr = fileMatchStr (&reckeys, rGroup, fname, 
                                     rGroup->recordId);
            if (matchStr)
            {
                rinfo = dict_lookup (matchDict, matchStr);
                if (rinfo)
                    memcpy (sysno, rinfo+1, sizeof(*sysno));
            }
            else
            {
                logf (LOG_WARN, "Record not inserted");
                return 0;
            }
        }
    }

    /* new record ? */
    if (! *sysno)
    {
        if (deleteFlag)
        {
            logf (LOG_LOG, "? %s", fname);
            return 1;
        }
        logf (LOG_LOG, "add %s %s", rGroup->recordType, fname);
        rec = rec_new (records);
        *sysno = rec->sysno;

        if (matchStr)
        {
            dict_insert (matchDict, matchStr, sizeof(*sysno), sysno);
        }
        flushRecordKeys (*sysno, 1, &reckeys, rGroup->databaseName);

        records_inserted++;
    }
    else
    {
        struct recKeys delkeys;

        rec = rec_get (records, *sysno);
        assert (rec);
        delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];
        flushRecordKeys (*sysno, 0, &delkeys, rec->info[recInfo_databaseName]);
        if (deleteFlag)
        {
            if (!delkeys.buf_used)
            {
                logf (LOG_WARN, "cannot delete %s: storeKeys false",
                      fname);
            }
            else
            {
                logf (LOG_LOG, "delete %s %s", rGroup->recordType, fname);
                records_deleted++;
                if (matchStr)
                    dict_delete (matchDict, matchStr);
                rec_del (records, &rec);
            }
            return 1;
        }
        else
        {
            if (!delkeys.buf_used)
            {
                logf (LOG_WARN, "cannot update %s: storeKeys false",
                      fname);
            }
            else
            {
                logf (LOG_LOG, "update %s %s", rGroup->recordType,
                      fname);
                flushRecordKeys (*sysno, 1, &reckeys, rGroup->databaseName); 
                records_updated++;
            }
        }
    }
    xfree (rec->info[recInfo_fileType]);
    rec->info[recInfo_fileType] =
        rec_strdup (rGroup->recordType, &rec->size[recInfo_fileType]);

    xfree (rec->info[recInfo_filename]);
    rec->info[recInfo_filename] =
        rec_strdup (fname, &rec->size[recInfo_filename]);

    xfree (rec->info[recInfo_delKeys]);
    if (reckeys.buf_used > 0 && rGroup->flagStoreKeys == 1)
    {
#if 1
        rec->size[recInfo_delKeys] = reckeys.buf_used;
        rec->info[recInfo_delKeys] = reckeys.buf;
        reckeys.buf = NULL;
        reckeys.buf_max = 0;
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

    xfree (rec->info[recInfo_storeData]);
    if (rGroup->flagStoreData == 1)
    {
        rec->size[recInfo_storeData] = fi->file_noread;
        rec->info[recInfo_storeData] = xmalloc (fi->file_noread);
#if FILE_READ_BUFSIZE
        if (fi->file_noread < FILE_READ_BUFSIZE)
	    memcpy (rec->info[recInfo_storeData], fi->file_buf,
                    fi->file_noread);
        else
#endif
        {
            if (lseek (fi->fd, 0L, SEEK_SET) < 0)
            {
                logf (LOG_ERRNO|LOG_FATAL, "seek to 0 in %s", fname);
                exit (1);
            }
            if (read (fi->fd, rec->info[recInfo_storeData], fi->file_noread) 
                < fi->file_noread)
            {
                logf (LOG_ERRNO|LOG_FATAL, "read %d bytes of %s",
                      fi->file_noread, fname);
                exit (1);
            }
        }
    }
    else
    {
        rec->info[recInfo_storeData] = NULL;
        rec->size[recInfo_storeData] = 0;
    }
    xfree (rec->info[recInfo_databaseName]);
    rec->info[recInfo_databaseName] =
        rec_strdup (rGroup->databaseName, &rec->size[recInfo_databaseName]); 

    rec_put (records, &rec);
    return 1;
}

int fileExtract (SYSNO *sysno, const char *fname, 
                 const struct recordGroup *rGroupP, int deleteFlag)
{
    int i, fd;
    char gprefix[128];
    char ext[128];
    char ext_res[128];
    char subType[128];
    RecType recType;
    struct recordGroup rGroupM;
    struct recordGroup *rGroup = &rGroupM;
    struct file_read_info *fi;

    memcpy (rGroup, rGroupP, sizeof(*rGroupP));
   
    if (!rGroup->groupName || !*rGroup->groupName)
        *gprefix = '\0';
    else
        sprintf (gprefix, "%s.", rGroup->groupName);

    logf (LOG_DEBUG, "fileExtract %s", fname);

    /* determine file extension */
    for (i = strlen(fname); --i >= 0; )
        if (fname[i] == '/')
        {
            strcpy (ext, "");
            break;
        }
        else if (fname[i] == '.')
        {
            strcpy (ext, fname+i+1);
            break;
        }
    /* determine file type - depending on extension */
    if (!rGroup->recordType)
    {
        sprintf (ext_res, "%srecordType.%s", gprefix, ext);
        if (!(rGroup->recordType = res_get (common_resource, ext_res)))
        {
            sprintf (ext_res, "%srecordType", gprefix);
            if (!(rGroup->recordType = res_get (common_resource, ext_res)))
            {
                logf (LOG_LOG, "? %s", fname);
                return 0;
            }
        }
    }
    if (!rGroup->recordType)
    {
        logf (LOG_LOG, "? record %s", fname);
        return 0;
    }
    if (!(recType = recType_byName (rGroup->recordType, subType)))
    {
        logf (LOG_WARN, "No such record type: %s", rGroup->recordType);
        return 0;
    }

    /* determine match criteria */
    if (!rGroup->recordId)
    {
        sprintf (ext_res, "%srecordId.%s", gprefix, ext);
        rGroup->recordId = res_get (common_resource, ext_res);
    }

    /* determine database name */
    if (!rGroup->databaseName)
    {
        sprintf (ext_res, "%sdatabase.%s", gprefix, ext);
        if (!(rGroup->databaseName = res_get (common_resource, ext_res)))
        {
            sprintf (ext_res, "%sdatabase", gprefix);
            rGroup->databaseName = res_get (common_resource, ext_res);
        }
    }
    if (!rGroup->databaseName)
        rGroup->databaseName = "Default";

    if (rGroup->flagStoreData == -1)
    {
        const char *sval;
        sprintf (ext_res, "%sstoreData.%s", gprefix, ext);
        if (!(sval = res_get (common_resource, ext_res)))
        {
            sprintf (ext_res, "%sstoreData", gprefix);
            sval = res_get (common_resource, ext_res);
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
        if (!(sval = res_get (common_resource, ext_res)))
        {
            sprintf (ext_res, "%sstoreKeys", gprefix);
            sval = res_get (common_resource, ext_res);
        }
        if (sval)
            rGroup->flagStoreKeys = atoi (sval);
    }
    if (rGroup->flagStoreKeys == -1)
        rGroup->flagStoreKeys = 0;

    if (sysno && deleteFlag)
        fd = -1;
    else
    {
        if ((fd = open (fname, O_RDONLY)) == -1)
        {
            logf (LOG_WARN|LOG_ERRNO, "open %s", fname);
            return 0;
        }
    }
    fi = file_read_start (fd);
    recordExtract (sysno, fname, rGroup, deleteFlag, fi, recType, subType);
    file_read_stop (fi);
    if (fd != -1)
        close (fd);
    return 1;
}

