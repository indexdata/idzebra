/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: extract.c,v $
 * Revision 1.28  1995-11-21 09:20:30  adam
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

static Dict file_idx;

static Records records = NULL;

static char **key_buf;
static size_t ptr_top;
static size_t ptr_i;
static size_t key_buf_used;
static int key_file_no;

void key_open (int mem)
{
    if (mem < 50000)
        mem = 50000;
    key_buf = xmalloc (mem);
    ptr_top = mem/sizeof(char*);
    ptr_i = 0;

    key_buf_used = 0;
    key_file_no = 0;

    if (!(file_idx = dict_open (FNAME_FILE_DICT, 40, 1)))
    {
        logf (LOG_FATAL, "dict_open fail of %s", "fileidx");
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
    dict_close (file_idx);

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
} reckeys;

static void addRecordKey (const RecWord *p)
{
    char *dst;
    char attrSet;
    short attrUse;
    size_t i;

    if (reckeys.buf_used+1024 > reckeys.buf_max)
    {
        char *b;

        b = malloc (reckeys.buf_max += 65000);
        if (reckeys.buf_used > 0)
            memcpy (b, reckeys.buf, reckeys.buf_used);
        free (reckeys.buf);
        reckeys.buf = b;
    }
    dst = reckeys.buf + reckeys.buf_used;
    switch (p->which)
    {
    case Word_String:
        attrSet = p->attrSet;
        memcpy (dst, &attrSet, sizeof(attrSet));
        dst += sizeof(attrSet);

        attrUse = p->attrUse;
        memcpy (dst, &attrUse, sizeof(attrUse));
        dst += sizeof(attrUse);
        
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
    int off = 0;
    while (off < reckeys->buf_used)
    {
        const char *src = reckeys->buf + off;
        char attrSet;
        short attrUse;
        struct it_key key;
        
        memcpy (&attrSet, src, sizeof(attrSet));
        src += sizeof(attrSet);

        memcpy (&attrUse, src, sizeof(attrUse));
        src += sizeof(attrUse);

        if (key_buf_used + 1024 > (ptr_top-ptr_i)*sizeof(char*))
            key_flush ();
        ++ptr_i;
        key_buf[ptr_top-ptr_i] = (char*)key_buf + key_buf_used;
        key_buf_used += index_word_prefix ((char*)key_buf + key_buf_used,
                                           attrSet, attrUse, databaseName);
        while (*src)
            ((char*)key_buf) [key_buf_used++] = index_char_cvt (*src++);
        ((char*)key_buf) [key_buf_used++] = '\0';
        
        ((char*) key_buf)[key_buf_used++] = cmd;

        memcpy (&key.seqno, src, sizeof(key.seqno));
        src += sizeof(key.seqno);
        key.sysno = sysno;
        memcpy ((char*)key_buf + key_buf_used, &key, sizeof(key));
        key_buf_used += sizeof(key);
        off = src - reckeys->buf;
    }
    assert (off = reckeys->buf_used);
}

#if 0
static int key_cmd;
static int key_sysno;
static const char *key_databaseName;
static int key_del_max;
static int key_del_used;
static char *key_del_buf;

static void wordAdd (const RecWord *p)
{
    struct it_key key;
    size_t i;

    if (key_buf_used + 1024 > (ptr_top-ptr_i)*sizeof(char*))
        key_flush ();
    ++ptr_i;
    key_buf[ptr_top-ptr_i] = (char*)key_buf + key_buf_used;
    key_buf_used += index_word_prefix ((char*)key_buf + key_buf_used,
                                p->attrSet, p->attrUse,
                                key_databaseName);
    switch (p->which)
    {
    case Word_String:
        for (i = 0; p->u.string[i]; i++)
            ((char*)key_buf) [key_buf_used++] =
                index_char_cvt (p->u.string[i]);
        ((char*)key_buf) [key_buf_used++] = '\0';
        break;
    default:
        return ;
    }
    ((char*) key_buf)[key_buf_used++] = ((key_cmd == 'a') ? 1 : 0);
    key.sysno = key_sysno;
    key.seqno = p->seqno;
    memcpy ((char*)key_buf + key_buf_used, &key, sizeof(key));
    key_buf_used += sizeof(key);

    if (key_cmd == 'a' && key_del_used >= 0)
    {
        char attrSet;
        short attrUse;
        if (key_del_used + 1024 > key_del_max)
        {
            char *kbn;
            
            if (!(kbn = malloc (key_del_max += 64000)))
            {
                logf (LOG_FATAL, "malloc");
                exit (1);
            }
            if (key_del_buf)
                memcpy (kbn, key_del_buf, key_del_used);
            free (key_del_buf);
            key_del_buf = kbn;
        }
        switch (p->which)
        {
        case Word_String:
            for (i = 0; p->u.string[i]; i++)
                ((char*)key_del_buf) [key_del_used++] = p->u.string[i];
            ((char*)key_del_buf) [key_del_used++] = '\0';
            break;
        default:
            return ;
        }
        attrSet = p->attrSet;
        memcpy (key_del_buf + key_del_used, &attrSet, sizeof(attrSet));
        key_del_used += sizeof(attrSet);

        attrUse = p->attrUse;
        memcpy (key_del_buf + key_del_used, &attrUse, sizeof(attrUse));
        key_del_used += sizeof(attrUse);

        memcpy (key_del_buf + key_del_used, &p->seqno, sizeof(p->seqno));
        key_del_used += sizeof(p->seqno);
    }
}

#endif

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
    addRecordKey (p);
}

static char *file_buf;
static int file_offset;
static int file_bufsize;

static void file_read_start (int fd)
{
    file_offset = 0;
    file_buf = xmalloc (4096);
    file_bufsize = read (fd, file_buf, 4096);
}

static void file_read_stop (int fd)
{
    xfree (file_buf);
}

static int file_read (int fd, char *buf, size_t count)
{
    int l = file_bufsize - file_offset;

    if (count > l)
    {
        int r;
        if (l > 0)
            memcpy (buf, file_buf + file_offset, l);
        count = count-l;
        if (count > file_bufsize)
        {
            if ((r = read (fd, buf + l, count)) == -1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "read");
                exit (1);
            }
            file_bufsize = 0;
            file_offset = 0;
            return r;
        }
        file_bufsize = r = read (fd, file_buf, 4096);
        if (r == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "read");
            exit (1);
        }
        else if (r <= count)
        {
            file_offset = r;
            memcpy (buf + l, file_buf, r);
            return l + r;
        }
        else
        {
            file_offset = count;
            memcpy (buf + l, file_buf, count - l);
            return count;
        }
    }
    memcpy (buf, file_buf + file_offset, count);
    file_offset += count;
    return count;
}

int fileExtract (SYSNO *sysno, const char *fname, const char *databaseName,
                 int deleteFlag)
{
    int i, r;
    char ext[128];
    char ext_res[128];
    const char *file_type;
    struct recExtractCtrl extractCtrl;
    RecType rt;
    Record rec;

    logf (LOG_DEBUG, "fileExtractAdd %s", fname);

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
    sprintf (ext_res, "fileExtension.%s", ext);
    if (!(file_type = res_get (common_resource, ext_res)))
        return 0;
    if (!(rt = recType_byName (file_type)))
        return 0;

    if ((extractCtrl.fd = open (fname, O_RDONLY)) == -1)
    {
        logf (LOG_WARN|LOG_ERRNO, "open %s", fname);
        return 0;
    }

    extractCtrl.subType = "";
    extractCtrl.init = wordInit;
    extractCtrl.add = addRecordKeyAny;

    reckeys.buf_used = 0;
    file_read_start (extractCtrl.fd);
    extractCtrl.readf = file_read;
    r = (*rt->extract)(&extractCtrl);
    file_read_stop (extractCtrl.fd);
    close (extractCtrl.fd);
  
    if (r)      
    {
        logf (LOG_WARN, "Couldn't extract file %s, code %d", fname, r);
        return 0;
    }
    if (! *sysno)  /* match criteria */
    {
        logf (LOG_LOG, "add record %s", fname);
        rec = rec_new (records);
        *sysno = rec->sysno;

        flushRecordKeys (*sysno, 1, &reckeys, databaseName);
    }
    else
    {
        struct recKeys delkeys;
        
        rec = rec_get (records, *sysno);

        delkeys.buf_used = rec->size[2];
	delkeys.buf = rec->info[2];
        flushRecordKeys (*sysno, 0, &delkeys, rec->info[3]);
        flushRecordKeys (*sysno, 1, &reckeys, databaseName); 
    }
    free (rec->info[0]);
    rec->info[0] = rec_strdup (file_type, &rec->size[0]);

    free (rec->info[1]);
    rec->info[1] = rec_strdup (fname, &rec->size[1]);

    free (rec->info[2]);
    if (reckeys.buf_used > 0)
    {
        rec->info[2] = malloc (reckeys.buf_used);
        rec->size[2] = reckeys.buf_used;
        memcpy (rec->info[2], reckeys.buf, rec->size[2]);
    }
    else
    {
        rec->info[2] = NULL;
        rec->size[2] = 0;
    }
    free (rec->info[3]);
    rec->info[3] = rec_strdup (databaseName, &rec->size[3]); 

    rec_put (records, &rec);
    return 1;
}
