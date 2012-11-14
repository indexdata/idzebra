/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

#ifndef ZEBRA_IT_KEY_H
#define ZEBRA_IT_KEY_H

#include <idzebra/util.h>

YAZ_BEGIN_CDECL

#define IT_MAX_WORD 512

#define IT_KEY_LEVEL_MAX 5
struct it_key {
    int  len;
    zint mem[IT_KEY_LEVEL_MAX];
};

void *iscz1_start(void);
void iscz1_reset(void *vp);
void iscz1_stop(void *p);
void iscz1_decode(void *vp, char **dst, const char **src);
void iscz1_encode(void *vp, char **dst, const char **src);

int key_compare(const void *p1, const void *p2);
void key_init(struct it_key *k);
zint key_get_seq(const void *p);
zint key_get_segment(const void *p);
int key_qsort_compare(const void *p1, const void *p2);
char *key_print_it(const void *p, char *buf);
void key_logdump(int mask, const void *p);
void key_logdump_txt(int logmask, const void *p, const char *txt);

int key_SU_decode(int *ch, const unsigned char *out);
int key_SU_encode(int ch, char *out);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

