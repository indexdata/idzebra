/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: memory.h,v $
 * Revision 1.3  1994-09-28 16:58:33  quinn
 * Small mod.
 *
 * Revision 1.2  1994/09/27  20:03:52  quinn
 * Seems relatively bug-free.
 *
 * Revision 1.1  1994/09/26  17:12:32  quinn
 * Back again
 *
 * Revision 1.1  1994/09/26  16:07:57  quinn
 * Most of the functionality in place.
 *
 */

#ifndef MEMORY_H
#define MEMORY_H

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
int is_m_read_record(is_mtable *tab, void *buf);
int is_m_seek_record(is_mtable *tab, const void *rec);
void is_m_delete_record(is_mtable *tab);
int is_m_peek_record(is_mtable *tab, void *rec);
int is_m_read_full(is_mtable *tab, is_mblock *mblock);
int is_m_num_records(is_mtable *tab);

#endif
