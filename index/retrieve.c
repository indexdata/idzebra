/*
 * Copyright (C) 1995-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: retrieve.c,v $
 * Revision 1.14  2001-01-22 11:41:41  adam
 * Added support for raw retrieval (element set name "R").
 *
 * Revision 1.13  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.12  2000/03/15 15:00:30  adam
 * First work on threaded version.
 *
 * Revision 1.11  1999/10/29 10:00:00  adam
 * Fixed minor bug where database name wasn't set in zebra_record_fetch.
 *
 * Revision 1.10  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.9  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.8  1999/03/09 16:27:49  adam
 * More work on SDRKit integration.
 *
 * Revision 1.7  1999/03/02 16:15:43  quinn
 * Added "tagsysno" and "tagrank" directives to zebra.cfg.
 *
 * Revision 1.6  1999/02/18 15:01:25  adam
 * Minor changes.
 *
 * Revision 1.5  1999/02/17 11:29:56  adam
 * Fixed in record_fetch. Minor updates to API.
 *
 * Revision 1.4  1999/02/02 14:51:07  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.3  1998/10/28 10:54:40  adam
 * SDRKit integration.
 *
 * Revision 1.2  1998/10/16 08:14:33  adam
 * Updated record control system.
 *
 * Revision 1.1  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 */

#include <stdio.h>
#include <assert.h>

#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include <recctrl.h>
#include "zserver.h"

#ifndef ZEBRASDR
#define ZEBRASDR 0
#endif

#if ZEBRASDR
#include "zebrasdr.h"
#endif

int zebra_record_ext_read (void *fh, char *buf, size_t count)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    return read (fc->fd, buf, count);
}

off_t zebra_record_ext_seek (void *fh, off_t offset)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    return lseek (fc->fd, offset + fc->record_offset, SEEK_SET);
}

off_t zebra_record_ext_tell (void *fh)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    return lseek (fc->fd, 0, SEEK_CUR) - fc->record_offset;
}

off_t zebra_record_int_seek (void *fh, off_t offset)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    return (off_t) (fc->record_int_pos = offset);
}

off_t zebra_record_int_tell (void *fh)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    return (off_t) fc->record_int_pos;
}

int zebra_record_int_read (void *fh, char *buf, size_t count)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    int l = fc->record_int_len - fc->record_int_pos;
    if (l <= 0)
        return 0;
    l = (l < (int) count) ? l : count;
    memcpy (buf, fc->record_int_buf + fc->record_int_pos, l);
    fc->record_int_pos += l;
    return l;
}

void zebra_record_int_end (void *fh, off_t off)
{
    struct zebra_fetch_control *fc = (struct zebra_fetch_control *) fh;
    fc->offset_end = off;
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
    struct zebra_fetch_control fc;
    RecordAttr *recordAttr;
    void *clientData;

    rec = rec_get (zh->service->records, sysno);
    if (!rec)
    {
        logf (LOG_DEBUG, "rec_get fail on sysno=%d", sysno);
	*basenamep = 0;
        return 14;
    }
    recordAttr = rec_init_attr (zh->service->zei, rec);

    file_type = rec->info[recInfo_fileType];
    fname = rec->info[recInfo_filename];
    basename = rec->info[recInfo_databaseName];
    *basenamep = (char *) odr_malloc (stream, strlen(basename)+1);
    strcpy (*basenamep, basename);

    if (comp && comp->which == Z_RecordComp_simple &&
        comp->u.simple->which == Z_ElementSetNames_generic)
    {
        if (!strcmp (comp->u.simple->u.generic, "R"))
            file_type = "text";
    }
    if (!(rt = recType_byName (zh->service->recTypes,
			       file_type, subType, &clientData)))
    {
        logf (LOG_WARN, "Retrieve: Cannot handle type %s",  file_type);
	return 14;
    }
    logf (LOG_DEBUG, "retrieve localno=%d score=%d", sysno, score);
    retrieveCtrl.fh = &fc;
    fc.fd = -1;
    if (rec->size[recInfo_storeData] > 0)
    {
        retrieveCtrl.readf = zebra_record_int_read;
        retrieveCtrl.seekf = zebra_record_int_seek;
        retrieveCtrl.tellf = zebra_record_int_tell;
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

        retrieveCtrl.readf = zebra_record_ext_read;
        retrieveCtrl.seekf = zebra_record_ext_seek;
        retrieveCtrl.tellf = zebra_record_ext_tell;

        zebra_record_ext_seek (retrieveCtrl.fh, 0);
    }
    retrieveCtrl.subType = subType;
    retrieveCtrl.localno = sysno;
    retrieveCtrl.score = score;
    retrieveCtrl.recordSize = recordAttr->recordSize;
    retrieveCtrl.odr = stream;
    retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
    retrieveCtrl.comp = comp;
    retrieveCtrl.diagnostic = 0;
    retrieveCtrl.dh = zh->service->dh;
    retrieveCtrl.res = zh->service->res;
    (*rt->retrieve)(clientData, &retrieveCtrl);
    *output_format = retrieveCtrl.output_format;
    *rec_bufp = (char *) retrieveCtrl.rec_buf;
    *rec_lenp = retrieveCtrl.rec_len;
    if (fc.fd != -1)
        close (fc.fd);
    rec_rm (&rec);

    return retrieveCtrl.diagnostic;
}
