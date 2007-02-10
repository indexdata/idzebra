/* $Id: mfile.c,v 1.75 2007-02-10 18:37:42 adam Exp $
   Copyright (C) 1995-2007
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

#include <sys/types.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <direntz.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <zebra-lock.h>
#include <idzebra/util.h>
#include <yaz/yaz-util.h>
#include "mfile.h"

static int scan_areadef(MFile_area ma, const char *ad, const char *base)
{
    /*
     * If no definition is given, use current directory, unlimited.
     */
    char dirname[FILENAME_MAX+1]; 
    mf_dir **dp = &ma->dirs, *dir = *dp;

    if (!ad)
        ad = ".:-1b";
    for (;;)
    {
        const char *ad0 = ad;
        int i = 0, fact = 1, multi;
	mfile_off_t size = 0;

        while (*ad == ' ' || *ad == '\t')
            ad++;
        if (!*ad)
            break;
        if (!yaz_is_abspath(ad) && base)
        {
            strcpy(dirname, base);
            i = strlen(dirname);
            dirname[i++] = '/';
        }
        while (*ad)
        {
	    if (*ad == ':' && strchr("+-0123456789", ad[1]))
		break;
            if (i < FILENAME_MAX)
                dirname[i++] = *ad;
            ad++;
        }
        dirname[i] = '\0';
        if (*ad++ != ':')
        {
	    yaz_log(YLOG_WARN, "Missing colon after path: %s", ad0);
            return -1;
        }
        if (i == 0)
        {
	    yaz_log(YLOG_WARN, "Empty path: %s", ad0);
            return -1;
        }
        while (*ad == ' ' || *ad == '\t')
            ad++;
        if (*ad == '-')
        {
            fact = -1;
            ad++;
        }
        else if (*ad == '+')
            ad++;
        size = 0;
        if (*ad < '0' || *ad > '9')
        {
	    yaz_log(YLOG_FATAL, "Missing size after path: %s", ad0);
            return -1;
        }
        size = 0;
        while (*ad >= '0' && *ad <= '9')
            size = size*10 + (*ad++ - '0');
        switch (*ad)
	{
	case 'B': case 'b': multi = 1; break;
	case 'K': case 'k': multi = 1024; break;
	case 'M': case 'm': multi = 1048576; break;
	case 'G': case 'g': multi = 1073741824; break;
        case '\0':
            yaz_log(YLOG_FATAL, "Missing unit: %s", ad0);
            return -1;
        default:
            yaz_log(YLOG_FATAL, "Illegal unit: %c in %s", *ad, ad0);
            return -1;
	}
        ad++;
	*dp = dir = (mf_dir *) xmalloc(sizeof(mf_dir));
	dir->next = 0;
	strcpy(dir->name, dirname);
	dir->max_bytes = dir->avail_bytes = fact * size * multi;
	dp = &dir->next;
    }
    return 0;
}

/** \brief position within metafile (perform seek)
    \param mf metafile handle
    \param pos block position
    \param offset offset within block
    \retval 0 OK
    \retval -1 ERROR
    \retval -2 OK, but file does not created (read-only)
*/
   
static zint file_position(MFile mf, zint pos, int offset)
{
    zint off = 0, ps;
    int c = mf->cur_file;

    if ((c > 0 && pos <= mf->files[c-1].top) ||
	(c < mf->no_files -1 && pos > mf->files[c].top))
    {
	c = 0;
	while (c + 1 < mf->no_files && mf->files[c].top < pos)
	{
	    off += mf->files[c].blocks;
	    c++;
	}
	assert(c < mf->no_files);
    }
    else
    	off = c ? (mf->files[c-1].top + 1) : 0;
    if (mf->files[c].fd < 0)
    {
        if ((mf->files[c].fd = open(mf->files[c].path,
	                            mf->wr ?
                                        (O_BINARY|O_RDWR|O_CREAT) :
                                        (O_BINARY|O_RDONLY), 0666)) < 0)
        {
            if (!mf->wr && errno == ENOENT && off == 0)
            {
                /* we can't open it for reading. But not really an error */
                return -2;
            }
    	    yaz_log(YLOG_WARN|YLOG_ERRNO, "Failed to open %s", mf->files[c].path);
	    return -1;
        }
    }
    ps = pos - off;
    if (mfile_seek(mf->files[c].fd, ps *(mfile_off_t) mf->blocksize + offset,
    	SEEK_SET) < 0)
    {
    	yaz_log(YLOG_WARN|YLOG_ERRNO, "Failed to seek in %s", mf->files[c].path);
        yaz_log(YLOG_WARN, "pos=" ZINT_FORMAT " off=" ZINT_FORMAT " blocksize=%d offset=%d",
                       pos, off, mf->blocksize, offset);
    	return -1;
    }
    mf->cur_file = c;
    return ps;
}

static int cmp_part_file(const void *p1, const void *p2)
{
    zint d = ((part_file *)p1)->number - ((part_file *)p2)->number;
    if (d > 0)
        return 1;
    if (d < 0)
	return -1;
    return 0;
}

MFile_area mf_init(const char *name, const char *spec, const char *base,
                   int only_shadow_files)
{
    MFile_area ma = (MFile_area) xmalloc(sizeof(*ma));
    mf_dir *dirp;
    meta_file *meta_f;
    part_file *part_f = 0;
    DIR *dd;
    struct dirent *dent;
    int fd, number;
    char metaname[FILENAME_MAX+1], tmpnam[FILENAME_MAX+1];
    
    yaz_log(YLOG_DEBUG, "mf_init(%s)", name);
    strcpy(ma->name, name);
    ma->mfiles = 0;
    ma->dirs = 0;
    if (scan_areadef(ma, spec, base) < 0)
    {
    	yaz_log(YLOG_WARN, "Failed to access description of '%s'", name);
        mf_destroy(ma);
    	return 0;
    }
    /* look at each directory */
    for (dirp = ma->dirs; dirp; dirp = dirp->next)
    {
    	if (!(dd = opendir(dirp->name)))
    	{
    	    yaz_log(YLOG_WARN|YLOG_ERRNO, "Failed to open directory %s",
                    dirp->name);
            mf_destroy(ma);
    	    return 0;
	}
	/* look at each file */
	while ((dent = readdir(dd)))
	{
	    int len = strlen(dent->d_name);
	    const char *cp = strrchr(dent->d_name, '-');
	    if (strchr(".-", *dent->d_name))
		continue;
	    if (len < 5 || !cp || strcmp(dent->d_name + len - 3, ".mf"))
		continue;
	    number = atoi(cp+1);
	    memcpy(metaname, dent->d_name, cp - dent->d_name);
	    metaname[ cp - dent->d_name] = '\0';

            /* only files such as file-i-0.mf and file-i-b-0.mf, bug #739 */
            if (only_shadow_files && cp[-2] != '-')
                continue;
            if (!only_shadow_files && cp[-2] == '-')
                continue;
	    for (meta_f = ma->mfiles; meta_f; meta_f = meta_f->next)
	    {
	    	/* known metafile */
		if (!strcmp(meta_f->name, metaname))
	    	{
		    part_f = &meta_f->files[meta_f->no_files++];
		    break;
		}
	    }
	    /* new metafile */
	    if (!meta_f)
	    {
	    	meta_f = (meta_file *) xmalloc(sizeof(*meta_f));
        	zebra_mutex_init(&meta_f->mutex);
	    	meta_f->ma = ma;
	    	meta_f->next = ma->mfiles;
	    	meta_f->open = 0;
	    	meta_f->cur_file = -1;
	    	ma->mfiles = meta_f;
	    	strcpy(meta_f->name, metaname);
	    	part_f = &meta_f->files[0];
	    	meta_f->no_files = 1;
	    }
	    part_f->number = number;
	    part_f->dir = dirp;
	    part_f->fd = -1;
	    sprintf(tmpnam, "%s/%s", dirp->name, dent->d_name);
	    part_f->path = xstrdup(tmpnam);
	    /* get size */
	    if ((fd = open(part_f->path, O_BINARY|O_RDONLY)) < 0)
	    {
	    	yaz_log(YLOG_FATAL|YLOG_ERRNO, "Failed to access %s",
                      dent->d_name);
                closedir(dd);
                mf_destroy(ma);
	    	return 0;
	    }
	    if ((part_f->bytes = mfile_seek(fd, 0, SEEK_END)) < 0)
	    {
	    	yaz_log(YLOG_FATAL|YLOG_ERRNO, "Failed to seek in %s",
                      dent->d_name);
                close(fd);
                closedir(dd);
                mf_destroy(ma);
	    	return 0;
	    }
	    close(fd);
	    if (dirp->max_bytes >= 0)
		dirp->avail_bytes -= part_f->bytes;
	}
	closedir(dd);
    }
    for (meta_f = ma->mfiles; meta_f; meta_f = meta_f->next)
    {
    	yaz_log(YLOG_DEBUG, "mf_init: %s consists of %d part(s)", meta_f->name,
              meta_f->no_files);
    	qsort(meta_f->files, meta_f->no_files, sizeof(part_file),
              cmp_part_file);
    }
    return ma;
}

void mf_destroy(MFile_area ma)
{
    mf_dir *dp;

    if (!ma)
	return;
    dp = ma->dirs;
    while (dp)
    {
	mf_dir *d = dp;
	dp = dp->next;
	xfree(d);
    }
    mf_reset(ma, 0);
    xfree(ma);
}

void mf_reset(MFile_area ma, int unlink_flag)
{
    meta_file *meta_f;

    if (!ma)
	return;
    meta_f = ma->mfiles;
    while (meta_f)
    {
	int i;
	meta_file *m = meta_f;

	meta_f = meta_f->next;

	assert(!m->open);
	for (i = 0; i<m->no_files; i++)
	{
            if (unlink_flag)
                unlink(m->files[i].path);
	    xfree(m->files[i].path);
	}
	zebra_mutex_destroy(&m->mutex);
	xfree(m);
    }
    ma->mfiles = 0;
}

MFile mf_open(MFile_area ma, const char *name, int block_size, int wflag)
{
    meta_file *mnew;
    int i;
    char tmp[FILENAME_MAX+1];
    mf_dir *dp;

    yaz_log(YLOG_DEBUG, "mf_open(%s bs=%d, %s)", name, block_size,
         wflag ? "RW" : "RDONLY");
    assert(ma);
    for (mnew = ma->mfiles; mnew; mnew = mnew->next)
    	if (!strcmp(name, mnew->name))
	{
    	    if (mnew->open)
            {
                yaz_log(YLOG_WARN, "metafile %s already open", name);
                return 0;
            }
            break;
	}
    if (!mnew)
    {
    	mnew = (meta_file *) xmalloc(sizeof(*mnew));
    	strcpy(mnew->name, name);
    	/* allocate one, empty file */
	zebra_mutex_init(&mnew->mutex);
    	mnew->no_files = 1;
    	mnew->files[0].bytes = 0;
	mnew->files[0].blocks = 0;
    	mnew->files[0].top = -1;
    	mnew->files[0].number = 0;
    	mnew->files[0].fd = -1;
    	mnew->min_bytes_creat = MF_MIN_BLOCKS_CREAT * block_size;
    	for (dp = ma->dirs; dp && dp->max_bytes >= 0 && dp->avail_bytes <
	    mnew->min_bytes_creat; dp = dp->next);
	if (!dp)
	{
	    yaz_log(YLOG_FATAL, "Insufficient space for file %s", name);
            xfree(mnew);
	    return 0;
	}
	mnew->files[0].dir = dp;
    	sprintf(tmp, "%s/%s-%d.mf", dp->name, mnew->name, 0);
    	mnew->files[0].path = xstrdup(tmp);
    	mnew->ma = ma;
	mnew->next = ma->mfiles;
	ma->mfiles = mnew;
    }
    else
    {
    	for (i = 0; i < mnew->no_files; i++)
    	{
	    if (mnew->files[i].bytes % block_size)
	    	mnew->files[i].bytes += block_size - mnew->files[i].bytes %
		    block_size;
	    mnew->files[i].blocks = (int) (mnew->files[i].bytes / block_size);
	}
    	assert(!mnew->open);
    }
    mnew->blocksize = block_size;
    mnew->min_bytes_creat = MF_MIN_BLOCKS_CREAT * block_size;
    mnew->wr=wflag;
    mnew->cur_file = 0;
    mnew->open = 1;

    for (i = 0; i < mnew->no_files; i++)
    {
    	mnew->files[i].blocks = (int)(mnew->files[i].bytes / mnew->blocksize);
    	if (i == mnew->no_files - 1)
	    mnew->files[i].top = -1;
	else
	    mnew->files[i].top =
		i ? (mnew->files[i-1].top + mnew->files[i].blocks)
	        : (mnew->files[i].blocks - 1);
    }
    return mnew;
}

int mf_close(MFile mf)
{
    int i;

    yaz_log(YLOG_DEBUG, "mf_close(%s)", mf->name);
    assert(mf->open);
    for (i = 0; i < mf->no_files; i++)
    {
    	if (mf->files[i].fd >= 0)
    	{
#ifndef WIN32
            if (mf->wr)
                fsync(mf->files[i].fd);
#endif
    	    close(mf->files[i].fd);
    	    mf->files[i].fd = -1;
	}
    }
    mf->open = 0;
    return 0;
}

int mf_read(MFile mf, zint no, int offset, int nbytes, void *buf)
{
    zint rd;
    int toread;

    zebra_mutex_lock(&mf->mutex);
    if ((rd = file_position(mf, no, offset)) < 0)
    {
        if (rd == -2)
	{
	    zebra_mutex_unlock(&mf->mutex);
            return 0;
	}
        else
        {
            yaz_log(YLOG_FATAL, "mf_read2 %s internal error", mf->name);
            return -1;
        }
    }
    toread = nbytes ? nbytes : mf->blocksize;
    if ((rd = read(mf->files[mf->cur_file].fd, buf, toread)) < 0)
    {
    	yaz_log(YLOG_FATAL|YLOG_ERRNO, "mf_read2: Read failed (%s)",
              mf->files[mf->cur_file].path);
        return -1;
    }
    zebra_mutex_unlock(&mf->mutex);
    if (rd < toread)
    	return 0;
    else
    	return 1;
}

int mf_write(MFile mf, zint no, int offset, int nbytes, const void *buf)
{
    int ret = 0;
    zint ps;
    zint nblocks;
    int towrite;
    mf_dir *dp;
    char tmp[FILENAME_MAX+1];
    unsigned char dummych = '\xff';

    zebra_mutex_lock(&mf->mutex);
    if ((ps = file_position(mf, no, offset)) < 0)
    {
        yaz_log(YLOG_FATAL, "mf_write: %s error (1)", mf->name);
        ret = -1;
        goto out;
    }
    /* file needs to grow */
    while (ps >= mf->files[mf->cur_file].blocks)
    {
        mfile_off_t needed = (ps - mf->files[mf->cur_file].blocks + 1) *
                       mf->blocksize;
    	/* file overflow - allocate new file */
    	if (mf->files[mf->cur_file].dir->max_bytes >= 0 &&
	    needed > mf->files[mf->cur_file].dir->avail_bytes)
	{
	    /* cap off file? */
	    if ((nblocks = (int) (mf->files[mf->cur_file].dir->avail_bytes /
		mf->blocksize)) > 0)
	    {
	    	yaz_log(YLOG_DEBUG, "Capping off file %s at pos " ZINT_FORMAT,
		    mf->files[mf->cur_file].path, nblocks);
	    	if ((ps = file_position(mf,
		    (mf->cur_file ? mf->files[mf->cur_file-1].top : 0) +
		    mf->files[mf->cur_file].blocks + nblocks - 1, 0)) < 0)
                {
                    yaz_log(YLOG_FATAL, "mf_write: %s error (2)",
				 mf->name);
                    ret = -1;
                    goto out;
                }
		yaz_log(YLOG_DEBUG, "ps = " ZINT_FORMAT, ps);
		if (write(mf->files[mf->cur_file].fd, &dummych, 1) < 1)
		{
		    yaz_log(YLOG_ERRNO|YLOG_FATAL, "mf_write: %s error (3)",
				      mf->name);
                    ret = -1;
                    goto out;
		}
		mf->files[mf->cur_file].blocks += nblocks;
		mf->files[mf->cur_file].bytes += nblocks * mf->blocksize;
		mf->files[mf->cur_file].dir->avail_bytes -= nblocks *
		    mf->blocksize;
	    }
	    /* get other bit */
    	    yaz_log(YLOG_DEBUG, "Creating new file.");
    	    for (dp = mf->ma->dirs; dp && dp->max_bytes >= 0 &&
		dp->avail_bytes < needed; dp = dp->next);
	    if (!dp)
	    {
	    	yaz_log(YLOG_FATAL, "mf_write: %s error (4) no more space",
                        mf->name);
                ret = -1;
                goto out;
	    }
	    mf->files[mf->cur_file].top = (mf->cur_file ?
                                           mf->files[mf->cur_file-1].top : -1) +
		mf->files[mf->cur_file].blocks;
	    mf->files[++(mf->cur_file)].top = -1;
	    mf->files[mf->cur_file].dir = dp;
	    mf->files[mf->cur_file].number =
		mf->files[mf->cur_file-1].number + 1;
	    mf->files[mf->cur_file].blocks = 0;
	    mf->files[mf->cur_file].bytes = 0;
	    mf->files[mf->cur_file].fd = -1;
	    sprintf(tmp, "%s/%s-" ZINT_FORMAT ".mf", dp->name, mf->name,
		mf->files[mf->cur_file].number);
	    mf->files[mf->cur_file].path = xstrdup(tmp);
	    mf->no_files++;
	    /* open new file and position at beginning */
	    if ((ps = file_position(mf, no, offset)) < 0)
            {
                yaz_log(YLOG_FATAL, "mf_write: %s error (5)", mf->name);
                ret = -1;
                goto out;
            }	
	}
	else
	{
	    nblocks = ps - mf->files[mf->cur_file].blocks + 1;
	    mf->files[mf->cur_file].blocks += nblocks;
	    mf->files[mf->cur_file].bytes += nblocks * mf->blocksize;
	    if (mf->files[mf->cur_file].dir->max_bytes >= 0)
		mf->files[mf->cur_file].dir->avail_bytes -=
		nblocks * mf->blocksize;
	}
    }
    towrite = nbytes ? nbytes : mf->blocksize;
    if (write(mf->files[mf->cur_file].fd, buf, towrite) < towrite)
    {
    	yaz_log(YLOG_FATAL|YLOG_ERRNO, "Write failed for file %s part %d",
		mf->name, mf->cur_file);
        ret = -1;
    }
 out:
    zebra_mutex_unlock(&mf->mutex);
    return ret;
}

/** \brief metafile area statistics 
    \param ma metafile area handle
    \param no area number (0=first, 1=second, ..)
    \param directory holds directory upon completion (if non-NULL)
    \param used_bytes holds used bytes upon completion (if non-NULL)
    \param max_bytes holds max size bytes upon completion (if non-NULL)
    \retval 0 area number does not exist
    \retval 1 area number exists (and directory,used_bytes,.. are set)
*/
int mf_area_directory_stat(MFile_area ma, int no, const char **directory,
			   double *used_bytes, double *max_bytes)
{
    int i;
    mf_dir *d = ma->dirs;
    for (i = 0; d && i<no; i++, d = d->next)
	;
    if (!d)
	return 0;
    if (directory)
	*directory = d->name;
    if (max_bytes)
    {
        /* possible loss of data. But it's just statistics and lies */
	*max_bytes = (double) d->max_bytes;
    }
    if (used_bytes)
    {
        /* possible loss of data. But it's just statistics and lies */
	*used_bytes = (double) (d->max_bytes - d->avail_bytes);
    }
    return 1;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

