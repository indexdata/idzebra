/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: imalloc.h,v $
 * Revision 1.5  1999-05-26 07:49:12  adam
 * C++ compilation.
 *
 * Revision 1.4  1999/02/02 14:50:09  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.3  1997/10/27 14:27:13  adam
 * Minor changes.
 *
 * Revision 1.2  1996/05/14 11:33:41  adam
 * MEMDEBUG turned off by default.
 *
 * Revision 1.1  1994/09/26  10:16:54  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

void *imalloc        (size_t);
void *icalloc        (size_t);
void  ifree         (void *);

#if MEMDEBUG
void  imemstat       (void);

extern long alloc;
extern long max_alloc;
extern int  alloc_calls;
extern int  free_calls;

#endif

#ifdef __cplusplus
}
#endif

