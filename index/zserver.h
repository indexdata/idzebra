/*
 * Copyright (C) 1994-2002, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: zserver.h,v 1.53 2002-03-20 20:24:30 adam Exp $
 */

#if HAVE_SYS_TIMES_H
#include <sys/times.h>
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
    char *term;
    char *db;
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
    ISAMD isamd;
#endif
    Dict dict;
    Dict matchDict;
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
    data1_handle dh;
    ZebraMaps zebra_maps;
    ZebraRankClass rank_classes;
    RecTypes recTypes;
    Passwd_db passwd_db;
    Zebra_mutex_cond session_lock;
    int stop_flag;
    int active; /* 0=shutdown, 1=enabled and inactive, 2=activated */
};

struct recKeys {
    int buf_used;
    int buf_max;
    char *buf;
    char prevAttrSet;
    short prevAttrUse;
    int prevSeqNo;
};

struct sortKey {
    char *string;
    int length;
    int attrSet;
    int attrUse;
    struct sortKey *next;
};

struct zebra_session {
    struct zebra_session *next;
    struct zebra_service *service;

    struct recKeys keys;
    struct sortKey *sortKeys;

    char **key_buf;
    size_t ptr_top;
    size_t ptr_i;
    size_t key_buf_used;
    int key_file_no;
    char *admin_databaseName;

    ZebraLockHandle lock_normal;
    ZebraLockHandle lock_shadow;

    int trans_no;
    int seqno;
    int last_val;
    int destroyed;
    ZebraSet sets;
    int errCode;
    int hits;
    char *errString;
#if HAVE_SYS_TIMES_H
    struct tms tms1;
    struct tms tms2;    
#endif
    struct recordGroup rGroup;
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

struct term_set_entry {
    char *term;
    struct term_set_entry *next;
};

struct term_set_list {
    struct term_set_entry *first;
    struct term_set_entry *last;
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

void resultSetAddTerm (ZebraHandle zh, ZebraSet s, int reg_type,
		       const char *db, int set,
		       int use, const char *term);
ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov);
ZebraSet resultSetGet (ZebraHandle zh, const char *name);
ZebraSet resultSetAddRPN (ZebraHandle zh, ODR stream, ODR decode,
                          Z_RPNQuery *rpn, int num_bases, char **basenames,
                          const char *setname);
RSET resultSetRef (ZebraHandle zh, Z_ResultSetId *resultSetId);
void resultSetDestroy (ZebraHandle zh, int num_names, char **names,
		       int *statuses);

const char *zebra_resultSetTerms (ZebraHandle zh, const char *setname, 
                                  int no, int *count, int *no_max);

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
void resultSetInvalidate (ZebraHandle zh);

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

void extract_get_fname_tmp (ZebraHandle zh, char *fname, int no);
void zebra_index_merge (ZebraHandle zh);


int extract_rec_in_mem (ZebraHandle zh, const char *recordType,
                        const char *buf, size_t buf_size,
                        const char *databaseName, int delete_flag,
                        int test_mode, int *sysno,
                        int store_keys, int store_data,
                        const char *match_criteria);

void extract_flushWriteKeys (ZebraHandle zh);

struct zebra_fetch_control {
    off_t offset_end;
    off_t record_offset;
    off_t record_int_pos;
    const char *record_int_buf;
    int record_int_len;
    int fd;
};

int zebra_record_ext_read (void *fh, char *buf, size_t count);
off_t zebra_record_ext_seek (void *fh, off_t offset);
off_t zebra_record_ext_tell (void *fh);
off_t zebra_record_int_seek (void *fh, off_t offset);
off_t zebra_record_int_tell (void *fh);
int zebra_record_int_read (void *fh, char *buf, size_t count);
void zebra_record_int_end (void *fh, off_t offset);

void extract_flushRecordKeys (ZebraHandle zh, SYSNO sysno,
                              int cmd, struct recKeys *reckeys);
void extract_flushSortKeys (ZebraHandle zh, SYSNO sysno,
                            int cmd, struct sortKey **skp);
void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid);
void extract_token_add (RecWord *p);
int explain_extract (void *handle, Record rec, data1_node *n);

YAZ_END_CDECL
