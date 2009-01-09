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


#ifndef ISAMS_H
#define ISAMS_H

#include <idzebra/isam-codec.h>
#include <idzebra/bfile.h>
#include <idzebra/isamc.h>

YAZ_BEGIN_CDECL

typedef struct ISAMS_s *ISAMS;
typedef struct ISAMS_PP_s *ISAMS_PP;

typedef struct ISAMS_M_s {
    int (*compare_item)(const void *a, const void *b);
    void (*log_item)(int logmask, const void *p, const char *txt);

    ISAM_CODEC codec;

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
ISAM_P isams_merge (ISAMS is, ISAMS_I data);
ISAMS_PP isams_pp_open (ISAMS is, ISAM_P pos);
void isams_pp_close (ISAMS_PP pp);
int isams_read_item (ISAMS_PP pp, char **dst);
int isams_pp_read (ISAMS_PP pp, void *buf);
int isams_pp_num (ISAMS_PP pp);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

