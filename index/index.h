/*
 * Copyright (C) 1995-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 * $Id: index.h,v 1.74 2002-03-21 10:25:42 adam Exp $
 */

#ifndef INDEX_H
#define INDEX_H

#include <time.h>
#include <zebraver.h>
#include <zebrautl.h>
#include <zebramap.h>

#include <dict.h>
#include <isams.h>
#if ZMBOL
#include <isam.h>
#include <isamc.h>
#include <isamd.h>
#define ISAM_DEFAULT "c"
#else
#define ISAM_DEFAULT "s"
#endif
#include <yaz/data1.h>
#include <recctrl.h>
#include "zebraapi.h"

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

struct recordGroup {
    char         *groupName;
    char         *databaseName;
    char         *path;
    char         *recordId;
    char         *recordType;
    int          flagStoreData;
    int          flagStoreKeys;
    int          flagRw;
    int          fileVerboseLimit;
    int          databaseNamePath;
    int          explainDatabase;
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

struct dir_entry *dir_open (const char *rep);
void dir_sort (struct dir_entry *e);
void dir_free (struct dir_entry **e_p);

void repositoryUpdate (ZebraHandle zh);
void repositoryAdd (ZebraHandle zh);
void repositoryDelete (ZebraHandle zh);
void repositoryShow (ZebraHandle zh);

int key_open (ZebraHandle zh, int mem);
int key_close (ZebraHandle zh);
int key_compare (const void *p1, const void *p2);
int key_get_pos (const void *p);
int key_compare_it (const void *p1, const void *p2);
int key_qsort_compare (const void *p1, const void *p2);
void key_logdump (int mask, const void *p);
void inv_prstat (ZebraHandle zh);
void inv_compact (BFiles bfs);
void key_input (ZebraHandle zh, int nkeys, int cache, Res res);
ISAMS_M key_isams_m (Res res, ISAMS_M me);
#if ZMBOL
ISAMC_M key_isamc_m (Res res, ISAMC_M me);
ISAMD_M key_isamd_m (Res res, ISAMD_M me);
#endif
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

int fileExtract (ZebraHandle zh, SYSNO *sysno, const char *fname,
                 const struct recordGroup *rGroup, int deleteFlag);

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


YAZ_END_CDECL

#endif
