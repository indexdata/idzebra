/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.h,v $
 * Revision 1.1  1995-11-15 14:46:21  adam
 * Started work on better record management system.
 *
 */

#include <alexutil.h>

typedef struct records_info {
    int fd;
    char *fname;
    struct records_head {
        char magic[8];
	int no_records;
        int freelist;
    } head;
} *Records;

typedef struct record_info {
    int sysno;
    char *type;
    char *fname;
    char *kinfo;
} *Record;

