/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.h,v $
 * Revision 1.7  1995-11-28 09:09:45  adam
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

#include <alexutil.h>

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
Records rec_open (int rw);
char *rec_strdup (const char *s, size_t *len);

enum { 
    recInfo_fileType, 
    recInfo_filename, 
    recInfo_delKeys, 
    recInfo_databaseName,
    recInfo_storeData
};
