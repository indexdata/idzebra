/* $Id: dict.h,v 1.32 2002-08-02 19:26:55 adam Exp $
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



#ifndef DICT_H
#define DICT_H

#include <bfile.h>
#include <yaz/log.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned Dict_ptr;
typedef unsigned char Dict_char;

struct Dict_head {
    char magic_str[8];
    int page_size;
    int compact_flag;
    Dict_ptr root, last, freelist;
};

struct Dict_file_block
{
    struct Dict_file_block *h_next, **h_prev;
    struct Dict_file_block *lru_next, *lru_prev;
    void *data;
    int dirty;
    int no;
    int nbytes;
};

typedef struct Dict_file_struct
{
    int cache;
    BFile bf;
    
    struct Dict_file_block *all_blocks;
    struct Dict_file_block *free_list;
    struct Dict_file_block **hash_array;
    
    struct Dict_file_block *lru_back, *lru_front;
    int hash_size;
    void *all_data;
    
    int  block_size;
    int  hits;
    int  misses;
    int  compact_flag;
} *Dict_BFile;

typedef struct Dict_struct {
    int rw;
    Dict_BFile dbf;
    const char **(*grep_cmap)(void *vp, const char **from, int len);
    void *grep_cmap_data;
    struct Dict_head head;
} *Dict;

#define DICT_MAGIC "dict01"

#define DICT_DEFAULT_PAGESIZE 4096

int        dict_bf_readp (Dict_BFile bf, int no, void **bufp);
int        dict_bf_newp (Dict_BFile bf, int no, void **bufp, int nbytes);
int        dict_bf_touch (Dict_BFile bf, int no);
void       dict_bf_flush_blocks (Dict_BFile bf, int no_to_flush);
Dict_BFile dict_bf_open (BFiles bfs, const char *name, int block_size,
			 int cache, int rw);
int        dict_bf_close (Dict_BFile dbf);
void       dict_bf_compact (Dict_BFile dbf);
     
Dict       dict_open (BFiles bfs, const char *name, int cache, int rw,
		      int compact_flag);
int        dict_close (Dict dict);
int        dict_insert (Dict dict, const char *p, int userlen, void *userinfo);
int        dict_delete (Dict dict, const char *p);
int        dict_delete_subtree (Dict dict, const char *p, void *client,
				int (*f)(const char *info, void *client));
char      *dict_lookup (Dict dict, const char *p);
int        dict_lookup_ec (Dict dict, char *p, int range,
                           int (*f)(char *name));
int        dict_lookup_grep (Dict dict, const char *p, int range, void *client,
                             int *max_pos, int init_pos,
                             int (*f)(char *name, const char *info,
                                      void *client));
int        dict_strcmp (const Dict_char *s1, const Dict_char *s2);
int        dict_strncmp (const Dict_char *s1, const Dict_char *s2, size_t n);
int        dict_strlen (const Dict_char *s);
int	   dict_scan (Dict dict, char *str, 
		      int *before, int *after, void *client,
		      int (*f)(char *name, const char *info, int pos,
                               void *client));

void       dict_grep_cmap (Dict dict, void *vp,
                           const char **(*cmap)(void *vp,
						const char **from, int len));
int        dict_copy_compact (BFiles bfs, const char *from, const char *to);


#define DICT_EOS        0
#define DICT_type(x)    0[(Dict_ptr*) x]
#define DICT_backptr(x) 1[(Dict_ptr*) x]
#define DICT_bsize(x)   2[(short*)((char*)(x)+2*sizeof(Dict_ptr))]
#define DICT_nodir(x)   0[(short*)((char*)(x)+2*sizeof(Dict_ptr))]
#define DICT_size(x)    1[(short*)((char*)(x)+2*sizeof(Dict_ptr))]
#define DICT_infoffset  (2*sizeof(Dict_ptr)+3*sizeof(short))
#define DICT_xxxxpagesize(x) ((x)->head.page_size)

#define DICT_to_str(x)  sizeof(Dict_info)+sizeof(Dict_ptr)

/*
   type            type of page
   backptr         pointer to parent
   nextptr         pointer to next page (if any)
   nodir           no of words
   size            size of strings,info,ptr entries

   dir[0..nodir-1]
   ptr,info,string
 */
#ifdef __cplusplus
}
#endif

   
#endif
