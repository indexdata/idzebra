/*
 * Copyright (C) 1994-2000, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.h,v $
 * Revision 1.46  2000-03-15 15:00:31  adam
 * First work on threaded version.
 *
 * Revision 1.45  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.44  1999/11/04 15:00:45  adam
 * Implemented delete result set(s).
 *
 * Revision 1.43  1999/07/14 10:59:27  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.42  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.41  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.40  1998/11/16 16:03:45  adam
 * Moved loggin utilities to Yaz. Was implemented in file zlogs.c.
 *
 * Revision 1.39  1998/10/16 08:14:35  adam
 * Updated record control system.
 *
 * Revision 1.38  1998/09/22 10:03:44  adam
 * Changed result sets to be persistent in the sense that they can
 * be re-searched if needed.
 * Fixed memory leak in rsm_or.
 *
 * Revision 1.37  1998/09/02 13:53:22  adam
 * Extra parameter decode added to search routines to implement
 * persistent queries.
 *
 * Revision 1.36  1998/06/24 12:16:16  adam
 * Support for relations on text operands. Open range support in
 * DFA module (i.e. [-j], [g-]).
 *
 * Revision 1.35  1998/06/23 15:33:35  adam
 * Added feature to specify sort criteria in query (type 7 specifies
 * sort flags).
 *
 * Revision 1.34  1998/06/22 11:36:50  adam
 * Added authentication check facility to zebra.
 *
 * Revision 1.33  1998/06/12 12:22:14  adam
 * Work on Zebra API.
 *
 * Revision 1.32  1998/05/27 16:57:47  adam
 * Zebra returns surrogate diagnostic for single records when
 * appropriate.
 *
 * Revision 1.31  1998/05/20 10:12:23  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.30  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.29  1998/02/10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.28  1998/01/29 13:40:11  adam
 * Better logging for scan service.
 *
 * Revision 1.27  1997/10/27 14:33:06  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.26  1997/09/29 12:41:35  adam
 * Fixed bug regarding USE_TIMES var.
 *
 * Revision 1.25  1997/09/29 09:08:36  adam
 * Revised locking system to be thread safe for the server.
 *
 * Revision 1.24  1997/09/17 12:19:19  adam
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

#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#if HAVE_PTHREADS_H
#include <pthreads.h>
#endif

#include <yaz/backend.h>
#include <rset.h>

#include <sortidx.h>
#include <passwddb.h>
#include "index.h"
#include "zebraapi.h"
#include "zinfo.h"

YAZ_BEGIN_CDECL

typedef struct {
    int sysno;
    int score;
} *ZebraPosSet;

typedef struct zebra_set *ZebraSet;

typedef struct zebra_rank_class {
    struct rank_control *control;
    int init_flag;
    void *class_handle;
    struct zebra_rank_class *next;
} *ZebraRankClass;

struct zebra_service {
    char *configName;
    struct zebra_session *sessions;
    ISAMS isams;
#if ZMBOL
    ISAM isam;
    ISAMC isamc;
#endif
    Dict dict;
    SortIdx sortIdx;
    int registerState; /* 0 (no commit pages), 1 (use commit pages) */
    time_t registerChange;
    BFiles bfs;
    Records records;
    ZebraExplainInfo zei;
    Res res;
    ZebraLockHandle server_lock_cmt;
    ZebraLockHandle server_lock_org;
    char *server_path_prefix;
#if HAVE_SYS_TIMES_H
    struct tms tms1;
    struct tms tms2;    
#endif
    data1_handle dh;
    ZebraMaps zebra_maps;
    ZebraRankClass rank_classes;
    RecTypes recTypes;
    Passwd_db passwd_db;
};

struct zebra_session {
#if HAVE_PTHREADS_H
    pthread_t *pthread_session;
#endif
    struct zebra_session *next;
    struct zebra_info *info;
    struct zebra_service *service;

    ZebraSet sets;
    int errCode;
    int hits;
    char *errString;

};

struct rank_control {
    char *name;
    void *(*create)(ZebraService zh);
    void (*destroy)(ZebraService zh, void *class_handle);
    void *(*begin)(ZebraHandle zh, void *class_handle, RSET rset);
    void (*end)(ZebraHandle zh, void *set_handle);
    int (*calc)(void *set_handle, int sysno);
    void (*add)(void *set_handle, int seqno, int term_index);
};

RSET rpn_search (ZebraHandle zh, NMEM mem,
		 Z_RPNQuery *rpn, int num_bases, char **basenames, 
		 const char *setname, ZebraSet sset);


void rpn_scan (ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
	       oid_value attributeset,
	       int num_bases, char **basenames,
	       int *position, int *num_entries, ZebraScanEntry **list,
	       int *is_partial);

RSET rset_trunc (ZebraHandle zh, ISAMS_P *isam_p, int no,
		 const char *term, int length_term, const char *flags);

ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov);
ZebraSet resultSetGet (ZebraHandle zh, const char *name);
ZebraSet resultSetAddRPN (ZebraHandle zh, ODR stream, ODR decode,                                         Z_RPNQuery *rpn, int num_bases, char **basenames,
                          const char *setname);
RSET resultSetRef (ZebraHandle zh, Z_ResultSetId *resultSetId);
void resultSetDestroy (ZebraHandle zh, int num_names, char **names,
		       int *statuses);

ZebraPosSet zebraPosSetCreate (ZebraHandle zh, const char *name,
			       int num, int *positions);
void zebraPosSetDestroy (ZebraHandle zh, ZebraPosSet records, int num);

void resultSetSort (ZebraHandle zh, NMEM nmem,
		    int num_input_setnames, const char **input_setnames,
		    const char *output_setname,
		    Z_SortKeySpecList *sort_sequence, int *sort_status);
void resultSetSortSingle (ZebraHandle zh, NMEM nmem,
			  ZebraSet sset, RSET rset,
			  Z_SortKeySpecList *sort_sequence, int *sort_status);
void resultSetRank (ZebraHandle zh, ZebraSet zebraSet, RSET rset);

void zebra_sort (ZebraHandle zh, ODR stream,
		 int num_input_setnames, const char **input_setnames,
		 const char *output_setname, Z_SortKeySpecList *sort_sequence,
		 int *sort_status);

int zebra_server_lock_init (ZebraService zh);
int zebra_server_lock_destroy (ZebraService zh);
int zebra_server_lock (ZebraService zh, int lockCommit);
void zebra_server_unlock (ZebraService zh, int commitPhase);
int zebra_server_lock_get_state (ZebraService zh, time_t *timep);

typedef struct attent
{
    int attset_ordinal;
    data1_local_attribute *local_attributes;
} attent;

void zebraRankInstall (ZebraService zh, struct rank_control *ctrl);
ZebraRankClass zebraRankLookup (ZebraHandle zh, const char *name);
void zebraRankDestroy (ZebraService zh);

int att_getentbyatt(ZebraHandle zh, attent *res, oid_value set, int att);

extern struct rank_control *rank1_class;

int zebra_record_fetch (ZebraHandle zh, int sysno, int score, ODR stream,
			oid_value input_format, Z_RecordComposition *comp,
			oid_value *output_format, char **rec_bufp,
			int *rec_lenp, char **basenamep);

YAZ_END_CDECL
