/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.h,v $
 * Revision 1.13  1998-03-05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.12  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.11  1997/09/17 12:19:16  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.10  1996/10/29 14:06:53  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.9  1996/05/01 13:46:36  adam
 * First work on multiple records in one file.
 * New option, -offset, to the "unread" command in the filter module.
 *
 * Revision 1.8  1995/12/06  12:41:25  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.7  1995/11/28  09:09:45  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.6  1995/11/27  09:56:21  adam
 * Record info elements better enumerated. Internal store of records.
 *
 * Revision 1.5  1995/11/25  10:24:07  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.4  1995/11/22  17:19:19  adam
 * Record management uses the bfile system.
 *
 * Revision 1.3  1995/11/20  16:59:46  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.2  1995/11/15  19:13:08  adam
 * Work on record management.
 *
 * Revision 1.1  1995/11/15  14:46:21  adam
 * Started work on better record management system.
 *
 */

#include <zebrautl.h>
#include <bfile.h>

#define REC_NO_INFO 8

typedef struct record_info {
    int sysno;
    int newFlag;
    char *info[REC_NO_INFO];
    size_t size[REC_NO_INFO];
} *Record;

typedef struct records_info *Records;

Record rec_cp (Record rec);
void rec_del (Records p, Record *recpp);
void rec_rm (Record *recpp);
void rec_put (Records p, Record *recpp);
Record rec_new (Records p);
Record rec_get (Records p, int sysno);
void rec_close (Records *p);
Records rec_open (BFiles bfs, int rw);
char *rec_strdup (const char *s, size_t *len);
void rec_prstat (Records p);

enum { 
    recInfo_fileType, 
    recInfo_filename, 
    recInfo_delKeys, 
    recInfo_databaseName,
    recInfo_storeData,
    recInfo_attr
};

