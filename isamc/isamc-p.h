/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc-p.h,v $
 * Revision 1.1  1996-10-29 13:40:47  adam
 * First work.
 *
 */

#include <bfile.h>
#include <isamc.h>

typedef struct {
    int lastblock;
    int freelist;
} ISAMC_head;

typedef struct ISAMC_file_s {
    ISAMC_head head;
    BFile bf;
    int head_is_dirty;
} *ISAMC_file;

struct ISAMC_s {
    int no_files;
    int max_cat;
    char *r_buf;
    ISAMC_M method;
    ISAMC_file files;
}; 

struct ISAMC_PP_s {
    char *buf;
    int offset;
    int size;
    int cat;
    int pos;
    int next;
    ISAMC is;
    void *decodeClientData;
};

#define ISAMC_BLOCK_OFFSET (sizeof(int)+sizeof(int))    
