/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.h,v $
 * Revision 1.2  1995-09-06 16:11:19  adam
 * Option: only one word key per file.
 *
 * Revision 1.1  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 */

#include "index.h"
#include <proto.h>
#include <rset.h>

typedef struct {
    size_t size;
    char *buf;
} ZServerRecord;

typedef struct ZServerSet_ {
    char *name;
    RSET rset;
    int size;
    struct ZServerSet_ *next;
} ZServerSet;
   
typedef struct {
    ZServerSet *sets;
    Dict wordDict;
    ISAM wordIsam;
    Dict fileDict;
    int sys_idx_fd;
} ZServerInfo;

int rpn_search (ZServerInfo *zi, 
                Z_RPNQuery *rpn, int num_bases, char **basenames, 
                const char *setname, int *hits);

ZServerSet *resultSetAdd (ZServerInfo *zi, const char *name,
                          int ov, RSET rset);
ZServerSet *resultSetGet (ZServerInfo *zi, const char *name);
ZServerRecord *resultSetRecordGet (ZServerInfo *zi, const char *name,
                                   int num, int *positions);
void resultSetRecordDel (ZServerInfo *zi, ZServerRecord *records, int num);
