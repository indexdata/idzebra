/* $Id: isamg.c,v 1.3 2003-04-02 19:01:47 adam Exp $
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



#include <bfile.h>
#include <isamg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Local type declarations 
 * Not to be used from outside this module
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
enum isam_types { 
   isamtype_unknown=0,
   isamtype_d=1,
   }

/* The isam type itself. File handles, statistics, block sizes */
typedef struct ISAMG_file_s {
    char *isam_type_name;
    enum isam_types isam_type;

    /* these might be better off as an union of different types. */
    ISAMD isamd;
    ISAMD_M isamd_m;
    
} *ISAMG_file;

struct ISAMG_s {
/*
    int no_files;
    ISAMG_M method;
    ISAMG_file files;
*/
}; 


typedef struct ISAMG_DIFF_s *ISAMG_DIFF;

/* ISAMG position structure. Used for reading through the isam */
struct ISAMG_PP_s {
#ifdef SKIPTHIS
    char *buf;   /* buffer for read/write operations */
    ISAMG_BLOCK_SIZE offset; /* position for next read/write */
    ISAMG_BLOCK_SIZE size;   /* size of actual data */
    int cat;  /* category of this block */
    int pos;  /* block number of this block */
    int next; /* number of the next block */
    int diffs; /* not used in the modern isam-d, but kept for stats compatibility */
               /* never stored on disk, though */
#endif
    ISAMG is;  /* the isam itself */
    void *decodeClientData;  /* delta-encoder's own data */
#ifdef SKIPTHIS
    ISAMG_DIFF diffinfo;
    char *diffbuf; /* buffer for the diff block */
    int numKeys;
#endif
};

#ifdef SKIPTHIS
#define ISAMG_BLOCK_OFFSET_N (sizeof(int) +  \
                              sizeof(ISAMG_BLOCK_SIZE)) 
/* == 8 */
#define ISAMG_BLOCK_OFFSET_1 (sizeof(int) + \
                              sizeof(ISAMG_BLOCK_SIZE) + \
                              sizeof(ISAMG_BLOCK_SIZE)) 
/* == 12  (was 16) */


int isamg_alloc_block (ISAMG is, int cat);
void isamg_release_block (ISAMG is, int cat, int pos);
int isamg_read_block (ISAMG is, int cat, int pos, char *dst);
int isamg_write_block (ISAMG is, int cat, int pos, char *src);
void isamg_free_diffs(ISAMG_PP pp);

int is_singleton(ISAMG_P ipos);
void singleton_decode (int code, struct it_key *k);
int singleton_encode(struct it_key *k);
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Splitter functions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

ISAMG isamg_open (BFiles bfs, int writeflag, char *isam_type_name, Res res){
    ISAMG is;
    is = (ISAMD) xmalloc(sizeof(*is));
    is->isam_type_name = strdup(isam_type_name);
    if ( (0==strcmp(isam_type_name,"d") ||
         (0==strcmp(isam_type_name,"isam_d")) {
        is->isam_type = isamtype_isamd;
        is->isamd = isamd_open(bfs,FNAME_ISAMD, 
                    writeflag, key_isamd_m (res,&isamd_m)
    }
    else {
        logf (LOG_FATAL, "isamg: Unknown isam type: %s", isam_type_name);
        exit(1);
    }
} /* open */

int isamg_close (ISAMG is){
    assert(is);
    assert(is->isam_type);
    select (is->isam_type) {
        case isamtype_isamd: return isams_close(is->isamd);
    }
} /* close */




#ifdef __cplusplus
}
#endif




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Log
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/*
 * $Log: isamg.c,v $
 * Revision 1.3  2003-04-02 19:01:47  adam
 * Remove // comment
 *
 * Revision 1.2  2002/08/02 19:26:56  adam
 * Towards GPL
 *
 * Revision 1.1  2001/01/16 19:05:45  heikki
 * Started to work on isamg
 *
 *
 *
 */
