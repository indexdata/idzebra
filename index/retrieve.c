/* $Id: retrieve.c,v 1.21 2004-05-27 09:28:01 adam Exp $
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

#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "index.h"
#include <direntz.h>

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
    int raw_mode = 0;

    rec = rec_get (zh->reg->records, sysno);
    if (!rec)
    {
        logf (LOG_DEBUG, "rec_get fail on sysno=%d", sysno);
	*basenamep = 0;
        return 14;
    }
    recordAttr = rec_init_attr (zh->reg->zei, rec);

    file_type = rec->info[recInfo_fileType];
    fname = rec->info[recInfo_filename];
    basename = rec->info[recInfo_databaseName];
    *basenamep = (char *) odr_malloc (stream, strlen(basename)+1);
    strcpy (*basenamep, basename);

    if (comp && comp->which == Z_RecordComp_simple &&
        comp->u.simple->which == Z_ElementSetNames_generic)
    {
        if (!strcmp (comp->u.simple->u.generic, "R"))
	    raw_mode = 1;
    }
    if (!(rt = recType_byName (zh->reg->recTypes,
			       file_type, subType, &clientData)))
    {
        logf (LOG_WARN, "Retrieve: Cannot handle type %s",  file_type);
	return 14;
    }
    logf (LOG_DEBUG, "retrieve localno=%d score=%d", sysno, score);
    retrieveCtrl.fh = &fc;
    fc.fd = -1;
    retrieveCtrl.fname = fname;
    if (rec->size[recInfo_storeData] > 0)
    {
        retrieveCtrl.readf = zebra_record_int_read;
        retrieveCtrl.seekf = zebra_record_int_seek;
        retrieveCtrl.tellf = zebra_record_int_tell;
        fc.record_int_len = rec->size[recInfo_storeData];
        fc.record_int_buf = rec->info[recInfo_storeData];
        fc.record_int_pos = 0;
        logf (LOG_DEBUG, "Internal retrieve. %d bytes", fc.record_int_len);
	if (raw_mode)
	{
            *output_format = VAL_SUTRS;
            *rec_lenp = rec->size[recInfo_storeData];
       	    *rec_bufp = (char *) odr_malloc(stream, *rec_lenp);
	    memcpy(*rec_bufp, rec->info[recInfo_storeData], *rec_lenp);
            rec_rm (&rec);
	    return 0;
	}
    }
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

        if ((fc.fd = open (full_rep, O_BINARY|O_RDONLY)) == -1)
        {
            logf (LOG_WARN|LOG_ERRNO, "Retrieve fail; missing file: %s",
		  full_rep);
            rec_rm (&rec);
            return 14;
        }
	fc.record_offset = recordAttr->recordOffset;

        retrieveCtrl.readf = zebra_record_ext_read;
        retrieveCtrl.seekf = zebra_record_ext_seek;
        retrieveCtrl.tellf = zebra_record_ext_tell;

        zebra_record_ext_seek (retrieveCtrl.fh, 0);
	if (raw_mode)
	{
            *output_format = VAL_SUTRS;
            *rec_lenp = recordAttr->recordSize;
       	    *rec_bufp = (char *) odr_malloc(stream, *rec_lenp);
	    zebra_record_ext_read(&fc, *rec_bufp, *rec_lenp);
            rec_rm (&rec);
            close (fc.fd);
	    return 0;
	}
    }
    retrieveCtrl.subType = subType;
    retrieveCtrl.localno = sysno;
    retrieveCtrl.score = score;
    retrieveCtrl.recordSize = recordAttr->recordSize;
    retrieveCtrl.odr = stream;
    retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
    retrieveCtrl.comp = comp;
    retrieveCtrl.encoding = zh->record_encoding;
    retrieveCtrl.diagnostic = 0;
    retrieveCtrl.dh = zh->reg->dh;
    retrieveCtrl.res = zh->res;
    retrieveCtrl.rec_buf = 0;
    retrieveCtrl.rec_len = -1;
    
    (*rt->retrieve)(clientData, &retrieveCtrl);
    *output_format = retrieveCtrl.output_format;
    *rec_bufp = (char *) retrieveCtrl.rec_buf;
    *rec_lenp = retrieveCtrl.rec_len;
    if (fc.fd != -1)
        close (fc.fd);
    rec_rm (&rec);

    return retrieveCtrl.diagnostic;
}
