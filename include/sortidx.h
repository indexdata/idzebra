/*
 * Copyright (C) 1994-1998, Index Data ApS
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: sortidx.h,v $
 * Revision 1.2  1998-06-25 09:55:49  adam
 * Minor changes - fixex headers.
 *
 * Revision 1.1  1998/02/10 12:03:05  adam
 * Implemented Sort.
 *
 */

#ifndef SORTIDX_H
#define SORTIDX_H

#include <bfile.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SORT_IDX_ENTRYSIZE 64

typedef struct sortIdx *SortIdx;

SortIdx sortIdx_open (BFiles bfs, int write_flag);
void sortIdx_close (SortIdx si);
int sortIdx_type (SortIdx si, int type);
void sortIdx_sysno (SortIdx si, int sysno);
void sortIdx_add (SortIdx si, const char *buf, int len);
void sortIdx_read (SortIdx si, char *buf);

#ifdef __cplusplus
}
#endif

#endif
