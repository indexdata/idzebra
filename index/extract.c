/*
 * Copyright (C) 1994-1998, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: extract.c,v $
 * Revision 1.87  1998-10-15 13:10:33  adam
 * Fixed bug in Zebra that caused it to stop indexing when empty
 * record was read.
 *
 * Revision 1.86  1998/10/13 20:33:53  adam
 * Fixed one log message and change use ordinal to be an unsigned char.
 *
 * Revision 1.85  1998/09/22 10:03:41  adam
 * Changed result sets to be persistent in the sense that they can
 * be re-searched if needed.
 * Fixed memory leak in rsm_or.
 *
 * Revision 1.84  1998/06/11 15:42:22  adam
 * Changed the way use attributes are specified in the recordId
 * specification.
 *
 * Revision 1.83  1998/06/08 14:43:10  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.82  1998/05/20 10:12:15  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.81  1998/03/11 11:19:04  adam
 * Changed the way sequence numbers are generated.
 *
 * Revision 1.80  1998/03/05 08:45:11  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.79  1998/02/17 10:32:52  adam
 * Fixed bug: binary files weren't opened with flag b on NT.
 *
 * Revision 1.78  1998/02/10 12:03:05  adam
 * Implemented Sort.
 *
 * Revision 1.77  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.76  1997/10/27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.75  1997/09/17 12:19:12  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.74  1997/09/09 13:38:06  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.73  1997/09/04 13:57:20  adam
 * New file extract/retrieve method tellf (added).
 * Added O_BINARY for open calls.
 *
 * Revision 1.72  1997/07/15 16:32:29  adam
 * Bug fix: Match handler didn't terminate the resulting string!
 *
 * Revision 1.71  1997/07/15 16:28:41  adam
 * Bug fix: storeData didn't work with files with multiple records.
 * Bug fix: fixed memory management with records; not really well
 *  thought through.
 *
 * Revision 1.70  1997/07/01 13:00:42  adam
 * Bug fix in routine searchRecordKey: uninitialized variables.
 *
 * Revision 1.69  1997/04/29 09:26:03  adam
 * Bug fix: generic recordId handling didn't work for compressed internal
 * keys.
 *
 * Revision 1.68  1997/02/12 20:39:45  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.67  1996/11/15 15:02:14  adam
 * Minor changes regarding logging.
 *
 * Revision 1.66  1996/11/14  09:52:21  adam
 * Strings in record keys bound by IT_MAX_WORD.
 *
 * Revision 1.65  1996/11/14  08:57:56  adam
 * Reduction of storeKeys area.
 *
 * Revision 1.64  1996/11/08 11:10:16  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.63  1996/10/29 14:09:39  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.62  1996/10/11 10:57:01  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 * Several files have been moved to the recctrl sub directory.
 *
 * Revision 1.61  1996/06/06 12:08:37  quinn
 * Added showRecord function
 *
 * Revision 1.60  1996/06/04  10:18:12  adam
 * Search/scan uses character mapping module.
 *
 * Revision 1.59  1996/05/14  15:47:07  adam
 * Cleanup of various buffer size entities.
 *
 * Revision 1.58  1996/05/14  06:16:38  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.57  1996/05/13 14:23:04  adam
 * Work on compaction of set/use bytes in dictionary.
 *
 * Revision 1.56  1996/05/09  09:54:42  adam
 * Server supports maps from one logical attributes to a list of physical
 * attributes.
 * The extraction process doesn't make space consuming 'any' keys.
 *
 * Revision 1.55  1996/05/09  07:28:55  quinn
 * Work towards phrases and multiple registers
 *
 * Revision 1.54  1996/05/01  13:46:35  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.53  1996/04/26  12:09:43  adam
 * Added a few comments.
 *
 * Revision 1.52  1996/04/25  13:27:57  adam
 * Function recordExtract modified so that files with no keys (possibly empty)
 * are ignored.
 *
 * Revision 1.51  1996/03/19  11:08:42  adam
 * Bug fix: Log preamble wasn't always turned off after recordExtract.
 *
 * Revision 1.50  1996/02/12  18:45:36  adam
 * New fileVerboseFlag in record group control.
 *
 * Revision 1.49  1996/02/05  12:29:57  adam
 * Logging reduced a bit.
 * The remaining running time is estimated during register merge.
 *
 * Revision 1.48  1996/02/01  20:53:26  adam
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
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include <recctrl.h>
#include <charmap.h>
#include <sortidx.h>
#include "index.h"

#include "zinfo.h"

static Dict matchDict;

static Records records = NULL;
static SortIdx sortIdx = NULL;

static char **key_buf;
static size_t ptr_top;
static size_t ptr_i;
static size_t key_buf_used;
static int key_file_no;

static int records_inserted = 0;
static int records_updated = 0;
static int records_deleted = 0;
static int records_processed = 0;

static ZebraExplainInfo zti = NULL;

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

static int explain_extract (void *handle, Record drec, data1_node *n);

int key_open (struct recordGroup *rGroup, int mem)
{
    BFiles bfs = rGroup->bfs;
    int rw = rGroup->flagRw;
    data1_handle dh = rGroup->dh;
    if (!mem)
        mem = atoi(res_get_def (common_resource, "memMax", "4"))*1024*1024;
    if (mem < 50000)
        mem = 50000;
    key_buf = xmalloc (mem);
    ptr_top = mem/sizeof(char*);
    ptr_i = 0;

    key_buf_used = 0;
    key_file_no = 0;

    if (!(matchDict = dict_open (bfs, GMATCH_DICT, 50, rw)))
    {
        logf (LOG_FATAL, "dict_open fail of %s", GMATCH_DICT);
	return -1;
    }
    assert (!records);
    records = rec_open (bfs, rw);
    if (!records)
    {
	dict_close (matchDict);
	return -1;
    }
    zti = zebraExplain_open (records, dh, common_resource,
			     rw, rGroup, explain_extract);
    if (!zti)
    {
	rec_close (&records);
	dict_close (matchDict);
	return -1;	
    }
    sortIdx = sortIdx_open (bfs, 1);
    return 0;
}

struct encode_info {
    int  sysno;
    int  seqno;
    char buf[768];
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

#define SORT_EXTRA 0

#if SORT_EXTRA
static int key_y_len;

static int key_y_compare (const void *p1, const void *p2)
{
    int r;

    if ((r = key_compare (*(char**) p1 + key_y_len + 1,
                          *(char**) p2 + key_y_len + 1)))
         return r;
    return *(*(char**) p1 + key_y_len) - *(*(char**) p2 + key_y_len);
}

static int key_x_compare (const void *p1, const void *p2)
{
    return strcmp (*(char**) p1, *(char**) p2);
}
#endif

void key_flush (void)
{
    FILE *outf;
    char out_fname[200];
    char *prevcp, *cp;
    struct encode_info encode_info;
#if SORT_EXTRA
    int i;
#endif
    
    if (ptr_i <= 0)
        return;

    key_file_no++;
    logf (LOG_LOG, "sorting section %d", key_file_no);
#if !SORT_EXTRA
    qsort (key_buf + ptr_top-ptr_i, ptr_i, sizeof(char*), key_qsort_compare);
    getFnameTmp (out_fname, key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", out_fname);
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
#else
    qsort (key_buf + ptr_top-ptr_i, ptr_i, sizeof(char*), key_x_compare);
    getFnameTmp (out_fname, key_file_no);

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
    logf (LOG_LOG, "finished section %d", key_file_no);
    ptr_i = 0;
    key_buf_used = 0;
}

int key_close (struct recordGroup *rGroup)
{
    int rw = rGroup->flagRw;
    if (rw)
	zebraExplain_runNumberIncrement (zti, 1);
    zebraExplain_close (zti, rw, 0);
    key_flush ();
    xfree (key_buf);
    rec_close (&records);
    dict_close (matchDict);
    sortIdx_close (sortIdx);

    logRecord (1);
    return key_file_no;
}

static void wordInit (struct recExtractCtrl *p, RecWord *w)
{
    w->zebra_maps = p->zebra_maps;
    w->seqnos = p->seqno;
    w->attrSet = VAL_BIB1;
    w->attrUse = 1016;
    w->reg_type = 'w';
}

static struct sortKey {
    char *string;
    int length;
    int attrSet;
    int attrUse;
    struct sortKey *next;
} *sortKeys = NULL;

static struct recKeys {
    int buf_used;
    int buf_max;
    char *buf;
    char prevAttrSet;
    short prevAttrUse;
    int prevSeqNo;
} reckeys;

static void addIndexString (RecWord *p, const char *string, int length)
{
    char *dst;
    unsigned char attrSet;
    unsigned short attrUse;
    int lead = 0;
    int diff = 0;
    int *pseqno = &p->seqnos[p->reg_type];

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
#if 1
    diff = 1 + *pseqno - reckeys.prevSeqNo;
    if (diff >= 1 && diff <= 15)
        lead |= (diff << 2);
    else
        diff = 0;
#endif
    reckeys.prevSeqNo = *pseqno;
    
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
    reckeys.buf_used = dst - reckeys.buf;
    (*pseqno)++;
}

static void addSortString (RecWord *p, const char *string, int length)
{
    struct sortKey *sk;

    for (sk = sortKeys; sk; sk = sk->next)
	if (sk->attrSet == p->attrSet && sk->attrUse == p->attrUse)
	    return;

    sk = xmalloc (sizeof(*sk));
    sk->next = sortKeys;
    sortKeys = sk;

    sk->string = xmalloc (length);
    sk->length = length;
    memcpy (sk->string, string, length);

    sk->attrSet = p->attrSet;
    sk->attrUse = p->attrUse;
}

static void addString (RecWord *p, const char *string, int length)
{
    assert (length > 0);
    if (zebra_maps_is_sort (p->zebra_maps, p->reg_type))
	addSortString (p, string, length);
    else
	addIndexString (p, string, length);
}

static void addIncompleteField (RecWord *p)
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
	addString (p, buf, i);
    }
}

static void addCompleteField (RecWord *p)
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
    addString (p, buf, i);
}

static void addRecordKey (RecWord *p)
{
    if (zebra_maps_is_complete (p->zebra_maps, p->reg_type))
	addCompleteField (p);
    else
	addIncompleteField(p);
}

static void flushSortKeys (SYSNO sysno, int cmd)
{
    struct sortKey *sk = sortKeys;

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
    sortKeys = NULL;
}

static void flushRecordKeys (SYSNO sysno, int cmd, struct recKeys *reckeys)
{
    unsigned char attrSet = (unsigned char) -1;
    unsigned short attrUse = (unsigned short) -1;
    int seqno = 0;
    int off = 0;

    zebraExplain_recordCountIncrement (zti, cmd ? 1 : -1);
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
        if (key_buf_used + 1024 > (ptr_top-ptr_i)*sizeof(char*))
            key_flush ();
        ++ptr_i;
        key_buf[ptr_top-ptr_i] = (char*)key_buf + key_buf_used;

        ch = zebraExplain_lookupSU (zti, attrSet, attrUse);
        if (ch < 0)
            ch = zebraExplain_addSU (zti, attrSet, attrUse);
        assert (ch > 0);
	key_buf_used += key_SU_code (ch, ((char*)key_buf) + key_buf_used);

        while (*src)
            ((char*)key_buf) [key_buf_used++] = *src++;
        src++;
        ((char*)key_buf) [key_buf_used++] = '\0';
        ((char*) key_buf)[key_buf_used++] = cmd;

        if (lead & 60)
            seqno += ((lead>>2) & 15)-1;
        else
        {
            memcpy (&seqno, src, sizeof(seqno));
            src += sizeof(seqno);
        }
        key.seqno = seqno;
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
    short attrUse;
    char attrSet;
    int seqno = 0;

    for (i = 0; i<32; i++)
        ws[i] = NULL;
    
    while (off < reckeys->buf_used)
    {

        const char *src = reckeys->buf + off;
	const char *wstart;
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

struct file_read_info {
    off_t file_max;
    off_t file_offset;
    off_t file_moffset;
    int file_more;
    int fd;
};

static struct file_read_info *file_read_start (int fd)
{
    struct file_read_info *fi = xmalloc (sizeof(*fi));

    fi->fd = fd;
    fi->file_max = 0;
    fi->file_moffset = 0;
    return fi;
}

static void file_read_stop (struct file_read_info *fi)
{
    xfree (fi);
}

static off_t file_seek (void *handle, off_t offset)
{
    struct file_read_info *p = handle;
    p->file_offset = offset;
    return lseek (p->fd, offset, SEEK_SET);
}

static off_t file_tell (void *handle)
{
    struct file_read_info *p = handle;
    return p->file_offset;
}

static int file_read (void *handle, char *buf, size_t count)
{
    struct file_read_info *p = handle;
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

static void file_begin (void *handle)
{
    struct file_read_info *p = handle;

    p->file_offset = p->file_moffset;
    if (p->file_moffset)
        lseek (p->fd, p->file_moffset, SEEK_SET);
    p->file_more = 0;
}

static void file_end (void *handle, off_t offset)
{
    struct file_read_info *p = handle;

    assert (p->file_more == 0);
    p->file_more = 1;
    p->file_moffset = offset;
}

static char *fileMatchStr (struct recKeys *reckeys, struct recordGroup *rGroup,
                           const char *fname, const char *spec)
{
    static char dstBuf[2048];
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
	    
	    if ((attset = data1_get_attset (rGroup->dh, attset_str)))
	    {
		data1_att *att;
		attSet = attset->reference;
		att = data1_getattbyname(rGroup->dh, attset, attname_str);
		if (att)
		    attUse = att->value;
		else
		    attUse = atoi (attname_str);
	    }
            w = searchRecordKey (reckeys, attSet, attUse);
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
    struct recordLogInfo *p = info;
    FILE *outf = log_file ();

    if (level & LOG_LOG)
        return ;
    fprintf (outf, "File %s, offset %d, type %s\n",
             p->rGroup->recordType, p->recordOffset, p->fname);
    log_event_start (NULL, NULL);
}

void addSchema (struct recExtractCtrl *p, Odr_oid *oid)
{
    zebraExplain_addSchema (zti, oid);
}

static int recordExtract (SYSNO *sysno, const char *fname,
                          struct recordGroup *rGroup, int deleteFlag,
                          struct file_read_info *fi,
			  RecType recType, char *subType)
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

	reckeys.buf_used = 0;
	reckeys.prevAttrUse = -1;
	reckeys.prevAttrSet = -1;
	reckeys.prevSeqNo = 0;
	
	recordOffset = fi->file_moffset;
	extractCtrl.offset = fi->file_moffset;
	extractCtrl.readf = file_read;
	extractCtrl.seekf = file_seek;
	extractCtrl.tellf = file_tell;
	extractCtrl.endf = file_end;
	extractCtrl.fh = fi;
	extractCtrl.subType = subType;
	extractCtrl.init = wordInit;
	extractCtrl.addWord = addRecordKey;
	extractCtrl.addSchema = addSchema;
	extractCtrl.dh = rGroup->dh;
	for (i = 0; i<256; i++)
	    extractCtrl.seqno[i] = 0;
	extractCtrl.zebra_maps = rGroup->zebra_maps;
	extractCtrl.flagShowRecords = !rGroup->flagRw;

        if (!rGroup->flagRw)
            printf ("File: %s %ld\n", fname, (long) recordOffset);

        logInfo.fname = fname;
        logInfo.recordOffset = recordOffset;
        logInfo.rGroup = rGroup;
        log_event_start (recordLogPreamble, &logInfo);

        r = (*recType->extract)(&extractCtrl);

        log_event_start (NULL, NULL);

        if (r)      
        {
            /* error occured during extraction ... */
            if (rGroup->flagRw &&
		records_processed < rGroup->fileVerboseLimit)
            {
                logf (LOG_WARN, "fail %s %s %ld code = %d", rGroup->recordType,
                      fname, (long) recordOffset, r);
            }
            return 0;
        }
        if (reckeys.buf_used == 0)
        {
            /* the extraction process returned no information - the record
               is probably empty - unless flagShowRecords is in use */
            if (!rGroup->flagRw)
                return 1;
            logf (LOG_WARN, "No keys generated for file %s", fname);
            logf (LOG_WARN, " The file is probably empty");
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
	    logf (LOG_LOG, "delete %s %s %ld", rGroup->recordType,
		  fname, (long) recordOffset);
            logf (LOG_WARN, "cannot delete record above (seems new)");
            return 1;
        }
        if (records_processed < rGroup->fileVerboseLimit)
            logf (LOG_LOG, "add %s %s %ld", rGroup->recordType,
                  fname, (long) recordOffset);
        rec = rec_new (records);

        *sysno = rec->sysno;

	recordAttr = rec_init_attr (zti, rec);

        if (matchStr)
        {
            dict_insert (matchDict, matchStr, sizeof(*sysno), sysno);
        }
        flushRecordKeys (*sysno, 1, &reckeys);
	flushSortKeys (*sysno, 1);

        records_inserted++;
    }
    else
    {
        /* record already exists */
        struct recKeys delkeys;

        rec = rec_get (records, *sysno);
        assert (rec);
	
	recordAttr = rec_init_attr (zti, rec);

	if (recordAttr->runNumber == zebraExplain_runNumberIncrement (zti, 0))
	{
	    logf (LOG_LOG, "skipped %s %s %ld", rGroup->recordType,
		  fname, (long) recordOffset);
	    rec_rm (&rec);
	    logRecord (0);
	    return 1;
	}
        delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];
	flushSortKeys (*sysno, 0);
        flushRecordKeys (*sysno, 0, &delkeys);
        if (deleteFlag)
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
                if (records_processed < rGroup->fileVerboseLimit)
                    logf (LOG_LOG, "delete %s %s %ld", rGroup->recordType,
                          fname, (long) recordOffset);
                records_deleted++;
                if (matchStr)
                    dict_delete (matchDict, matchStr);
                rec_del (records, &rec);
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
                logf (LOG_LOG, "update %s %s %ld", rGroup->recordType,
                      fname, (long) recordOffset);
                logf (LOG_WARN, "cannot update file above, storeKeys false");
            }
            else
            {
                if (records_processed < rGroup->fileVerboseLimit)
                    logf (LOG_LOG, "update %s %s %ld", rGroup->recordType,
                          fname, (long) recordOffset);
                flushRecordKeys (*sysno, 1, &reckeys);
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

    /* save file size of original record */
    zebraExplain_recordBytesIncrement (zti, - recordAttr->recordSize);
    recordAttr->recordSize = fi->file_moffset - recordOffset;
    if (!recordAttr->recordSize)
	recordAttr->recordSize = fi->file_max - recordOffset;
    zebraExplain_recordBytesIncrement (zti, recordAttr->recordSize);

    /* set run-number for this record */
    recordAttr->runNumber = zebraExplain_runNumberIncrement (zti, 0);

    /* update store data */
    xfree (rec->info[recInfo_storeData]);
    if (rGroup->flagStoreData == 1)
    {
        rec->size[recInfo_storeData] = recordAttr->recordSize;
        rec->info[recInfo_storeData] = xmalloc (recordAttr->recordSize);
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
    rec_put (records, &rec);
    logRecord (0);
    return 1;
}

int fileExtract (SYSNO *sysno, const char *fname, 
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
        if (!(rGroup->recordType = res_get (common_resource, ext_res)))
        {
            sprintf (ext_res, "%srecordType", gprefix);
            rGroup->recordType = res_get (common_resource, ext_res);
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

    /* determine if explain database */
    
    sprintf (ext_res, "%sexplainDatabase", gprefix);
    rGroup->explainDatabase =
	atoi (res_get_def (common_resource, ext_res, "0"));

    /* announce database */
    if (zebraExplain_curDatabase (zti, rGroup->databaseName))
    {
        if (zebraExplain_newDatabase (zti, rGroup->databaseName,
				      rGroup->explainDatabase))
            abort ();
    }

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
        r = recordExtract (sysno, fname, rGroup, deleteFlag, fi,
                           recType, subType);
    } while (r && !sysno && fi->file_more);
    file_read_stop (fi);
    if (fd != -1)
        close (fd);
    return r;
}

static int explain_extract (void *handle, Record rec, data1_node *n)
{
    struct recordGroup *rGroup = (struct recordGroup*) handle;
    struct recExtractCtrl extractCtrl;
    int i;

    if (zebraExplain_curDatabase (zti, rec->info[recInfo_databaseName]))
    {
        if (zebraExplain_newDatabase (zti, rec->info[recInfo_databaseName], 0))
            abort ();
    }

    reckeys.buf_used = 0;
    reckeys.prevAttrUse = -1;
    reckeys.prevAttrSet = -1;
    reckeys.prevSeqNo = 0;
    
    extractCtrl.init = wordInit;
    extractCtrl.addWord = addRecordKey;
    extractCtrl.addSchema = addSchema;
    extractCtrl.dh = rGroup->dh;
    for (i = 0; i<256; i++)
	extractCtrl.seqno[i] = 0;
    extractCtrl.zebra_maps = rGroup->zebra_maps;
    extractCtrl.flagShowRecords = !rGroup->flagRw;
    
    grs_extract_tree(&extractCtrl, n);

    logf (LOG_DEBUG, "flush explain record, sysno=%d", rec->sysno);

    if (rec->size[recInfo_delKeys])
    {
	struct recKeys delkeys;

	delkeys.buf_used = rec->size[recInfo_delKeys];
	delkeys.buf = rec->info[recInfo_delKeys];
	flushSortKeys (rec->sysno, 0);
	flushRecordKeys (rec->sysno, 0, &delkeys);
    }
    flushRecordKeys (rec->sysno, 1, &reckeys);
    flushSortKeys (rec->sysno, 1);

    xfree (rec->info[recInfo_delKeys]);
    rec->size[recInfo_delKeys] = reckeys.buf_used;
    rec->info[recInfo_delKeys] = reckeys.buf;
    reckeys.buf = NULL;
    reckeys.buf_max = 0;

    return 0;
}
