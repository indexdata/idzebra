/*
 * Copyright (C) 1995-0000, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 * (log at the end)
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
#if ZEBRASDR
    int          useSDR;
#endif
    data1_handle dh;
    BFiles       bfs;
    ZebraMaps    zebra_maps;
    RecTypes     recTypes;
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

void repositoryUpdate (struct recordGroup *rGroup);
void repositoryAdd (struct recordGroup *rGroup);
void repositoryDelete (struct recordGroup *rGroup);
void repositoryShow (struct recordGroup *rGroup);

int key_open (struct recordGroup *rGroup, int mem);
int key_close (struct recordGroup *group);
int key_compare (const void *p1, const void *p2);
int key_get_pos (const void *p);
int key_compare_it (const void *p1, const void *p2);
int key_qsort_compare (const void *p1, const void *p2);
void key_logdump (int mask, const void *p);
void inv_prstat (BFiles bfs);
void inv_compact (BFiles bfs);
void key_input (BFiles bfs, int nkeys, int cache, Res res);
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

int fileExtract (SYSNO *sysno, const char *fname,
                 const struct recordGroup *rGroup, int deleteFlag);

void zebraIndexLockMsg (const char *str);
void zebraIndexUnlock (void);
int zebraIndexLock (BFiles bfs, int commitNow, const char *rval);
int zebraIndexWait (int commitPhase);

#define FNAME_MAIN_LOCK   "zebraidx.LCK"
#define FNAME_COMMIT_LOCK "zebracmt.LCK"
#define FNAME_ORG_LOCK    "zebraorg.LCK"
#define FNAME_TOUCH_TIME  "zebraidx.time"

typedef struct zebra_lock_info *ZebraLockHandle;
ZebraLockHandle zebra_lock_create(const char *file, int excl_flag);
void zebra_lock_destroy (ZebraLockHandle h);
int zebra_lock (ZebraLockHandle h);
int zebra_lock_nb (ZebraLockHandle h);
int zebra_unlock (ZebraLockHandle h);
int zebra_lock_fd (ZebraLockHandle h);
void zebra_lock_prefix (Res res, char *dst);

void zebra_load_atts (data1_handle dh, Res res);

extern Res common_resource;

YAZ_END_CDECL

#endif
/*
 * $Log: index.h,v $
 * Revision 1.70  2000-12-05 10:01:44  adam
 * Fixed bug regarding user-defined attribute sets.
 *
 * Revision 1.69  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.68  2000/02/24 11:00:07  adam
 * Fixed bug: indexer would run forever when lock dir was non-existant.
 *
 * Revision 1.67  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.66  1999/07/14 13:21:34  heikki
 * Added isam-d files. Compiles (almost) clean. Doesn't work at all
 *
 * Revision 1.65  1999/07/14 10:59:26  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.64  1999/06/30 15:07:23  heikki
 * Adding isamh stuff
 *
 * Revision 1.63  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.62  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.61  1999/03/09 16:27:49  adam
 * More work on SDRKit integration.
 *
 * Revision 1.60  1998/10/16 08:14:31  adam
 * Updated record control system.
 *
 * Revision 1.59  1998/06/08 14:43:11  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.58  1998/05/20 10:12:16  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.57  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.56  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.55  1997/10/27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.54  1997/09/29 09:08:36  adam
 * Revised locking system to be thread safe for the server.
 *
 * Revision 1.53  1997/09/25 14:54:43  adam
 * WIN32 files lock support.
 *
 * Revision 1.52  1997/09/22 12:39:06  adam
 * Added get_pos method for the ranked result sets.
 *
 * Revision 1.51  1997/09/18 08:59:19  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.50  1997/09/17 12:19:13  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.49  1997/09/05 15:30:08  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.48  1997/02/12 20:39:45  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.47  1996/12/23 15:30:44  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.46  1996/11/08 11:10:19  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.45  1996/10/29 14:09:42  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.44  1996/06/06 12:08:40  quinn
 * Added showRecord function
 *
 * Revision 1.43  1996/06/04  10:18:12  adam
 * Search/scan uses character mapping module.
 *
 * Revision 1.42  1996/06/04  08:20:16  quinn
 * Smallish
 *
 * Revision 1.41  1996/06/04  07:54:55  quinn
 * Added output-map.
 *
 * Revision 1.40  1996/05/31  09:06:58  quinn
 * Work on character-set handling
 *
 * Revision 1.39  1996/05/14  14:04:33  adam
 * In zebraidx, the 'stat' command is improved. Statistics about ISAM/DICT
 * is collected.
 *
 * Revision 1.38  1996/04/12  07:02:23  adam
 * File update of single files.
 *
 * Revision 1.37  1996/03/26 16:01:13  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.36  1996/03/21  14:50:09  adam
 * File update uses modify-time instead of change-time.
 *
 * Revision 1.35  1996/02/12  18:45:36  adam
 * New fileVerboseFlag in record group control.
 *
 * Revision 1.34  1995/12/11  11:43:29  adam
 * Locking based on fcntl instead of flock.
 * Setting commitEnable removed. Command line option -n can be used to
 * prevent commit if commit setting is defined in the configuration file.
 *
 * Revision 1.33  1995/12/08  16:22:53  adam
 * Work on update while servers are running. Three lock files introduced.
 * The servers reload their registers when necessary, but they don't
 * reestablish result sets yet.
 *
 * Revision 1.32  1995/12/07  17:38:46  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.31  1995/12/06  12:41:22  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.30  1995/12/05  11:25:02  adam
 * Include of zebraver.h.
 *
 * Revision 1.29  1995/11/28  09:09:40  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.28  1995/11/27  13:58:53  adam
 * New option -t. storeStore data implemented in server.
 *
 * Revision 1.27  1995/11/25  10:24:06  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.26  1995/11/22  17:19:17  adam
 * Record management uses the bfile system.
 *
 * Revision 1.25  1995/11/21  15:29:12  adam
 * Config file 'base' read by default by both indexer and server.
 *
 * Revision 1.24  1995/11/21  15:01:15  adam
 * New general match criteria implemented.
 * New feature: document groups.
 *
 * Revision 1.23  1995/11/20  16:59:45  adam
 * New update method: the 'old' keys are saved for each records.
 *
 * Revision 1.22  1995/11/20  11:56:26  adam
 * Work on new traversal.
 *
 * Revision 1.21  1995/11/16  15:34:55  adam
 * Uses new record management system in both indexer and server.
 *
 * Revision 1.20  1995/11/15  14:46:18  adam
 * Started work on better record management system.
 *
 * Revision 1.19  1995/10/27  14:00:11  adam
 * Implemented detection of database availability.
 *
 * Revision 1.18  1995/10/17  18:02:08  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.17  1995/10/13  16:01:49  adam
 * Work on relations.
 *
 * Revision 1.16  1995/10/10  12:24:38  adam
 * Temporary sort files are compressed.
 *
 * Revision 1.15  1995/10/04  16:57:19  adam
 * Key input and merge sort in one pass.
 *
 * Revision 1.14  1995/09/29  14:01:40  adam
 * Bug fixes.
 *
 * Revision 1.13  1995/09/28  14:22:56  adam
 * Sort uses smaller temporary files.
 *
 * Revision 1.12  1995/09/28  12:10:32  adam
 * Bug fixes. Field prefix used in queries.
 *
 * Revision 1.11  1995/09/27  12:22:28  adam
 * More work on extract in record control.
 * Field name is not in isam keys but in prefix in dictionary words.
 *
 * Revision 1.10  1995/09/14  07:48:23  adam
 * Record control management.
 *
 * Revision 1.9  1995/09/11  13:09:33  adam
 * More work on relevance feedback.
 *
 * Revision 1.8  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.7  1995/09/06  16:11:16  adam
 * Option: only one word key per file.
 *
 * Revision 1.6  1995/09/05  15:28:39  adam
 * More work on search engine.
 *
 * Revision 1.5  1995/09/04  12:33:42  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.4  1995/09/04  09:10:35  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 * Revision 1.3  1995/09/01  14:06:35  adam
 * Split of work into more files.
 *
 * Revision 1.2  1995/09/01  10:30:24  adam
 * More work on indexing. Not working yet.
 *
 * Revision 1.1  1995/08/31  14:50:24  adam
 * New simple file index tool.
 *
 */
