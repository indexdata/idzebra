/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: index.h,v $
 * Revision 1.5  1995-09-04 12:33:42  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.4  1995/09/04  09:10:35  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 * Revision 1.3  1995/09/01  14:06:35  adam
 * Split of work into more files.
 *
 * Revision 1.2  1995/09/01  10:30:24  adam
 * More work on indexing. Not working yet.
 *
 * Revision 1.1  1995/08/31  14:50:24  adam
 * New simple file index tool.
 *
 */

#include <alexutil.h>
#include <dict.h>
#include <isam.h>

struct it_key {
    int sysno;
    int seqno;
    int field;
};

struct dir_entry {
    char *name;
};

struct dir_entry *dir_open (const char *rep);
void dir_sort (struct dir_entry *e);
void dir_free (struct dir_entry **e_p);
void repository (int cmd, const char *rep, const char *base_path);

void file_extract (int cmd, const char *fname, const char *kname);

void key_open (const char *fname);
int key_close (void);
void key_flush (void);
void key_write (int cmd, struct it_key *k, const char *str);
int key_compare (const void *p1, const void *p2);
int key_compare_x (const struct it_key *i1, const struct it_key *i2);
void key_input (const char *dict_fname, const char *isam_fname, 
                const char *key_fname, int cache);
int key_sort (const char *key_fname, size_t mem);
