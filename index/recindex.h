/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.h,v $
 * Revision 1.2  1995-11-15 19:13:08  adam
 * Work on record management.
 *
 * Revision 1.1  1995/11/15  14:46:21  adam
 * Started work on better record management system.
 *
 */

#include <alexutil.h>

#define REC_NO_INFO 4

typedef struct record_info {
    int sysno;
    char *info[REC_NO_INFO];
} *Record;

typedef struct records_info *Records;

Record rec_cp (Record rec);
void rec_rm (Record rec);
void rec_put (Records p, Record rec);
Record rec_new (Records p);
Record rec_get (Records p, int sysno);
void rec_close (Records *p);
Records rec_open (int rw);
char *rec_strdup (const char *s);
