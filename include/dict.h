/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dict.h,v $
 * Revision 1.19  1996-02-02 13:43:54  adam
 * The public functions simply use char instead of Dict_char to represent
 * search strings. Dict_char is used internally only.
 *
 * Revision 1.18  1996/02/01  20:41:06  adam
 * Bug fix: insert didn't work on 8-bit characters due to unsigned char
 * compares in dict_strcmp (strcmp) and signed Dict_char. Dict_char is
 * unsigned now.
 *
 * Revision 1.17  1995/12/07  11:47:04  adam
 * Default pagesize is 4k instead of 8k.
 *
 * Revision 1.16  1995/12/06  14:41:13  adam
 * New function: dict_delete.
 *
 * Revision 1.15  1995/10/27  13:59:17  adam
 * Function dict_look_grep got extra parameter max_pos that upon return
 * hold length of longest prefix that matches pattern.
 *
 * Revision 1.14  1995/10/09  16:18:35  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.13  1995/10/06  09:03:51  adam
 * First version of scan.
 *
 * Revision 1.12  1995/09/14  11:53:02  adam
 * Grep handle function parameter info is const now.
 *
 * Revision 1.11  1995/09/04  09:09:51  adam
 * String arg in dict lookup is const.
 * Minor changes.
 *
 * Revision 1.10  1994/10/05  12:16:58  adam
 * Pagesize is a resource now.
 *
 * Revision 1.9  1994/10/04  12:08:19  adam
 * Minor changes.
 *
 * Revision 1.8  1994/10/03  17:23:11  adam
 * First version of dictionary lookup with regular expressions and errors.
 *
 * Revision 1.7  1994/09/22  10:44:47  adam
 * Don't remember what changed!!
 *
 * Revision 1.6  1994/09/16  15:39:21  adam
 * Initial code of lookup - not tested yet.
 *
 * Revision 1.5  1994/09/06  13:05:29  adam
 * Further development of insertion. Some special cases are
 * not properly handled yet! assert(0) are put here. The
 * binary search in each page definitely reduce usr CPU.
 *
 * Revision 1.4  1994/09/01  17:44:40  adam
 * Work on insertion in dictionary. Not finished yet.
 *
 * Revision 1.3  1994/08/18  12:41:12  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.2  1994/08/17  13:32:33  adam
 * Use cache in dict - not in bfile.
 *
 * Revision 1.1  1994/08/16  16:26:53  adam
 * Added dict.
 *
 */

#ifndef DICT_H
#define DICT_H

#include <bfile.h>

typedef unsigned Dict_ptr;
typedef unsigned char Dict_char;

struct Dict_head {
    char magic_str[8];
    int page_size;
    Dict_ptr free_list, last;
};

struct Dict_file_block
{
    struct Dict_file_block *h_next, **h_prev;
    struct Dict_file_block *lru_next, *lru_prev;
    void *data;
    int dirty;
    int no;
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
} *Dict_BFile;

typedef struct Dict_struct {
    int rw;
    Dict_BFile dbf;
    struct Dict_head head;
}
*Dict;

#define DICT_MAGIC "dict00"

#define DICT_DEFAULT_PAGESIZE "4096"

int        dict_bf_readp (Dict_BFile bf, int no, void **bufp);
int        dict_bf_newp (Dict_BFile bf, int no, void **bufp);
int        dict_bf_touch (Dict_BFile bf, int no);
void       dict_bf_flush_blocks (Dict_BFile bf, int no_to_flush);
Dict_BFile dict_bf_open (const char *name, int block_size, int cache, int rw);
int        dict_bf_close (Dict_BFile dbf);
     
Dict       dict_open (const char *name, int cache, int rw);
int        dict_close (Dict dict);
int        dict_insert (Dict dict, const char *p, int userlen, void *userinfo);
int        dict_delete (Dict dict, const char *p);
char      *dict_lookup (Dict dict, const char *p);
int        dict_lookup_ec (Dict dict, char *p, int range,
                           int (*f)(char *name));
int        dict_lookup_grep (Dict dict, const char *p, int range, void *client,
                             int *max_pos,
                             int (*f)(char *name, const char *info,
                                      void *client));
int        dict_strcmp (const Dict_char *s1, const Dict_char *s2);
int        dict_strlen (const Dict_char *s);
int	   dict_scan (Dict dict, char *str, 
		      int *before, int *after, void *client,
		      int (*f)(char *name, const char *info, int pos,
                               void *client));

#define DICT_EOS        0
#define DICT_type(x)    0[(Dict_ptr*) x]
#define DICT_backptr(x) 1[(Dict_ptr*) x]
#define DICT_nextptr(x) 2[(Dict_ptr*) x]
#define DICT_nodir(x)   0[(short*)((char*)(x)+3*sizeof(Dict_ptr))]
#define DICT_size(x)    1[(short*)((char*)(x)+3*sizeof(Dict_ptr))]
#define DICT_infoffset  (3*sizeof(Dict_ptr)+2*sizeof(short))
#define DICT_pagesize(x) ((x)->head.page_size)

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

   
#endif
