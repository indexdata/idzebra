/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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
#include <stdlib.h>

#include <idzebra/util.h>
#include <yaz/xmalloc.h>
#include "imalloc.h"

#if MEMDEBUG
#define MAG1 0x8fe1
#define MAG2 0x91
#define MAG3 0xee

long alloc       = 0L;
long max_alloc   = 0L;
int  alloc_calls = 0;
int  free_calls  = 0;
#endif

void *imalloc (size_t size)
{
#if MEMDEBUG
    size_t words = (4*sizeof(unsigned) -1 + size)/sizeof(unsigned);
    char *p = (char *)xmalloc( words*sizeof(unsigned) );
    if( !p )
        yaz_log (YLOG_FATAL, "No memory: imalloc(%u); c/f %d/%d; %ld/%ld",
           size, alloc_calls, free_calls, alloc, max_alloc );
    *((unsigned *)p) = size;
    ((unsigned *)p)[1] = MAG1;
    p += sizeof(unsigned)*2;
    size[(unsigned char *) p] = MAG2;
    size[(unsigned char *) p+1] = MAG3;
    if( (alloc+=size) > max_alloc )
        max_alloc = alloc;
    ++alloc_calls;
    return (void *) p;
#else
    void *p = (void *)xmalloc( size );
    if( !p )
        yaz_log (YLOG_FATAL, "Out of memory (imalloc)" );
    return p;
#endif
}

void *icalloc (size_t size)
{
#if MEMDEBUG
    unsigned words = (4*sizeof(unsigned) -1 + size)/sizeof(unsigned);
    char *p = (char *) xcalloc( words*sizeof(unsigned), 1 );
    if( !p )
        yaz_log (YLOG_FATAL, "No memory: icalloc(%u); c/f %d/%d; %ld/%ld",
           size, alloc_calls, free_calls, alloc, max_alloc );
    ((unsigned *)p)[0] = size;
    ((unsigned *)p)[1] = MAG1;
    p += sizeof(unsigned)*2;
    size[(unsigned char *) p] = MAG2;
    size[(unsigned char *) p+1] = MAG3;
    if( (alloc+=size) > max_alloc )
        max_alloc = alloc;
    ++alloc_calls;
    return (void *)p;
#else
    void *p = (void *) xcalloc( size, 1 );
    if( !p )
        yaz_log (YLOG_FATAL, "Out of memory (icalloc)" );
    return p;
#endif
}

void ifree (void *p)
{
#if MEMDEBUG
    size_t size;
    if( !p )
        return;
    ++free_calls;
    size = (-2)[(unsigned *) p];
    if( (-1)[(unsigned *) p] != MAG1 )
        yaz_log (YLOG_FATAL,"Internal: ifree(%u) magic 1 corrupted", size );
    if( size[(unsigned char *) p] != MAG2 )
        yaz_log (YLOG_FATAL,"Internal: ifree(%u) magic 2 corrupted", size );
    if( (size+1)[(unsigned char *) p] != MAG3 )
        yaz_log (YLOG_FATAL,"Internal: ifree(%u) magic 3 corrupted", size );
    alloc -= size;
    if( alloc < 0L )
        yaz_log (YLOG_FATAL,"Internal: ifree(%u) negative alloc.", size );
    xfree( (unsigned *) p-2 );
#else
    xfree (p);
#endif
}

#if MEMDEBUG
void imemstat (void)
{
    fprintf( stdout, "imalloc: calls malloc/free %d/%d, ",
                                     alloc_calls, free_calls );
    if( alloc )
        fprintf( stdout, "memory cur/max %ld/%ld : unreleased",
                                                          alloc, max_alloc );
    else
        fprintf( stdout, "memory max %ld", max_alloc );
    fputc( '\n', stdout );
}
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

