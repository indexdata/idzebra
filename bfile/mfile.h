/* $Id: mfile.h,v 1.8 2006-11-08 22:06:50 adam Exp $
   Copyright (C) 1995-2006
   Index Data ApS

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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef MFILE_H
#define MFILE_H

#include <stdio.h>
#include <yaz/yconfig.h>
#include <idzebra/version.h>

#ifdef WIN32

/* 64-bit access .. */
typedef __int64 mfile_off_t;
#define mfile_seek _lseeki64

#else
#include <sys/types.h>
typedef off_t mfile_off_t;
#define mfile_seek lseek
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
    zint number;
    zint top;
    zint blocks;
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
MFile_area mf_init(const char *name, const char *spec, const char *base); 

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

int mf_read(MFile mf, zint no, int offset, int nbytes, void *buf);

int mf_write(MFile mf, zint no, int offset, int nbytes, const void *buf);

/*
 * Destroy all metafiles. No files may be opened.
 */
void mf_reset(MFile_area ma, int unlink_flag);

/* \brief gets statistics about directory in metafile area
   \param ma the area
   \param no directory number (0=first, 1=second,...)
   \param directory holds directory name (if found)
   \param used_bytes used file bytes in directory (if found)
   \param max_bytes max usage of bytes (if found)
   \retval 1 no is within range and directory, used, max are set.
   \retval 0 no is out of range and directory, used, max are unset

   We are using double, because off_t may have a different size
   on same platform depending on whether 64-bit is enabled or not.
   Note that if an area has unlimited size, that is represented
   as max_bytes = -1.
*/ 
int mf_area_directory_stat(MFile_area ma, int no, const char **directory,
			   double *bytes_used, double *bytes_max);
    
YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

