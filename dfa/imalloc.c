/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: imalloc.c,v $
 * Revision 1.3  1994-09-27 16:31:19  adam
 * First version of grepper: grep with error correction.
 *
 * Revision 1.2  1994/09/26  16:30:56  adam
 * Minor changes. imalloc uses xmalloc now.
 *
 * Revision 1.1  1994/09/26  10:16:54  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <util.h>
#include "imalloc.h"

#ifdef MEMDEBUG
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
#ifdef MEMDEBUG
    size_t words = (4*sizeof(unsigned) -1 + size)/sizeof(unsigned);
    char *p = (char *)xmalloc( words*sizeof(unsigned) );
    if( !p )
        log (LOG_FATAL, "No memory: imalloc(%u); c/f %d/%d; %ld/%ld",
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
        log (LOG_FATAL, "Out of memory (imalloc)" );
    return p;
#endif
}

void *icalloc (size_t size)
{
#ifdef MEMDEBUG
    unsigned words = (4*sizeof(unsigned) -1 + size)/sizeof(unsigned);
    char *p = (char *) xcalloc( words*sizeof(unsigned), 1 );
    if( !p )
        log (LOG_FATAL, "No memory: icalloc(%u); c/f %d/%d; %ld/%ld",
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
    void *p = (void) xcalloc( size, 1 );
    if( !p )
        log (LOG_FATAL, "Out of memory (icalloc)" );
    return p;
#endif
}

#ifdef MEMDEBUG
void i_free (void *p)
{
    size_t size;
    if( !p )
        return;
    ++free_calls;
    size = (-2)[(unsigned *) p];
    if( (-1)[(unsigned *) p] != MAG1 )
        log (LOG_FATAL,"Internal: ifree(%u) magic 1 corrupted", size );
    if( size[(unsigned char *) p] != MAG2 )
        log (LOG_FATAL,"Internal: ifree(%u) magic 2 corrupted", size );
    if( (size+1)[(unsigned char *) p] != MAG3 )
        log (LOG_FATAL,"Internal: ifree(%u) magic 3 corrupted", size );
    alloc -= size;
    if( alloc < 0L )
        log (LOG_FATAL,"Internal: ifree(%u) negative alloc.", size );
    xfree( (unsigned *) p-2 );
}
#else
#ifndef ANSI
void i_free (void *p)
{
    if (p)
        xfree( p );
}
#endif
#endif

#ifdef MEMDEBUG
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
