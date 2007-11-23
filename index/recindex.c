/* $Id: recindex.c,v 1.58 2007-11-23 13:52:52 adam Exp $
   Copyright (C) 1995-2007
   Index Data ApS

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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <yaz/yaz-util.h>
#include "recindex.h"

#define RIDX_CHUNK 128


struct recindex {
    char *index_fname;
    BFile index_BFile;
};

recindex_t recindex_open(BFiles bfs, int rw)
{
    recindex_t p = xmalloc(sizeof(*p));
    p->index_fname = "reci";
    p->index_BFile = bf_open(bfs, p->index_fname, RIDX_CHUNK, rw);
    if (p->index_BFile == NULL)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "open %s", p->index_fname);
	xfree(p);
	return 0;
    }
    return p;
}

void recindex_close(recindex_t p)
{
    if (p)
    {
        if (p->index_BFile)
            bf_close(p->index_BFile);
        xfree(p);
    }
}

int recindex_read_head(recindex_t p, void *buf)
{
    return bf_read(p->index_BFile, 0, 0, 0, buf);
}

const char *recindex_get_fname(recindex_t p)
{
    return p->index_fname;
}

ZEBRA_RES recindex_write_head(recindex_t p, const void *buf, size_t len)
{
    int r;

    assert(p);
    assert(p->index_BFile);

    r = bf_write(p->index_BFile, 0, 0, len, buf);
    if (r)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "write head of %s", p->index_fname);
	return ZEBRA_FAIL;
    }
    return ZEBRA_OK;
}

int recindex_read_indx(recindex_t p, zint sysno, void *buf, int itemsize, 
                       int ignoreError)
{
    int r;
    zint pos = (sysno-1)*itemsize;
    int off = CAST_ZINT_TO_INT(pos%RIDX_CHUNK);
    int sz1 = RIDX_CHUNK - off;    /* sz1 is size of buffer to read.. */

    if (sz1 > itemsize)
	sz1 = itemsize;  /* no more than itemsize bytes */

    r = bf_read(p->index_BFile, 1+pos/RIDX_CHUNK, off, sz1, buf);
    if (r == 1 && sz1 < itemsize) /* boundary? - must read second part */
	r = bf_read(p->index_BFile, 2+pos/RIDX_CHUNK, 0, itemsize - sz1,
			(char*) buf + sz1);
    if (r != 1 && !ignoreError)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "read in %s at pos %ld",
		p->index_fname, (long) pos);
    }
    return r;
}

void recindex_write_indx(recindex_t p, zint sysno, void *buf, int itemsize)
{
    zint pos = (sysno-1)*itemsize;
    int off = CAST_ZINT_TO_INT(pos%RIDX_CHUNK);
    int sz1 = RIDX_CHUNK - off;    /* sz1 is size of buffer to read.. */

    if (sz1 > itemsize)
	sz1 = itemsize;  /* no more than itemsize bytes */

    bf_write(p->index_BFile, 1+pos/RIDX_CHUNK, off, sz1, buf);
    if (sz1 < itemsize)   /* boundary? must write second part */
	bf_write(p->index_BFile, 2+pos/RIDX_CHUNK, 0, itemsize - sz1,
		(char*) buf + sz1);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

