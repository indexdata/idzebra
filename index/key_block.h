/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#ifndef ZEBRA_KEY_BLOCK_H
#define ZEBRA_KEY_BLOCK_H

#include "it_key.h"

YAZ_BEGIN_CDECL

typedef struct zebra_key_block *zebra_key_block_t;

zebra_key_block_t key_block_create(size_t mem, const char *key_tmp_dir,
                                   int use_threads);
void key_block_destroy(zebra_key_block_t *pp);
void key_block_flush(zebra_key_block_t p, int is_final);
void key_block_write(zebra_key_block_t p, zint sysno, struct it_key *key_in,
                     int cmd, const char *str_buf, size_t str_len,
                     zint staticrank, int static_rank_enable);
int key_block_get_no_files(zebra_key_block_t p);

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

