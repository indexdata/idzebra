/*
 * Copyright (C) 1995-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.c,v $
 * Revision 1.55  1998-02-10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.54  1998/01/29 13:39:13  adam
 * Compress ISAM is default.
 *
 * Revision 1.53  1998/01/12 15:04:09  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.52  1997/11/18 10:05:08  adam
 * Changed character map facility so that admin can specify character
 * mapping files for each register type, w, p, etc.
 *
 * Revision 1.51  1997/10/27 14:33:06  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.50  1997/09/29 09:08:36  adam
 * Revised locking system to be thread safe for the server.
 *
 * Revision 1.49  1997/09/25 14:57:23  adam
 * Windows NT port.
 *
 * Revision 1.48  1997/09/17 12:19:19  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.47  1997/09/04 13:58:36  adam
 * New retrieve/extract method tellf (added).
 * Added O_BINARY for open calls.
 *
 * Revision 1.46  1997/07/28 08:30:47  adam
 * Server returns diagnostic 14 when record doesn't exist.
 *
 * Revision 1.45  1996/12/23 15:30:45  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.44  1996/12/11 12:08:01  adam
 * Added better compression.
 *
 * Revision 1.43  1996/11/15 15:03:58  adam
 * Logging of execution speed by using the times(2) call.
 *
 * Revision 1.42  1996/11/08  11:10:36  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.41  1996/10/29 14:09:56  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.40  1996/06/04 10:19:02  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.39  1996/05/31  09:07:05  quinn
 * Work on character-set handling
 *
 * Revision 1.38  1996/05/14  11:34:01  adam
 * Scan support in multiple registers/databases.
 *
 * Revision 1.37  1996/05/14  06:16:48  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.36  1996/05/01 13:46:37  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.35  1996/03/26  16:01:14  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.34  1996/03/20  09:36:46  adam
 * Function dict_lookup_grep got extra parameter, init_pos, which marks
 * from which position in pattern approximate pattern matching should occur.
 * Approximate pattern matching is used in relevance=re-2.
 *
 * Revision 1.33  1996/01/17  14:57:56  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int.
 *
 * Revision 1.32  1995/12/11  09:12:58  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.31  1995/12/08  16:22:56  adam
 * Work on update while servers are running. Three lock files introduced.
 * The servers reload their registers when necessary, but they don't
 * reestablish result sets yet.
 *
 * Revision 1.30  1995/12/07  17:38:48  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.29  1995/12/04  14:22:32  adam
 * Extra arg to recType_byName.
 * Started work on new regular expression parsed input to
 * structured records.
 *
 * Revision 1.28  1995/11/28  09:09:48  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.27  1995/11/27  13:58:54  adam
 * New option -t. storeStore data implemented in server.
 *
 * Revision 1.26  1995/11/25  10:24:07  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.25  1995/11/21  15:29:13  adam
 * Config file 'base' read by default by both indexer and server.
 *
 * Revision 1.24  1995/11/20  16:59:47  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.23  1995/11/16  17:00:56  adam
 * Better logging of rpn query.
 *
 * Revision 1.22  1995/11/16  15:34:55  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.21  1995/11/01  16:25:52  quinn
 * *** empty log message ***
 *
 * Revision 1.20  1995/10/27  14:00:12  adam
 * Implemented detection of database availability.
 *
 * Revision 1.19  1995/10/17  18:02:11  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.18  1995/10/16  14:03:09  quinn
 * Changes to support element set names and espec1
 *
 * Revision 1.17  1995/10/16  09:32:40  adam
 * More work on relational op.
 *
 * Revision 1.16  1995/10/13  12:26:44  adam
 * Optimization of truncation.
 *
 * Revision 1.15  1995/10/12  12:40:55  adam
 * Bug fixes in rpn_prox.
 *
 * Revision 1.14  1995/10/09  16:18:37  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.13  1995/10/06  14:38:00  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.12  1995/10/06  13:52:06  adam
 * Bug fixes. Handler may abort further scanning.
 *
 * Revision 1.11  1995/10/06  10:43:57  adam
 * Scan added. 'occurrences' in scan entries not set yet.
 *
 * Revision 1.10  1995/10/02  16:43:32  quinn
 * Set default resulting record type in fetch.
 *
 * Revision 1.9  1995/10/02  15:18:52  adam
 * New member in recRetrieveCtrl: diagnostic.
 *
 * Revision 1.8  1995/09/28  09:19:47  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.7  1995/09/27  16:17:32  adam
 * More work on retrieve.
 *
 * Revision 1.6  1995/09/08  08:53:22  adam
 * Record buffer maintained in server_info.
 *
 * Revision 1.5  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.4  1995/09/06  10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.3  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 * Revision 1.2  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:41  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */
#include <stdio.h>
#include <assert.h>
#ifdef WINDOWS
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include <data1.h>
#include <recctrl.h>
#include <dmalloc.h>

#include "zserver.h"

static int register_lock (ZServerInfo *zi)
{
    time_t lastChange;
    int state = zebra_server_lock_get_state(zi, &lastChange);

    switch (state)
    {
    case 'c':
        state = 1;
        break;
    default:
        state = 0;
    }
    zebra_server_lock (zi, state);
#if USE_TIMES
    times (&zi->tms1);
#endif
    if (zi->registerState == state)
    {
        if (zi->registerChange >= lastChange)
            return 0;
        logf (LOG_LOG, "Register completely updated since last access");
    }
    else if (zi->registerState == -1)
        logf (LOG_LOG, "Reading register using state %d pid=%ld", state,
              (long) getpid());
    else
        logf (LOG_LOG, "Register has changed state from %d to %d",
              zi->registerState, state);
    zi->registerChange = lastChange;
    if (zi->records)
    {
        zebTargetInfo_close (zi->zti, 0);
        dict_close (zi->dict);
	sortIdx_close (zi->sortIdx);
        if (zi->isam)
            is_close (zi->isam);
        if (zi->isamc)
            isc_close (zi->isamc);
        rec_close (&zi->records);
    }
    bf_cache (zi->bfs, state ? res_get (zi->res, "shadow") : NULL);
    zi->registerState = state;
    zi->records = rec_open (zi->bfs, 0);
    if (!(zi->dict = dict_open (zi->bfs, FNAME_DICT, 40, 0)))
        return -1;
    if (!(zi->sortIdx = sortIdx_open (zi->bfs, 0)))
	return -1;
    zi->isam = NULL;
    zi->isamc = NULL;
    if (!res_get_match (zi->res, "isam", "i", NULL))
    {
        if (!(zi->isamc = isc_open (zi->bfs, FNAME_ISAMC,
				    0, key_isamc_m(zi->res))))
            return -1;

    }
    else
    {
        if (!(zi->isam = is_open (zi->bfs, FNAME_ISAM, key_compare, 0,
                                  sizeof (struct it_key), zi->res)))
            return -1;
    }
    zi->zti = zebTargetInfo_open (zi->records, 0);

    return 0;
}

static void register_unlock (ZServerInfo *zi)
{
    static int waitSec = -1;

#if USE_TIMES
    times (&zi->tms2);
    logf (LOG_LOG, "user/system: %ld/%ld",
			(long) (zi->tms2.tms_utime - zi->tms1.tms_utime),
			(long) (zi->tms2.tms_stime - zi->tms1.tms_stime));
#endif
    if (waitSec == -1)
    {
        char *s = res_get (zi->res, "debugRequestWait");
        if (s)
            waitSec = atoi (s);
        else
            waitSec = 0;
    }
#ifdef WINDOWS
#else
    if (waitSec > 0)
        sleep (waitSec);
#endif
    if (zi->registerState != -1)
        zebra_server_unlock (zi, zi->registerState);
}

static int bend_sort (void *handle, bend_sort_rr *rr);

bend_initresult *bend_init (bend_initrequest *q)
{
    bend_initresult *r = odr_malloc (q->stream, sizeof(*r));
    ZServerInfo *zi = xmalloc (sizeof(*zi));
    struct statserv_options_block *sob;

    r->errcode = 0;
    r->errstring = 0;
    r->handle = zi;
    q->bend_sort = bend_sort;

    logf (LOG_DEBUG, "bend_init");

    sob = statserv_getcontrol ();
    logf (LOG_LOG, "Reading resources from %s", sob->configname);
    if (!(zi->res = res_open (sob->configname)))
    {
	logf (LOG_FATAL, "Failed to read resources `%s'", sob->configname);
	r->errcode = 1;
	return r;
    }
    zebra_server_lock_init (zi);
    zi->dh = data1_create ();
    zi->bfs = bfs_create (res_get (zi->res, "register"));
    bf_lockDir (zi->bfs, res_get (zi->res, "lockDir"));
    data1_set_tabpath (zi->dh, res_get(zi->res, "profilePath"));
    zi->sets = NULL;
    zi->registerState = -1;  /* trigger open of registers! */
    zi->registerChange = 0;

    zi->records = NULL;
    zi->registered_sets = NULL;
    zi->zebra_maps = zebra_maps_open (res_get(zi->res, "profilePath"),
				      zi->res);
    return r;
}

bend_searchresult *bend_search (void *handle, bend_searchrequest *q, int *fd)
{
    ZServerInfo *zi = handle;
    bend_searchresult *r = odr_malloc (q->stream, sizeof(*r));

    r->errcode = 0;
    r->errstring = 0;
    r->hits = 0;

    register_lock (zi);
    zi->errCode = 0;
    zi->errString = NULL;

    logf (LOG_LOG, "ResultSet '%s'", q->setname);
    switch (q->query->which)
    {
    case Z_Query_type_1: case Z_Query_type_101:
        r->errcode = rpn_search (zi, q->stream, q->query->u.type_1,
                                q->num_bases, q->basenames, q->setname,
                                &r->hits);
        r->errstring = zi->errString;
        break;
    default:
        r->errcode = 107;
    }
    register_unlock (zi);
    return r;
}

struct fetch_control {
    int record_offset;
    int record_int_pos;
    char *record_int_buf;
    int record_int_len;
    int fd;
};

static int record_ext_read (void *fh, char *buf, size_t count)
{
    struct fetch_control *fc = fh;
    return read (fc->fd, buf, count);
}

static off_t record_ext_seek (void *fh, off_t offset)
{
    struct fetch_control *fc = fh;
    return lseek (fc->fd, offset + fc->record_offset, SEEK_SET);
}

static off_t record_ext_tell (void *fh)
{
    struct fetch_control *fc = fh;
    return lseek (fc->fd, 0, SEEK_CUR) - fc->record_offset;
}

static off_t record_int_seek (void *fh, off_t offset)
{
    struct fetch_control *fc = fh;
    return (off_t) (fc->record_int_pos = offset);
}

static off_t record_int_tell (void *fh)
{
    struct fetch_control *fc = fh;
    return (off_t) fc->record_int_pos;
}

static int record_int_read (void *fh, char *buf, size_t count)
{
    struct fetch_control *fc = fh;
    int l = fc->record_int_len - fc->record_int_pos;
    if (l <= 0)
        return 0;
    l = (l < count) ? l : count;
    memcpy (buf, fc->record_int_buf + fc->record_int_pos, l);
    fc->record_int_pos += l;
    return l;
}

static int record_fetch (ZServerInfo *zi, int sysno, int score, ODR stream,
                          oid_value input_format, Z_RecordComposition *comp,
			  oid_value *output_format, char **rec_bufp,
			  int *rec_lenp, char **basenamep)
{
    Record rec;
    char *fname, *file_type, *basename;
    RecType rt;
    struct recRetrieveCtrl retrieveCtrl;
    char subType[128];
    struct fetch_control fc;

    rec = rec_get (zi->records, sysno);
    if (!rec)
    {
        logf (LOG_DEBUG, "rec_get fail on sysno=%d", sysno);
        return 14;
    }
    file_type = rec->info[recInfo_fileType];
    fname = rec->info[recInfo_filename];
    basename = rec->info[recInfo_databaseName];
    *basenamep = odr_malloc (stream, strlen(basename)+1);
    strcpy (*basenamep, basename);

    if (!(rt = recType_byName (file_type, subType)))
    {
        logf (LOG_WARN, "Retrieve: Cannot handle type %s",  file_type);
	return 14;
    }
    logf (LOG_DEBUG, "retrieve localno=%d score=%d", sysno, score);
    retrieveCtrl.fh = &fc;
    fc.fd = -1;
    if (rec->size[recInfo_storeData] > 0)
    {
        retrieveCtrl.readf = record_int_read;
        retrieveCtrl.seekf = record_int_seek;
        retrieveCtrl.tellf = record_int_tell;
        fc.record_int_len = rec->size[recInfo_storeData];
        fc.record_int_buf = rec->info[recInfo_storeData];
        fc.record_int_pos = 0;
        logf (LOG_DEBUG, "Internal retrieve. %d bytes", fc.record_int_len);
    }
    else 
    {
        if ((fc.fd = open (fname, O_BINARY|O_RDONLY)) == -1)
        {
            logf (LOG_WARN|LOG_ERRNO, "Retrieve fail; missing file: %s",
		  fname);
            rec_rm (&rec);
            return 14;
        }
        memcpy (&fc.record_offset, rec->info[recInfo_offset],
                sizeof(fc.record_offset));

        retrieveCtrl.readf = record_ext_read;
        retrieveCtrl.seekf = record_ext_seek;
        retrieveCtrl.tellf = record_ext_tell;

        record_ext_seek (retrieveCtrl.fh, 0);
    }
    retrieveCtrl.subType = subType;
    retrieveCtrl.localno = sysno;
    retrieveCtrl.score = score;
    retrieveCtrl.odr = stream;
    retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
    retrieveCtrl.comp = comp;
    retrieveCtrl.diagnostic = 0;
    retrieveCtrl.dh = zi->dh;
    (*rt->retrieve)(&retrieveCtrl);
    *output_format = retrieveCtrl.output_format;
    *rec_bufp = retrieveCtrl.rec_buf;
    *rec_lenp = retrieveCtrl.rec_len;
    if (fc.fd != -1)
        close (fc.fd);
    rec_rm (&rec);

    return retrieveCtrl.diagnostic;
}

bend_fetchresult *bend_fetch (void *handle, bend_fetchrequest *q, int *num)
{
    ZServerInfo *zi = handle;
    bend_fetchresult *r = odr_malloc (q->stream, sizeof(*r));
    int positions[2];
    ZServerSetSysno *records;

    register_lock (zi);

    r->errstring = 0;
    r->last_in_set = 0;
    r->basename = "base";

    zi->errCode = 0;

    positions[0] = q->number;
    records = resultSetSysnoGet (zi, q->setname, 1, positions);
    if (!records)
    {
        logf (LOG_DEBUG, "resultSetRecordGet, error");
        r->errcode = 13;
        register_unlock (zi);
        return r;
    }
    if (!records[0].sysno)
    {
        r->errcode = 13;
        logf (LOG_DEBUG, "Out of range. pos=%d", q->number);
        register_unlock (zi);
        return r;
    }
    r->errcode = record_fetch (zi, records[0].sysno,
                              records[0].score, q->stream, q->format,
                              q->comp, &r->format, &r->record, &r->len,
                              &r->basename);
    resultSetSysnoDel (zi, records, 1);
    register_unlock (zi);
    return r;
}

bend_deleteresult *bend_delete (void *handle, bend_deleterequest *q, int *num)
{
    ZServerInfo *zi = handle;
    register_lock (zi);
    register_unlock (zi);
    return 0;
}

bend_scanresult *bend_scan (void *handle, bend_scanrequest *q, int *num)
{
    ZServerInfo *zi = handle;
    bend_scanresult *r = odr_malloc (q->stream, sizeof(*r));
    int status;

    register_lock (zi);
    zi->errCode = 0;
    zi->errString = 0;

    r->term_position = q->term_position;
    r->num_entries = q->num_entries;
    r->errcode = rpn_scan (zi, q->stream, q->term,
                          q->attributeset,
                          q->num_bases, q->basenames,
                          &r->term_position,
                          &r->num_entries, &r->entries, &status);
    r->errstring = zi->errString;
    r->status = status;
    register_unlock (zi);
    return r;
}

void bend_close (void *handle)
{
    ZServerInfo *zi = handle;

    if (zi->records)
    {
        resultSetDestroy (zi);
        zebTargetInfo_close (zi->zti, 0);
        dict_close (zi->dict);
	sortIdx_close (zi->sortIdx);
        if (zi->isam)
            is_close (zi->isam);
        if (zi->isamc)
            isc_close (zi->isamc);
        rec_close (&zi->records);
        register_unlock (zi);
    }
    zebra_maps_close (zi->zebra_maps);
    bfs_destroy (zi->bfs);
    data1_destroy (zi->dh);
    zebra_server_lock_destroy (zi);

    res_close (zi->res);
    xfree (zi);
}

#ifndef WINDOWS
static void pre_init (struct statserv_options_block *sob)
{
    char *pidfile = "zebrasrv.pid";
    int fd = creat (pidfile, 0666);
    
    if (fd == -1)
	logf (LOG_WARN|LOG_ERRNO, "creat %s", pidfile);
    else
    {
	char pidstr[30];
	
	sprintf (pidstr, "%ld", (long) getpid ());
	write (fd, pidstr, strlen(pidstr));
	close (fd);
    }
}
#endif

int bend_sort (void *handle, bend_sort_rr *rr)
{
    ZServerInfo *zi = handle;

#if 1
    register_lock (zi);

    resultSetSort (zi, rr);

    register_unlock (zi);
#endif
    return 0;
}

int main (int argc, char **argv)
{
    struct statserv_options_block *sob;

    sob = statserv_getcontrol ();
    strcpy (sob->configname, FNAME_CONFIG);
#ifndef WINDOWS
    sob->pre_init = pre_init;
#endif
    statserv_setcontrol (sob);

    return statserv_main (argc, argv);
}
