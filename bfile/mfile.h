/* $Id: mfile.h,v 1.11 2006-12-03 16:05:13 adam Exp $
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
#include <idzebra/util.h>

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

/** \brief creates a metafile area
    \param name of area (does not show up on disk - purely for notation)
    \param spec area specification (e.g. "/a:1G dir /b:2000M"
    \param base base directory (NULL for no base)
    \param only_shadow_files only consider shadow files in area
    \returns metafile area handle or NULL if error occurs
*/
MFile_area mf_init(const char *name, const char *spec, const char *base,
                   int only_shadow_files)
    ZEBRA_GCC_ATTR((warn_unused_result));
    
/** \brief destroys metafile area handle
    \param ma metafile area handle
*/
void mf_destroy(MFile_area ma);

/** \brief opens metafile
    \param ma metafile area handle
    \param name pseudo filename (name*.mf)
    \param block_size block size for this file
    \param wflag write flag, 0=read, 1=write&read
    \returns metafile handle, or NULL for error (could not be opened)
 */
MFile mf_open(MFile_area ma, const char *name, int block_size, int wflag)
    ZEBRA_GCC_ATTR((warn_unused_result));
    
/** \brief closes metafile
    \param mf metafile handle
    \retval 0 OK
*/
int mf_close(MFile mf);

/** \brief reads block from metafile
    \param mf metafile handle
    \param no block position
    \param offset offset within block
    \param nbytes no of bytes to read (0 for whole block)
    \param buf content (filled with data if OK)
    \retval 0 block partially read
    \retval 1 block fully read
    \retval -1 block could not be read due to error
 */
int mf_read(MFile mf, zint no, int offset, int nbytes, void *buf)
    ZEBRA_GCC_ATTR((warn_unused_result));
    
/** \brief writes block to metafile
    \param mf metafile handle
    \param no block position
    \param offset offset within block
    \param nbytes no of bytes to write (0 for whole block)
    \param buf content to be written
    \retval 0 block written
    \retval -1 error (block not written)
*/
int mf_write(MFile mf, zint no, int offset, int nbytes, const void *buf)
    ZEBRA_GCC_ATTR((warn_unused_result));    
    
/** \brief reset all files in a metafile area (optionally delete them as well)
    \param ma metafile area
    \param unlink_flag if unlink_flag=1 all files are removed from FS
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

