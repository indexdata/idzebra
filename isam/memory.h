/* $Id: memory.h,v 1.8 2002-08-02 19:26:56 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

extern int is_mbuf_size[3];

typedef unsigned int ISAM_P;

/*
 * Memory buffer. Used to manage records (keys) in memory for
 * reading and insertion/deletion.
 */
typedef struct is_mbuf
{
    int refcount;
    int type;
#define IS_MBUF_TYPE_SMALL  0
#define IS_MBUF_TYPE_MEDIUM 1
#define IS_MBUF_TYPE_LARGE  2 
    int offset;
    int num;
    int cur_record;
    struct is_mbuf *next;
    char *data;                 /* dummy */
} is_mbuf;

/*
 * Structure describing the virtual image of a disk block.
 * (these blocks may grow beyond the blocksize during insertion).
 */
typedef struct is_mblock
{
    int num_records;              /* number of records */
    int diskpos;                  /* positive if this block is mapped */
    int nextpos;                  /* pos of nxt blk. Only valid imm aft. rd. */
    int bread;                    /* number of bytes read */
    int state;                    /* state of block in rel. to disk block */
#define IS_MBSTATE_UNREAD  0          /* block hasn't been read */
#define IS_MBSTATE_PARTIAL 1          /* block has been partially read */
#define IS_MBSTATE_CLEAN   2          /* block is clean (unmodified) */
#define IS_MBSTATE_DIRTY   3          /* block has been modified */
    is_mbuf *cur_mbuf;
    is_mbuf *data;                /* data contained in block */

    struct is_mblock *next;       /* next diskblock */
} is_mblock;

typedef struct isam_struct *ISAM;
/*
 * Descriptor for a specific table.
 */
typedef struct is_mtable
{
    int num_records;               /* total number of records */
    int pos_type;                  /* blocktype */
    is_mblock *cur_mblock;
    is_mbuf *last_mbuf;
    is_mblock *data;               /* blocks contained in this table */
    ISAM is;
} is_mtable;

is_mblock *xmalloc_mblock();
is_mbuf *xmalloc_mbuf(int type);
void xfree_mblock(is_mblock *p);
void xfree_mblocks(is_mblock *l);
void xfree_mbuf(is_mbuf *p);
void xfree_mbufs(is_mbuf *l);
void xrelease_mblock(is_mblock *p);
void is_m_establish_tab(ISAM is, is_mtable *tab, ISAM_P pos);
void is_m_release_tab(is_mtable *tab);
void is_m_rewind(is_mtable *tab);
void is_m_replace_record(is_mtable *tab, const void *rec);
int is_m_write_record(is_mtable *tab, const void *rec);
void is_m_unread_record(is_mtable *tab);
int is_m_read_record(is_mtable *tab, void *buf, int keep);
int is_m_seek_record(is_mtable *tab, const void *rec);
void is_m_delete_record(is_mtable *tab);
int is_m_peek_record(is_mtable *tab, void *rec);
int is_m_read_full(is_mtable *tab, is_mblock *mblock);
int is_m_num_records(is_mtable *tab);

#ifdef __cplusplus
}
#endif


#endif
