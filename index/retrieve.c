/*
 * Copyright (C) 1995-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: retrieve.c,v $
 * Revision 1.1  1998-03-05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 */

#include <stdio.h>
#include <assert.h>

#include <fcntl.h>
#ifdef WINDOWS
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <recctrl.h>
#include "zserver.h"

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

int zebra_record_fetch (ZebraHandle zh, int sysno, int score, ODR stream,
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
    RecordAttr *recordAttr;

    rec = rec_get (zh->records, sysno);
    if (!rec)
    {
        logf (LOG_DEBUG, "rec_get fail on sysno=%d", sysno);
        return 14;
    }
    recordAttr = rec_init_attr (zh->zei, rec);

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
	fc.record_offset = recordAttr->recordOffset;

        retrieveCtrl.readf = record_ext_read;
        retrieveCtrl.seekf = record_ext_seek;
        retrieveCtrl.tellf = record_ext_tell;

        record_ext_seek (retrieveCtrl.fh, 0);
    }
    retrieveCtrl.subType = subType;
    retrieveCtrl.localno = sysno;
    retrieveCtrl.score = score;
    retrieveCtrl.recordSize = recordAttr->recordSize;
    retrieveCtrl.odr = stream;
    retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
    retrieveCtrl.comp = comp;
    retrieveCtrl.diagnostic = 0;
    retrieveCtrl.dh = zh->dh;
    (*rt->retrieve)(&retrieveCtrl);
    *output_format = retrieveCtrl.output_format;
    *rec_bufp = retrieveCtrl.rec_buf;
    *rec_lenp = retrieveCtrl.rec_len;
    if (fc.fd != -1)
        close (fc.fd);
    rec_rm (&rec);

    return retrieveCtrl.diagnostic;
}
