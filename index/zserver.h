/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.h,v $
 * Revision 1.24  1997-09-17 12:19:19  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.23  1996/12/23 15:30:46  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.22  1996/11/04 14:07:49  adam
 * Moved truncation code to trunc.c.
 *
 * Revision 1.21  1996/10/29 14:09:58  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.20  1996/06/04 10:19:02  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.19  1996/05/14  11:34:01  adam
 * Scan support in multiple registers/databases.
 *
 * Revision 1.18  1996/05/14  06:16:50  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.17  1995/12/08 16:22:57  adam
 * Work on update while servers are running. Three lock files introduced.
 * The servers reload their registers when necessary, but they don't
 * reestablish result sets yet.
 *
 * Revision 1.16  1995/12/07  17:38:48  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.15  1995/11/21  15:29:13  adam
 * Config file 'base' read by default by both indexer and server.
 *
 * Revision 1.14  1995/11/16  17:00:57  adam
 * Better logging of rpn query.
 *
 * Revision 1.13  1995/11/16  15:34:56  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.12  1995/10/27  14:00:12  adam
 * Implemented detection of database availability.
 *
 * Revision 1.11  1995/10/17  18:02:12  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.10  1995/10/09  16:18:38  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.9  1995/10/06  14:38:01  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.8  1995/10/06  13:52:06  adam
 * Bug fixes. Handler may abort further scanning.
 *
 * Revision 1.7  1995/10/06  10:43:57  adam
 * Scan added. 'occurrences' in scan entries not set yet.
 *
 * Revision 1.6  1995/09/28  09:19:48  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.5  1995/09/27  16:17:32  adam
 * More work on retrieve.
 *
 * Revision 1.4  1995/09/14  11:53:28  adam
 * First work on regular expressions/truncations.
 *
 * Revision 1.3  1995/09/08  08:53:23  adam
 * Record buffer maintained in server_info.
 *
 * Revision 1.2  1995/09/06  16:11:19  adam
 * Option: only one word key per file.
 *
 * Revision 1.1  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 */

#include <backend.h>
#include <rset.h>

#include "index.h"
#include "zinfo.h"

typedef struct {
    int sysno;
    int score;
} ZServerSetSysno;

typedef struct ZServerSet_ {
    char *name;
    RSET rset;
    int size;
    struct ZServerSet_ *next;
} ZServerSet;
   
typedef struct {
    int registerState; /* 0 (no commit pages), 1 (use commit pages) */
    time_t registerChange;
    ZServerSet *sets;
    Dict dict;
    ISAM isam;
    ISAMC isamc;
    Records records;
    int errCode;
    char *errString;
    ODR odr;
    ZebTargetInfo *zti;
    data1_handle dh;
    data1_attset *registered_sets;
    BFiles bfs;
    Res res;
} ZServerInfo;

int rpn_search (ZServerInfo *zi, 
                Z_RPNQuery *rpn, int num_bases, char **basenames, 
                const char *setname, int *hits);

int rpn_scan (ZServerInfo *zi, Z_AttributesPlusTerm *zapt,
              oid_value attributeset,
              int num_bases, char **basenames,
              int *position, int *num_entries, struct scan_entry **list,
              int *status);

RSET rset_trunc (ZServerInfo *zi, ISAM_P *isam_p, int no);

ZServerSet *resultSetAdd (ZServerInfo *zi, const char *name,
                          int ov, RSET rset);
ZServerSet *resultSetGet (ZServerInfo *zi, const char *name);
void resultSetDestroy (ZServerInfo *zi);

ZServerSetSysno *resultSetSysnoGet (ZServerInfo *zi, const char *name,
                                    int num, int *positions);
void resultSetSysnoDel (ZServerInfo *zi, ZServerSetSysno *records, int num);
void zlog_rpn (Z_RPNQuery *rpn);

int zebraServerLock (Res res, int lockCommit);
void zebraServerUnlock (int commitPhase);
int zebraServerLockGetState (Res res, time_t *timep);

typedef struct attent
{
    int attset_ordinal;
    data1_local_attribute *local_attributes;
} attent;

int att_getentbyatt(ZServerInfo *zi, attent *res, oid_value set, int att);
