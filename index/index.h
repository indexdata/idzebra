/* $Id: index.h,v 1.104 2004-01-22 15:40:25 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
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
#include <sys/stat.h>

#include <dict.h>
#include <isams.h>
#include <isam.h>
#include <isamc.h>
#include <isamd.h>
#include <isamb.h>
#define ISAM_DEFAULT "c"
#include <data1.h>
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

struct dir_entry *dir_open (const char *rep, const char *base,
                            int follow_links);
void dir_sort (struct dir_entry *e);
void dir_free (struct dir_entry **e_p);

void repositoryUpdate (ZebraHandle zh, const char *path);
void repositoryAdd (ZebraHandle zh, const char *path);
void repositoryDelete (ZebraHandle zh, const char *path);
void repositoryShow (ZebraHandle zh, const char *path);

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
ISAMS_M *key_isams_m (Res res, ISAMS_M *me);
ISAMC_M *key_isamc_m (Res res, ISAMC_M *me);
ISAMD_M *key_isamd_m (Res res, ISAMD_M *me);
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

#define ENCODE_BUFLEN 768
struct encode_info {
    int  sysno;  /* previously written values for delta-compress */
    int  seqno;
    int  cmd;
    int prevsys; /* buffer for skipping insert/delete pairs */
    int prevseq;
    int prevcmd;
    int keylen; /* tells if we have an unwritten key in buf, and how long*/
    char buf[ENCODE_BUFLEN];
};

void encode_key_init (struct encode_info *i);
char *encode_key_int (int d, char *bp);
void encode_key_write (char *k, struct encode_info *i, FILE *outf);
void encode_key_flush (struct encode_info *i, FILE *outf);

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

#if 1
struct sortKeys {
    int buf_used;
    int buf_max;
    char *buf;
};
#else
struct sortKey {
    char *string;
    int length;
    int attrSet;
    int attrUse;
    struct sortKey *next;
};
#endif

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
#if 1
    struct sortKeys sortKeys;
#else
    struct sortKey *sortKeys;
#endif
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
    const char *path_root;
};


struct zebra_session {
    struct zebra_session *next;
    struct zebra_service *service;
    struct zebra_register *reg;

    char *xadmin_databaseName;

    char **basenames;
    int num_basenames;
    char *reg_name;
    char *path_reg;

    ZebraLockHandle lock_normal;
    ZebraLockHandle lock_shadow;

    int trans_no;
    int trans_w_no;

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
    int  shadow_enable;

    int records_inserted;
    int records_updated;
    int records_deleted;
    int records_processed;
    char *record_encoding;

    yaz_iconv_t iconv_to_utf8;
    yaz_iconv_t iconv_from_utf8;

    int m_follow_links;
    const char *m_group;
    const char *m_record_id;
    const char *m_record_type;
    int m_store_data;
    int m_store_keys;
    int m_explain_database;
    int m_flag_rw;
    int m_file_verbose_limit;
};

struct rank_control {
    char *name;
    void *(*create)(ZebraHandle zh);
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
	       int *is_partial, RSET limit_set, int return_zero);

RSET rset_trunc (ZebraHandle zh, ISAMS_P *isam_p, int no,
		 const char *term, int length_term, const char *flags,
                 int preserve_position, int term_type);

void resultSetAddTerm (ZebraHandle zh, ZebraSet s, int reg_type,
		       const char *db, int set,
		       int use, const char *term);
ZebraSet resultSetAdd (ZebraHandle zh, const char *name, int ov);
ZebraSet resultSetGet (ZebraHandle zh, const char *name);
ZebraSet resultSetAddRPN (ZebraHandle zh, NMEM m, Z_RPNQuery *rpn,
                          int num_bases, char **basenames,
                          const char *setname);
RSET resultSetRef (ZebraHandle zh, const char *resultSetId);
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
extern struct rank_control *rankzv_class;
extern struct rank_control *rankliv_class;

int zebra_record_fetch (ZebraHandle zh, int sysno, int score, ODR stream,
			oid_value input_format, Z_RecordComposition *comp,
			oid_value *output_format, char **rec_bufp,
			int *rec_lenp, char **basenamep);

void extract_get_fname_tmp (ZebraHandle zh, char *fname, int no);

void zebra_index_merge (ZebraHandle zh);

int buffer_extract_record (ZebraHandle zh, 
			   const char *buf, size_t buf_size,
			   int delete_flag,
			   int test_mode, 
			   const char *recordType,
			   int *sysno,
			   const char *match_criteria,
			   const char *fname,
			   int force_update,
			   int allow_update);

#if 0
int extract_rec_in_mem (ZebraHandle zh, const char *recordType,
                        const char *buf, size_t buf_size,
                        const char *databaseName, int delete_flag,
                        int test_mode, int *sysno,
                        int store_keys, int store_data,
                        const char *match_criteria);
#endif
void extract_flushWriteKeys (ZebraHandle zh, int final);

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
                            int cmd, struct sortKeys *skp);
void extract_schema_add (struct recExtractCtrl *p, Odr_oid *oid);
void extract_token_add (RecWord *p);
int explain_extract (void *handle, Record rec, data1_node *n);

int fileExtract (ZebraHandle zh, SYSNO *sysno, const char *fname,
		 int deleteFlag);

int zebra_begin_read (ZebraHandle zh);
int zebra_end_read (ZebraHandle zh);

int zebra_file_stat (const char *file_name, struct stat *buf,
                     int follow_links);

void zebra_livcode_transform(ZebraHandle zh, Z_RPNQuery *query);

YAZ_END_CDECL

#endif
