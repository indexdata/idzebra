/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: mfile.h,v 1.16.2.1 2002-07-23 12:33:22 adam Exp $
 */

#ifndef MFILE_H
#define MFILE_H

#include <stdio.h>
#include <yaz/yconfig.h>

#ifdef WIN32
typedef __int64 mfile_off_t;
#else
#include <sys/types.h>
typedef off_t mfile_off_t;
#endif

#ifndef FILENAME_MAX
#include <sys/param.h>
#define FILENAME_MAX MAXPATHLEN
#endif

#include <zebra-lock.h>

YAZ_BEGIN_CDECL

#define MF_MIN_BLOCKS_CREAT 1          /* minimum free blocks in new dir */
#define MF_MAX_PARTS 28                 /* max # of part-files per metafile */

#define mf_blocksize(mf) ((mf)->blocksize)


typedef struct mf_dir
{
    char name[FILENAME_MAX+1];
    mfile_off_t max_bytes;      /* allocated bytes in this dir. */
    mfile_off_t avail_bytes;    /* bytes left */
    struct mf_dir *next;
} mf_dir;

typedef struct part_file
{
    int number;
    int top;
    int blocks;
    mfile_off_t bytes;
    mf_dir *dir;
    char *path;
    int fd;
} part_file;

struct MFile_area_struct;
typedef struct MFile_area_struct *MFile_area;

typedef struct meta_file
{
    char name[FILENAME_MAX+1];
    part_file files[MF_MAX_PARTS];
    int no_files;
    int cur_file;
    int open;                          /* is this file open? */
    int blocksize;
    mfile_off_t min_bytes_creat;  /* minimum bytes required to enter directory */
    MFile_area ma;
    int wr;
    Zebra_mutex mutex;

    struct meta_file *next;
} *MFile, meta_file;

typedef struct MFile_area_struct
{
    char name[FILENAME_MAX+1];
    mf_dir *dirs;
    struct meta_file *mfiles;
    struct MFile_area_struct *next;  /* global list of active areas */
    Zebra_mutex mutex;
} MFile_area_struct;

/*
 * Open an area, cotaining metafiles in directories.
 */
MFile_area mf_init(const char *name, const char *spec); 

/*
 * Release an area.
 */
void mf_destroy(MFile_area ma);

/*
 * Open a metafile.
 */
MFile mf_open(MFile_area ma, const char *name, int block_size, int wflag);

/*
 * Close a metafile.
 */
int mf_close(MFile mf);

/*
 * Read one block from a metafile. Interface mirrors bfile.
 */
int mf_read(MFile mf, int no, int offset, int nbytes, void *buf);

/*
 * Same.
 */
int mf_write(MFile mf, int no, int offset, int nbytes, const void *buf);

/*
 * Destroy a metafile, unlinking component files. File must be open.
 */
int mf_unlink(MFile mf);


/*
 * Destroy all metafiles. No files may be opened.
 */
void mf_reset(MFile_area ma);

/*
 * Unlink the file by name, rather than MFile-handle.
 */
int mf_unlink_name(MFile_area, const char *name);

YAZ_END_CDECL

#endif
