/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dict.h,v $
 * Revision 1.1  1994-08-16 16:26:53  adam
 * Added dict.
 *
 */

#ifndef DICT_H
#define DICT_H

#include <bfile.h>

typedef unsigned Dict_ptr;
typedef char Dict_char;

struct Dict_head {
    char magic_str[8];
    int page_size;
    Dict_ptr free_list, last;
};

typedef struct Dict_struct {
    BFile bf;
    struct Dict_head head;
} *Dict;

#define DICT_MAGIC "dict00"

typedef int Dict_info;

#define DICT_PAGESIZE 8192
    
Dict dict_open (const char *name, int cache, int rw);
int dict_close (Dict dict);
int dict_insert (Dict dict, const Dict_char *p, void *userinfo);
int dict_lookup (Dict dict, Dict_char *p);
int dict_strcmp (const Dict_char *s1, const Dict_char *s2);
int dict_strlen (const Dict_char *s);

#define DICT_EOS        0
#define DICT_type(x)    0[(Dict_ptr*) x]
#define DICT_backptr(x) 1[(Dict_ptr*) x]
#define DICT_nextptr(x) 2[(Dict_ptr*) x]
#define DICT_nodir(x)   0[(short*)((char*)(x)+3*sizeof(Dict_ptr))]
#define DICT_size(x)    1[(short*)((char*)(x)+3*sizeof(Dict_ptr))]
#define DICT_info(x)    ((char*)(x)+3*sizeof(Dict_ptr)+2*sizeof(short))

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
