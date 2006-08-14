/* $Id: isams.h,v 1.5.2.1 2006-08-14 10:38:56 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#ifndef ISAMS_H
#define ISAMS_H

#include <bfile.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ISAMS_s *ISAMS;
typedef int ISAMS_P;
typedef struct ISAMS_PP_s *ISAMS_PP;

typedef struct ISAMS_M_s {
    int (*compare_item)(const void *a, const void *b);
    void (*log_item)(int logmask, const void *p, const char *txt);

#define ISAMC_DECODE 0
#define ISAMC_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);

    int debug;
    int block_size;
} ISAMS_M;

typedef struct ISAMS_I_s {
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMS_I;

void isams_getmethod (ISAMS_M *me);

ISAMS isams_open (BFiles bfs, const char *name, int writeflag,
		  ISAMS_M *method);
int isams_close (ISAMS is);
ISAMS_P isams_merge (ISAMS is, ISAMS_I data);
ISAMS_PP isams_pp_open (ISAMS is, ISAMS_P pos);
void isams_pp_close (ISAMS_PP pp);
int isams_read_item (ISAMS_PP pp, char **dst);
int isams_pp_read (ISAMS_PP pp, void *buf);
int isams_pp_num (ISAMS_PP pp);

#ifdef __cplusplus
}
#endif

#endif
