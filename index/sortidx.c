/* $Id: sortidx.c,v 1.13 2005-01-15 19:38:27 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/


 
#include <string.h>

#include <yaz/log.h>
#include <yaz/xmalloc.h>
#include <idzebra/bfile.h>
#include <sortidx.h>

#define SORT_IDX_BLOCKSIZE 64

struct sortFileHead {
    SYSNO sysno_max;
};

struct sortFile {
    int type;
    BFile bf;
    struct sortFile *next;
    struct sortFileHead head;
};

struct sortIdx {
    BFiles bfs;
    int write_flag;
    SYSNO sysno;
    char *entry_buf;
    struct sortFile *current_file;
    struct sortFile *files;
};

SortIdx sortIdx_open (BFiles bfs, int write_flag)
{
    SortIdx si = (SortIdx) xmalloc (sizeof(*si));
    si->bfs = bfs;
    si->write_flag = write_flag;
    si->current_file = NULL;
    si->files = NULL;
    si->entry_buf = (char *) xmalloc (SORT_IDX_ENTRYSIZE);
    return si;
}

void sortIdx_close (SortIdx si)
{
    struct sortFile *sf = si->files;
    while (sf)
    {
	struct sortFile *sf_next = sf->next;
	if (sf->bf)
	    bf_close (sf->bf);
	xfree (sf);
	sf = sf_next;
    }
    xfree (si->entry_buf);
    xfree (si);
}

int sortIdx_type (SortIdx si, int type)
{
    char fname[80];
    struct sortFile *sf;
    if (si->current_file && si->current_file->type == type)
	return 0;
    for (sf = si->files; sf; sf = sf->next)
	if (sf->type == type)
	{
	    si->current_file = sf;
	    return 0;
	}
    sf = (struct sortFile *) xmalloc (sizeof(*sf));
    sf->type = type;
    sf->bf = NULL;
    sprintf (fname, "sort%d", type);
    yaz_log (YLOG_DEBUG, "sort idx %s wr=%d", fname, si->write_flag);
    sf->bf = bf_open (si->bfs, fname, SORT_IDX_BLOCKSIZE, si->write_flag);
    if (!sf->bf)
    {
        xfree (sf);
	return -1;
    }
    if (!bf_read (sf->bf, 0, 0, sizeof(sf->head), &sf->head))
    {
	sf->head.sysno_max = 0;
	if (!si->write_flag)
        {
            bf_close (sf->bf);
            xfree (sf);
	    return -1;
        }
    }
    sf->next = si->files;
    si->current_file = si->files = sf;
    return 0;
}

void sortIdx_sysno (SortIdx si, SYSNO sysno)
{
    si->sysno = sysno;
}

void sortIdx_add (SortIdx si, const char *buf, int len)
{
    if (!si->current_file || !si->current_file->bf)
	return;
    if (len > SORT_IDX_ENTRYSIZE)
    {
	len = SORT_IDX_ENTRYSIZE;
	memcpy (si->entry_buf, buf, len);
    }
    else
    {
	memcpy (si->entry_buf, buf, len);
	memset (si->entry_buf+len, 0, SORT_IDX_ENTRYSIZE-len);
    }
    bf_write (si->current_file->bf, si->sysno+1, 0, 0, si->entry_buf);
}

void sortIdx_read (SortIdx si, char *buf)
{
    int r = bf_read (si->current_file->bf, si->sysno+1, 0, 0, buf);
    if (!r)
        memset (buf, 0, SORT_IDX_ENTRYSIZE);
}
