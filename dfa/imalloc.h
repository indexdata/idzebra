/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: imalloc.h,v $
 * Revision 1.2  1996-05-14 11:33:41  adam
 * MEMDEBUG turned off by default.
 *
 * Revision 1.1  1994/09/26  10:16:54  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */

void *imalloc        (size_t);
void *icalloc        (size_t);

#if MEMDEBUG
void  i_free         (void *);
void  imemstat       (void);

extern long alloc;
extern long max_alloc;
extern int  alloc_calls;
extern int  free_calls;
#define ifree i_free

#else

#define ifree free

#endif


