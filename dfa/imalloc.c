/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: imalloc.c,v $
 * Revision 1.7  1997-10-27 14:27:13  adam
 * Minor changes.
 *
 * Revision 1.6  1996/10/29 13:57:25  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.5  1996/05/14 11:33:41  adam
 * MEMDEBUG turned off by default.
 *
 * Revision 1.4  1995/09/04  12:33:26  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.3  1994/09/27  16:31:19  adam
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

#include <zebrautl.h>
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
        logf (LOG_FATAL, "No memory: imalloc(%u); c/f %d/%d; %ld/%ld",
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
        logf (LOG_FATAL, "Out of memory (imalloc)" );
    return p;
#endif
}

void *icalloc (size_t size)
{
#if MEMDEBUG
    unsigned words = (4*sizeof(unsigned) -1 + size)/sizeof(unsigned);
    char *p = (char *) xcalloc( words*sizeof(unsigned), 1 );
    if( !p )
        logf (LOG_FATAL, "No memory: icalloc(%u); c/f %d/%d; %ld/%ld",
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
        logf (LOG_FATAL, "Out of memory (icalloc)" );
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
        logf (LOG_FATAL,"Internal: ifree(%u) magic 1 corrupted", size );
    if( size[(unsigned char *) p] != MAG2 )
        logf (LOG_FATAL,"Internal: ifree(%u) magic 2 corrupted", size );
    if( (size+1)[(unsigned char *) p] != MAG3 )
        logf (LOG_FATAL,"Internal: ifree(%u) magic 3 corrupted", size );
    alloc -= size;
    if( alloc < 0L )
        logf (LOG_FATAL,"Internal: ifree(%u) negative alloc.", size );
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
