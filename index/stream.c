/* $Id: stream.c,v 1.2 2007-01-15 15:10:17 adam Exp $
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
#include <assert.h>

#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "index.h"

struct zebra_mem_control {
    off_t offset_end;
    off_t record_int_pos;
    const char *record_int_buf;
    int record_int_len;
};

struct zebra_ext_control {
    off_t offset_end;
    off_t record_offset;
    int fd;
};

static off_t zebra_mem_seek(struct ZebraRecStream *s, off_t offset)
{
    struct zebra_mem_control *fc = (struct zebra_mem_control *) s->fh;
    return (off_t) (fc->record_int_pos = offset);
}

static off_t zebra_mem_tell(struct ZebraRecStream *s)
{
    struct zebra_mem_control *fc = (struct zebra_mem_control *) s->fh;
    return (off_t) fc->record_int_pos;
}

static int zebra_mem_read(struct ZebraRecStream *s, char *buf, size_t count)
{
    struct zebra_mem_control *fc = (struct zebra_mem_control *) s->fh;
    int l = fc->record_int_len - fc->record_int_pos;
    if (l <= 0)
        return 0;
    l = (l < (int) count) ? l : (int) count;
    memcpy (buf, fc->record_int_buf + fc->record_int_pos, l);
    fc->record_int_pos += l;
    return l;
}

static off_t zebra_mem_end(struct ZebraRecStream *s, off_t *offset)
{
    struct zebra_mem_control *fc = (struct zebra_mem_control *) s->fh;
    if (offset)
        fc->offset_end = *offset;
    return fc->offset_end;
}

static void zebra_mem_destroy(struct ZebraRecStream *s)
{
    struct zebra_mem_control *fc = s->fh;
    xfree(fc);
}

static int zebra_ext_read(struct ZebraRecStream *s, char *buf, size_t count)
{
    struct zebra_ext_control *fc = (struct zebra_ext_control *) s->fh;
    return read(fc->fd, buf, count);
}

static off_t zebra_ext_seek(struct ZebraRecStream *s, off_t offset)
{
    struct zebra_ext_control *fc = (struct zebra_ext_control *) s->fh;
    return lseek(fc->fd, offset + fc->record_offset, SEEK_SET);
}

static off_t zebra_ext_tell(struct ZebraRecStream *s)
{
    struct zebra_ext_control *fc = (struct zebra_ext_control *) s->fh;
    return lseek(fc->fd, 0, SEEK_CUR) - fc->record_offset;
}

static void zebra_ext_destroy(struct ZebraRecStream *s)
{
    struct zebra_ext_control *fc = s->fh;
    if (fc->fd != -1)
        close(fc->fd);
    xfree(fc);
}

static off_t zebra_ext_end(struct ZebraRecStream *s, off_t *offset)
{
    struct zebra_ext_control *fc = (struct zebra_ext_control *) s->fh;
    if (offset)
        fc->offset_end = *offset;
    return fc->offset_end;
}


void zebra_create_stream_mem(struct ZebraRecStream *stream,
                             const char *buf, size_t sz)
{
    struct zebra_mem_control *fc = xmalloc(sizeof(*fc));
    fc->record_int_buf = buf;
    fc->record_int_len = sz;
    fc->record_int_pos = 0;
    fc->offset_end = 0;

    stream->fh = fc;
    stream->readf = zebra_mem_read;
    stream->seekf = zebra_mem_seek;
    stream->tellf = zebra_mem_tell;
    stream->endf = zebra_mem_end;
    stream->destroy = zebra_mem_destroy;
}

void zebra_create_stream_fd(struct ZebraRecStream *stream,
                            int fd, off_t start_offset)
{
    struct zebra_ext_control *fc = xmalloc(sizeof(*fc));

    fc->fd = fd;
    fc->record_offset = start_offset;
    fc->offset_end = 0;
    
    stream->fh = fc;
    stream->readf = zebra_ext_read;
    stream->seekf = zebra_ext_seek;
    stream->tellf = zebra_ext_tell;
    stream->endf = zebra_ext_end;
    stream->destroy = zebra_ext_destroy;
    zebra_ext_seek(stream, 0);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

