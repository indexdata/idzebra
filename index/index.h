/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: index.h,v $
 * Revision 1.15  1995-10-04 16:57:19  adam
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

#include <alexutil.h>
#include <dict.h>
#include <isam.h>

#define IT_MAX_WORD 256
#define IT_KEY_HAVE_SEQNO 1
#define IT_KEY_HAVE_FIELD 0

struct it_key {
    int   sysno : 24;
    int   seqno : 16;
};

struct dir_entry {
    char *name;
};

struct dir_entry *dir_open (const char *rep);
void dir_sort (struct dir_entry *e);
void dir_free (struct dir_entry **e_p);
void repository (int cmd, const char *rep, const char *base_path);

void file_extract (int cmd, const char *fname, const char *kname);

void key_open (int mem);
int key_close (void);
void key_write (int cmd, struct it_key *k, const char *str);
int key_compare (const void *p1, const void *p2);
int key_qsort_compare (const void *p1, const void *p2);
void key_logdump (int mask, const void *p);
void key_input (const char *dict_fname, const char *isam_fname, 
                const char *key_fname, int cache);
void key_input2 (const char *dict_fname, const char *isam_fname,
                 int nkeys, int cache);
int merge_sort (char **buf, int from, int to);

#define TEMP_FNAME  "keys%d.tmp"
#define FNAME_WORD_DICT "worddict"
#define FNAME_WORD_ISAM "wordisam"
#define FNAME_FILE_DICT "filedict"
#define FNAME_SYS_IDX "sysidx"
#define SYS_IDX_ENTRY_LEN 120

struct strtab *strtab_mk (void);
int strtab_src (struct strtab *t, const char *name, void ***infop);
void strtab_del (struct strtab *t,
                 void (*func)(const char *name, void *info, void *data),
                 void *data);
int index_char_cvt (int c);
int index_word_prefix (char *string, int attrSet, int attrUse);
