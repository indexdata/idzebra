/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

/** \file bfile.h
    \brief Zebra Block File Layer

    Providers an interface to a file system , AKA persistent storage.
    The interface allows safe updates - using a shadow file facility.
*/

#ifndef BFILE_H
#define BFILE_H

#include <yaz/yconfig.h>
#include <idzebra/util.h>

YAZ_BEGIN_CDECL

/** \var BFiles
    \brief A collection of BFile(s).
*/
typedef struct BFiles_struct *BFiles;

/** \var BFile
    \brief A Block File
*/
typedef struct BFile_struct *BFile;

/** \brief creates a Block files collection
    \param spec register specification , e.g. "d1:100M d2:1G"
    \param base base directory for register spec (if that is relative path)
    \return block files handle
*/
BFiles bfs_create (const char *spec, const char *base);

/** \brief destroys a block files handle
    \param bfiles block files handle

    The files in the block files collection are not deleted. Only the
    handle is freed.
*/
void bfs_destroy (BFiles bfiles);

/** \brief closes a Block file (may call exit)
    \param bf block file
 */
YAZ_EXPORT
void bf_close(BFile bf);

/** \brief closes a Block file
    \param bf block file
    \retval 0 success
    \retval -1 failure
 */
YAZ_EXPORT
int bf_close2(BFile bf);

/** \brief opens and returns a Block file handle
    \param bfs block files
    \param name filename
    \param block_size block size in bytes
    \param wflag 1=opened for read&write, 0=read only
    \retval 0 success
    \retval -1 failure (can never happen as the code is now)
*/
YAZ_EXPORT
BFile bf_open(BFiles bfs, const char *name, int block_size, int wflag);


/** \brief read from block file (may call exit)
    \param bf block file handle
    \param no block no (first block is 0, second is 1..)
    \param offset offset within block to be read
    \param nbytes number of bytes to read (0 for whole block)
    \param buf raw bytes with content (at least nbytes of size)
    \retval 1 whole block could be read
    \retval 0 whole block could not be read
 */
YAZ_EXPORT
int bf_read(BFile bf, zint no, int offset, int nbytes, void *buf);

/** \brief read from block file
    \param bf block file handle
    \param no block no (first block is 0, second is 1..)
    \param offset offset within block to be read
    \param nbytes number of bytes to read (0 for whole block)
    \param buf raw bytes with content (at least nbytes of size)
    \retval 1 whole block could be read
    \retval 0 whole block could not be read
    \retval -1 error
 */
YAZ_EXPORT
int bf_read2(BFile bf, zint no, int offset, int nbytes, void *buf)
    ZEBRA_GCC_ATTR((warn_unused_result));


/** \brief writes block of bytes to file (may call exit)
    \param bf block file handle
    \param no block no
    \param offset within block
    \param nbytes number of bytes to write
    \param buf buffer to write
    \retval 0 success (block could be written)

    This function can not return a failure. System calls exit(1)
    if write failed.
 */
YAZ_EXPORT
int bf_write(BFile bf, zint no, int offset, int nbytes, const void *buf);


/** \brief writes block of bytes to file
    \param bf block file handle
    \param no block no
    \param offset within block
    \param nbytes number of bytes to write
    \param buf buffer to write
    \retval 0 success (block written)
    \retval -1 error

    This function can not return a failure. System calls exit(1)
    if write failed.
 */
YAZ_EXPORT
int bf_write2(BFile bf, zint no, int offset, int nbytes, const void *buf)
    ZEBRA_GCC_ATTR((warn_unused_result));

/** \brief enables or disables shadow for block files
    \param bfs block files
    \param spec  such as  "shadow:100M /other:200M"; or NULL to disable
    \retval ZEBRA_OK successful. spec is OK
    \retval ZEBRA_FAIL failure.
*/
YAZ_EXPORT
ZEBRA_RES bf_cache (BFiles bfs, const char *spec);

/** \brief Check if there is content in shadow area (to be committed).
    \param bfs block files
    \retval 1 there is content in shadow area
    \retval 0 no content in shadow area
*/
YAZ_EXPORT
int bf_commitExists (BFiles bfs);

/** \brief Executes commit operation
    \param bfs block files
*/
YAZ_EXPORT
int bf_commitExec (BFiles bfs) ZEBRA_GCC_ATTR((warn_unused_result));

/** \brief Cleans shadow files (remove them)
    \param bfs block files
    \param spec shadow specification
*/
YAZ_EXPORT
void bf_commitClean (BFiles bfs, const char *spec);

/** \brief Removes register and shadow completely
    \param bfs block files
*/
YAZ_EXPORT
void bf_reset (BFiles bfs);

/* \brief gets statistics about directory in register area
   \param bfs block files
   \param no directory number (0=first, 1=second,...)
   \param directory holds directory name (if found)
   \param used_bytes used file bytes in directory (if found)
   \param max_bytes max usage of bytes (if found)
   \retval 1 no is within range and directory, used, max are set.
   \retval 0 no is out of range and directory, used, max are unset

   We are using double, because off_t may have a different size
   on same platform depending on whether 64-bit is enabled or not.
   Note that if a register area has unlimited size, that is represented
   as max_bytes = -1.

*/
YAZ_EXPORT
int bfs_register_directory_stat(BFiles bfs, int no, const char **directory,
                                double *used_bytes, double *max_bytes);

/* \brief gets statistics about directory in shadow area
   \param bfs block files
   \param no directory number (0=first, 1=second,...)
   \param directory holds directory name (if found)
   \param used_bytes used file bytes in directory (if found)
   \param max_bytes max usage of bytes (if found)
   \retval 1 no is within range and directory, used, max are set.
   \retval 0 no is out of range and directory, used, max are unset

   We are using double, because off_t may have a different size
   on same platform depending on whether 64-bit is enabled or not.
   Note that if a shadow area has unlimited size, that is represented
   as max_bytes = -1.
*/
YAZ_EXPORT
int bfs_shadow_directory_stat(BFiles bfs, int no, const char **directory,
                              double *used_bytes, double *max_bytes);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

