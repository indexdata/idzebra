/*
 * Copyright (C) 1995-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 * $Id: index.h,v 1.82 2002-04-16 22:31:42 adam Exp $
 */

#ifndef INDEX_H
#define INDEX_H

#include <time.h>
#include <zebraver.h>
#include <zebrautl.h>
#include <zebramap.h>
#include <sortidx.h>

#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#include <dict.h>
#include <isams.h>
#include <isam.h>
#include <isamc.h>
#include <isamd.h>
#include <isamb.h>
#define ISAM_DEFAULT "c"
#include <yaz/data1.h>
#include <recctrl.h>
#include "recindex.h"
#include "zebraapi.h"
#include "zinfo.h"
#include <passwddb.h>
#include <rset.h>

YAZ_BEGIN_CDECL

#define SU_SCHEME 1

#define IT_MAX_WORD 256
#define IT_KEY_HAVE_SEQNO 1
#define IT_KEY_HAVE_FIELD 0

typedef int SYSNO;

struct it_key {
    int  sysno;
    int  seqno;
};

enum dirsKind { dirs_dir, dirs_file };

struct dir_entry {
    enum dirsKind kind;
    char *name;
    time_t mtime;
};

struct dirs_entry {
    enum dirsKind kind;
    char path[256];
    SYSNO sysno;
    time_t mtime;
};

void getFnameTmp (Res res, char *fname, int no);
        
struct dirs_info *dirs_open (Dict dict, const char *rep, int rw);
struct dirs_info *dirs_fopen (Dict dict, const char *path);
struct dirs_entry *dirs_read (struct dirs_info *p);
struct dirs_entry *dirs_last (struct dirs_info *p);
void dirs_mkdir (struct dirs_info *p, const char *src, time_t mtime);
void dirs_rmdir (struct dirs_info *p, const char *src);
void dirs_add (struct dirs_info *p, const char *src, int sysno, time_t mtime);
void dirs_del (struct dirs_info *p, const char *src);
void dirs_free (struct dirs_info **pp);

struct dir_entry *dir_open (const char *rep, const char *base);
void dir_sort (struct dir_entry *e);
void dir_free (struct dir_entry **e_p);

void repositoryUpdate (ZebraHandle zh);
void repositoryAdd (ZebraHandle zh);
void repositoryDelete (ZebraHandle zh);
void repositoryShow (ZebraHandle zh);

int key_open (ZebraHandle zh, int mem);
int key_close (ZebraHandle zh);
int key_compare (const void *p1, const void *p2);
char *key_print_it (const void *p, char *buf);
int key_get_pos (const void *p);
int key_compare_it (const void *p1, const void *p2);
int key_qsort_compare (const void *p1, const void *p2);
void key_logdump (int mask, const void *p);
void inv_prstat (ZebraHandle zh);
void inv_compact (BFiles bfs);
void key_input (ZebraHandle zh, int nkeys, int cache, Res res);
ISAMS_M key_isams_m (Res res, ISAMS_M me);
ISAMC_M key_isamc_m (Res res, ISAMC_M me);
ISAMD_M key_isamd_m (Res res, ISAMD_M me);
int merge_sort (char **buf, int from, int to);
int key_SU_code (int ch, char *out);

#define FNAME_DICT "dict"
#define FNAME_ISAM "isam"
#define FNAME_ISAMC "isamc"
#define FNAME_ISAMS "isams"
#define FNAME_ISAMH "isamh"
#define FNAME_ISAMD "isamd"
#define FNAME_CONFIG "zebra.cfg"

#define GMATCH_DICT "gmatch"
#define FMATCH_DICT "fmatch"

struct strtab *strtab_mk (void);
int strtab_src (struct strtab *t, const char *name, void ***infop);
void strtab_del (struct strtab *t,
                 void (*func)(const char *name, void *info, void *data),
                 void *data);
int index_char_cvt (int c);
int index_word_prefix (char *string, int attset_ordinal,
                       int local_attribute, const char *databaseName);


void zebraIndexLockMsg (ZebraHandle zh, const char *str);
void zebraIndexUnlock (ZebraHandle zh);
int zebraIndexLock (BFiles bfs, ZebraHandle zh, int commitNow, const char *rval);
int zebraIndexWait (ZebraHandle zh, int commitPhase);

#define FNAME_MAIN_LOCK   "zebraidx.LCK"
#define FNAME_COMMIT_LOCK "zebracmt.LCK"
#define FNAME_ORG_LOCK    "zebraorg.LCK"
#define FNAME_TOUCH_TIME  "zebraidx.time"

typedef struct zebra_lock_info *ZebraLockHandle;
ZebraLockHandle zebra_lock_create(const char *dir,
                                  const char *file, int excl_flag);
void zebra_lock_destroy (ZebraLockHandle h);
int zebra_lock (ZebraLockHandle h);
int zebra_lock_nb (ZebraLockHandle h);
int zebra_unlock (ZebraLockHandle h);
int zebra_lock_fd (ZebraLockHandle h);
void zebra_lock_prefix (Res res, char *dst);
char *zebra_mk_fname (const char *dir, const char *name);

int zebra_lock_w (ZebraLockHandle h);
int zebra_lock_r (ZebraLockHandle h);

void zebra_load_atts (data1_handle dh, Res res);

int key_SU_decode (int *ch, const unsigned char *out);
int key_SU_encode (int ch, char *out);

// extern Res common_resource;

struct encode_info {
    int  sysno;
    int  seqno;
    int  cmd;
    char buf[768];
};

void encode_key_init (struct encode_info *i);
char *encode_key_int (int d, char *bp);
void encode_key_write (char *k, struct encode_info *i, FILE *outf);

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

struct zebra_register {
    char *name;
    
    ISAMS isams;
    ISAM isam;
    ISAMC isamc;
    ISAMD isamd;
    ISAMB isamb;
    Dict dict;
    Dict matchDict;
    SortIdx sortIdx;
    int registerState; /* 0 (no commit pages), 1 (use commit pages) */
    time_t registerChange;
    BFiles bfs;
    Records records;
    ZebraExplainInfo zei;

    char *server_path_prefix;
    data1_handle dh;
    ZebraMaps zebra_maps;
    ZebraRankClass rank_classes;
    RecTypes recTypes;
    int seqno;
    int last_val;
    int stop_flag;
    int active; /* 0=shutdown, 1=enabled and inactive, 2=activated */



    struct recKeys keys;
    struct sortKey *sortKeys;

    char **key_buf;
    size_t ptr_top;
    size_t ptr_i;
    size_t key_buf_used;
    int key_file_no;
};

struct zebra_service {
    int stop_flag;
    Res global_res;
    char *configName;
    struct zebra_session *sessions;
    struct zebra_register *regs;
    Zebra_mutex_cond session_lock;
    Passwd_db passwd_db;
    char *path_root;
};


struct zebra_session {
    struct zebra_session *next;
    struct zebra_service *service;
    struct zebra_register *reg;

    char *admin_databaseName;

    char **basenames;
    int num_basenames;
    char *reg_name;
    char *path_reg;

    ZebraLockHandle lock_normal;
    ZebraLockHandle lock_shadow;

    int trans_no;
    int destroyed;
    ZebraSet sets;
    Res res;
    int errCode;
    int hits;
    char *errString;
#if HAVE_SYS_TIMES_H
    struct tms tms1;
    struct tms tms2;    
#endif
    struct recordGroup rGroup;
    int  shadow_enable;

    int records_inserted;
    int records_updated;
    int records_deleted;
    int records_processed;

};

struct rank_control {
    char *name;
    void *(*create)(struct zebra_register *reg);
    void (*destroy)(struct zebra_register *reg, void *class_handle);
    void *(*begin)(struct zebra_register *reg, void *class_handle, RSET rset);
    void (*end)(struct zebra_register *reg, void *set_handle);
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
		 const char *term, int length_term, const char *flags,
                 int preserve_position);

void resultSetAddTerm (ZebraHandle zh, ZebraSet s, int reg_type,
		       const char *db, int set,
		       int use, const char *term);
ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov);
ZebraSet resultSetGet (ZebraHandle zh, const char *name);
ZebraSet resultSetAddRPN (ZebraHandle zh, ODR stream, ODR decode,
                          Z_RPNQuery *rpn, int num_bases,
                          char **basenames, const char *setname);
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
void resultSetInvalidate (ZebraHandle zh);

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

void zebraRankInstall (struct zebra_register *reg, struct rank_control *ctrl);
ZebraRankClass zebraRankLookup (ZebraHandle zh, const char *name);
void zebraRankDestroy (struct zebra_register *reg);

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

int fileExtract (ZebraHandle zh, SYSNO *sysno, const char *fname,
                 const struct recordGroup *rGroup, int deleteFlag);

YAZ_END_CDECL

#endif
