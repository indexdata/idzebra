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



#include <idzebra/bfile.h>
#include <idzebra/isamc.h>

YAZ_BEGIN_CDECL

typedef struct {
    zint lastblock;
    zint freelist;
} ISAMC_head;

typedef unsigned ISAMC_BLOCK_SIZE;

typedef struct ISAMC_file_s {
    ISAMC_head head;
    BFile bf;
    int head_is_dirty;

    int no_writes;
    int no_reads;
    int no_skip_writes;
    int no_allocated;
    int no_released;
    int no_remap;

    int no_forward;
    int no_backward;
    zint sum_forward;
    zint sum_backward;
    int no_next;
    int no_prev;

    char *alloc_buf;
    int alloc_entries_num;
    int alloc_entries_max;

    int fc_max;
    zint *fc_list;
} *ISAMC_file;

struct ISAMC_s {
    int no_files;
    int max_cat;
    char *merge_buf;
    ISAMC_M *method;
    ISAMC_file files;
};

struct ISAMC_PP_s {
    char *buf;
    ISAMC_BLOCK_SIZE offset;
    ISAMC_BLOCK_SIZE size;
    int cat;
    zint pos;
    zint next;
    ISAMC is;
    void *decodeClientData;
    int deleteFlag;
    zint numKeys;
};

/*
  first block consists of
      next pointer : zint
      size         : ISAMC_BLOCK_SIZE (int)
      numkeys      : zint
      data
  other blocks consists of
      next pointer : zint
      size :         ISAMC_BLOCK_SIZE (int)
      data
*/
#define ISAMC_BLOCK_OFFSET_1 (sizeof(zint)+sizeof(ISAMC_BLOCK_SIZE)+sizeof(zint))
#define ISAMC_BLOCK_OFFSET_N (sizeof(zint)+sizeof(ISAMC_BLOCK_SIZE))

zint isamc_alloc_block (ISAMC is, int cat);
void isamc_release_block (ISAMC is, int cat, zint pos);
int isamc_read_block (ISAMC is, int cat, zint pos, char *dst);
int isamc_write_block (ISAMC is, int cat, zint pos, char *src);

YAZ_END_CDECL


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

