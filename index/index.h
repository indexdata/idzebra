/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: index.h,v $
 * Revision 1.2  1995-09-01 10:30:24  adam
 * More work on indexing. Not working yet.
 *
 * Revision 1.1  1995/08/31  14:50:24  adam
 * New simple file index tool.
 *
 */

#include <util.h>
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
