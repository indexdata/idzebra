/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: mfile.c,v $
 * Revision 1.35  1999-10-14 14:33:50  adam
 * Added truncation 5=106.
 *
 * Revision 1.34  1999/05/26 07:49:12  adam
 * C++ compilation.
 *
 * Revision 1.33  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.32  1999/04/28 14:53:07  adam
 * Fixed stupid bug regarding split-files.
 *
 * Revision 1.31  1999/02/18 12:49:33  adam
 * Changed file naming scheme for register files as well as record
 * store/index files.
 *
 * Revision 1.30  1999/02/02 14:50:02  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.29  1998/05/27 14:28:34  adam
 * Fixed bug in mf_write. 'Cap off' byte written at wrong offset.
 *
 * Revision 1.28  1998/05/20 10:00:35  adam
 * Fixed register spec so that colon isn't treated as size separator
 * unless followed by [0-9+-] in order to allow DOS drive specifications.
 *
 * Revision 1.27  1998/02/10 11:55:07  adam
 * Minor changes.
 *
 * Revision 1.26  1997/10/27 14:25:38  adam
 * Fixed memory leaks.
 *
 * Revision 1.25  1997/09/18 08:59:16  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.24  1997/09/17 12:19:06  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.23  1997/09/09 13:37:53  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.22  1997/09/04 13:56:39  adam
 * Added O_BINARY to open calls.
 *
 * Revision 1.21  1996/10/29 13:56:18  adam
 * Include of zebrautl.h instead of alexutil.h.
 *
 * Revision 1.20  1996/05/14 12:10:16  quinn
 * Bad areadef scan
 *
 * Revision 1.19  1996/05/01  07:16:30  quinn
 * Fixed ancient bug.
 *
 * Revision 1.18  1996/04/09  06:47:30  adam
 * Function scan_areadef doesn't use sscanf (%n fails on this Linux).
 *
 * Revision 1.17  1996/03/20 13:29:11  quinn
 * Bug-fix
 *
 * Revision 1.16  1995/12/12  15:57:57  adam
 * Implemented mf_unlink. cf_unlink uses mf_unlink.
 *
 * Revision 1.15  1995/12/08  16:21:14  adam
 * Work on commit/update.
 *
 * Revision 1.14  1995/12/05  13:12:37  quinn
 * Added <errno.h>
 *
 * Revision 1.13  1995/11/30  17:00:50  adam
 * Several bug fixes. Commit system runs now.
 *
 * Revision 1.12  1995/11/24  17:26:11  quinn
 * Mostly about making some ISAM stuff in the config file optional.
 *
 * Revision 1.11  1995/11/13  09:32:43  quinn
 * Comment work.
 *
 * Revision 1.10  1995/09/04  12:33:22  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.9  1994/11/04  14:26:39  quinn
 * bug-fix.
 *
 * Revision 1.8  1994/10/05  16:56:42  quinn
 * Minor.
 *
 * Revision 1.7  1994/09/19  14:12:37  quinn
 * dunno.
 *
 * Revision 1.6  1994/09/14  13:10:15  quinn
 * Corrected some bugs in the init-phase
 *
 * Revision 1.5  1994/09/12  08:01:51  quinn
 * Small
 *
 * Revision 1.4  1994/09/01  14:51:07  quinn
 * Allowed mf_write to write beyond eof+1.
 *
 * Revision 1.3  1994/08/24  09:37:17  quinn
 * Changed reaction to read return values.
 *
 * Revision 1.2  1994/08/23  14:50:48  quinn
 * Fixed mf_close().
 *
 * Revision 1.1  1994/08/23  14:41:33  quinn
 * First functional version.
 *
 */


 /*
  * TODO: The size estimates in init may not be accurate due to
  * only partially written final blocks.
  */

#include <sys/types.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <direntz.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <zebrautl.h>
#include <mfile.h>

static int scan_areadef(MFile_area ma, const char *name, const char *ad)
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
        int i = 0, fact = 1, multi, size = 0;

        while (*ad == ' ' || *ad == '\t')
            ad++;
        if (!*ad)
            break;
        while (*ad)
        {
	    if (*ad == ':' && strchr ("+-0123456789", ad[1]))
		break;
            if (i < FILENAME_MAX)
                dirname[i++] = *ad;
            ad++;
        }
        dirname[i] = '\0';
        if (*ad++ != ':')
        {
	    logf (LOG_WARN, "Missing colon after path: %s", ad0);
            return -1;
        }
        if (i == 0)
        {
	    logf (LOG_WARN, "Empty path: %s", ad0);
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
	    logf (LOG_FATAL, "Missing size after path: %s", ad0);
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
            case '\0':
	        logf (LOG_FATAL, "Missing unit: %s", ad0);
		return -1;
	    default:
	        logf (LOG_FATAL, "Illegal unit: %c in %s", *ad, ad0);
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

static int file_position(MFile mf, int pos, int offset)
{
    int off = 0, c = mf->cur_file, ps;

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
    if (mf->files[c].fd < 0 && (mf->files[c].fd = open(mf->files[c].path,
	mf->wr ? (O_BINARY|O_RDWR|O_CREAT) : (O_BINARY|O_RDONLY), 0666)) < 0)
    {
        if (!mf->wr && errno == ENOENT && off == 0)
            return -2;
    	logf (LOG_WARN|LOG_ERRNO, "Failed to open %s", mf->files[c].path);
    	return -1;
    }
    if (lseek(mf->files[c].fd, (ps = pos - off) * mf->blocksize + offset,
    	SEEK_SET) < 0)
    {
    	logf (LOG_WARN|LOG_ERRNO, "Failed to seek in %s", mf->files[c].path);
    	return -1;
    }
    mf->cur_file = c;
    return ps;
}

static int cmp_part_file(const void *p1, const void *p2)
{
    return ((part_file *)p1)->number - ((part_file *)p2)->number;
}

/*
 * Create a new area, cotaining metafiles in directories.
 * Find the part-files in each directory, and inventory the existing metafiles.
 */
MFile_area mf_init(const char *name, const char *spec)
{
    MFile_area ma = (MFile_area) xmalloc(sizeof(*ma));
    mf_dir *dirp;
    meta_file *meta_f;
    part_file *part_f = 0;
    DIR *dd;
    struct dirent *dent;
    int fd, number;
    char metaname[FILENAME_MAX+1], tmpnam[FILENAME_MAX+1];
    
    logf (LOG_DEBUG, "mf_init(%s)", name);
    strcpy(ma->name, name);
    ma->mfiles = 0;
    ma->dirs = 0;
    if (scan_areadef(ma, name, spec) < 0)
    {
    	logf (LOG_WARN, "Failed to access description of '%s'", name);
    	return 0;
    }
    /* look at each directory */
    for (dirp = ma->dirs; dirp; dirp = dirp->next)
    {
    	if (!(dd = opendir(dirp->name)))
    	{
    	    logf (LOG_WARN|LOG_ERRNO, "Failed to open directory %s",
                                     dirp->name);
    	    return 0;
	}
	/* look at each file */
	while ((dent = readdir(dd)))
	{
	    if (*dent->d_name == '.')
	    	continue;
	    if (sscanf(dent->d_name, "%[^-]-%d.mf", metaname, &number) != 2)
	    {
	    	logf (LOG_DEBUG, "bf: %s is not a part-file.", dent->d_name);
	    	continue;
	    }
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
	    	logf (LOG_FATAL|LOG_ERRNO, "Failed to access %s",
                      dent->d_name);
	    	return 0;
	    }
	    if ((part_f->bytes = lseek(fd, 0, SEEK_END)) < 0)
	    {
	    	logf (LOG_FATAL|LOG_ERRNO, "Failed to seek in %s",
                      dent->d_name);
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
    	logf (LOG_DEBUG, "mf_init: %s consists of %d part(s)", meta_f->name,
              meta_f->no_files);
    	qsort(meta_f->files, meta_f->no_files, sizeof(part_file),
              cmp_part_file);
    }
    return ma;
}

void mf_destroy(MFile_area ma)
{
    mf_dir *dp;
    meta_file *meta_f;

    if (!ma)
	return;
    dp = ma->dirs;
    while (dp)
    {
	mf_dir *d = dp;
	dp = dp->next;
	xfree (d);
    }
    meta_f = ma->mfiles;
    while (meta_f)
    {
	int i;
	meta_file *m = meta_f;
	
	for (i = 0; i<m->no_files; i++)
	{
	    xfree (m->files[i].path);
	}
	meta_f = meta_f->next;
	xfree (m);
    }
    xfree (ma);
}

/*
 * Open a metafile.
 * If !ma, Use MF_DEFAULT_AREA.
 */
MFile mf_open(MFile_area ma, const char *name, int block_size, int wflag)
{
    meta_file *mnew;
    int i;
    char tmp[FILENAME_MAX+1];
    mf_dir *dp;

    logf(LOG_DEBUG, "mf_open(%s bs=%d, %s)", name, block_size,
         wflag ? "RW" : "RDONLY");
    assert (ma);
    for (mnew = ma->mfiles; mnew; mnew = mnew->next)
    	if (!strcmp(name, mnew->name))
	{
    	    if (mnew->open)
    	        abort();
	    else
	        break;
	}
    if (!mnew)
    {
    	mnew = (meta_file *) xmalloc(sizeof(*mnew));
    	strcpy(mnew->name, name);
    	/* allocate one, empty file */
    	mnew->no_files = 1;
    	mnew->files[0].bytes = mnew->files[0].blocks = 0;
    	mnew->files[0].top = -1;
    	mnew->files[0].number = 0;
    	mnew->files[0].fd = -1;
    	mnew->min_bytes_creat = MF_MIN_BLOCKS_CREAT * block_size;
    	for (dp = ma->dirs; dp && dp->max_bytes >= 0 && dp->avail_bytes <
	    mnew->min_bytes_creat; dp = dp->next);
	if (!dp)
	{
	    logf (LOG_FATAL, "Insufficient space for new mfile.");
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
	    mnew->files[i].blocks = mnew->files[i].bytes / block_size;
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
    	mnew->files[i].blocks = mnew->files[i].bytes / mnew->blocksize;
    	if (i == mnew->no_files - 1)
	    mnew->files[i].top = -1;
	else
	    mnew->files[i].top =
		i ? (mnew->files[i-1].top + mnew->files[i].blocks)
	        : (mnew->files[i].blocks - 1);
    }
    return mnew;
}

/*
 * Close a metafile.
 */
int mf_close(MFile mf)
{
    int i;

    logf (LOG_DEBUG, "mf_close(%s)", mf->name);
    assert(mf->open);
    for (i = 0; i < mf->no_files; i++)
    	if (mf->files[i].fd >= 0)
    	{
    	    close(mf->files[i].fd);
    	    mf->files[i].fd = -1;
	}
    mf->open = 0;
    return 0;
}

/*
 * Read one block from a metafile. Interface mirrors bfile.
 */
int mf_read(MFile mf, int no, int offset, int nbytes, void *buf)
{
    int rd, toread;

    if ((rd = file_position(mf, no, offset)) < 0)
    {
        if (rd == -2)
            return 0;
        else
            exit(1);
    }
    toread = nbytes ? nbytes : mf->blocksize;
    if ((rd = read(mf->files[mf->cur_file].fd, buf, toread)) < 0)
    {
    	logf (LOG_FATAL|LOG_ERRNO, "mf_read: Read failed (%s)",
              mf->files[mf->cur_file].path);
    	exit(1);
    }
    else if (rd < toread)
    	return 0;
    else
    	return 1;
}

/*
 * Write.
 */
int mf_write(MFile mf, int no, int offset, int nbytes, const void *buf)
{
    int ps, nblocks, towrite;
    mf_dir *dp;
    char tmp[FILENAME_MAX+1];
    unsigned char dummych = '\xff';

    if ((ps = file_position(mf, no, offset)) < 0)
	exit(1);
    /* file needs to grow */
    while (ps >= mf->files[mf->cur_file].blocks)
    {
    	/* file overflow - allocate new file */
    	if (mf->files[mf->cur_file].dir->max_bytes >= 0 &&
	    (ps - mf->files[mf->cur_file].blocks + 1) * mf->blocksize >
	    mf->files[mf->cur_file].dir->avail_bytes)
	{
	    /* cap off file? */
	    if ((nblocks = mf->files[mf->cur_file].dir->avail_bytes /
		mf->blocksize) > 0)
	    {
	    	logf (LOG_DEBUG, "Capping off file %s at pos %d",
		    mf->files[mf->cur_file].path, nblocks);
	    	if ((ps = file_position(mf,
		    (mf->cur_file ? mf->files[mf->cur_file-1].top : 0) +
		    mf->files[mf->cur_file].blocks + nblocks - 1, 0)) < 0)
			exit(1);
		logf (LOG_DEBUG, "ps = %d", ps);
		if (write(mf->files[mf->cur_file].fd, &dummych, 1) < 1)
		{
		    logf (LOG_ERRNO|LOG_FATAL, "write dummy");
		    exit(1);
		}
		mf->files[mf->cur_file].blocks += nblocks;
		mf->files[mf->cur_file].bytes += nblocks * mf->blocksize;
		mf->files[mf->cur_file].dir->avail_bytes -= nblocks *
		    mf->blocksize;
	    }
	    /* get other bit */
    	    logf (LOG_DEBUG, "Creating new file.");
    	    for (dp = mf->ma->dirs; dp && dp->max_bytes >= 0 &&
		dp->avail_bytes < mf->min_bytes_creat; dp = dp->next);
	    if (!dp)
	    {
	    	logf (LOG_FATAL, "Cannot allocate more space for %s",
                      mf->name);
	    	exit(1);
	    }
	    mf->files[mf->cur_file].top = (mf->cur_file ?
	    	mf->files[mf->cur_file-1].top : -1) +
		mf->files[mf->cur_file].blocks;
	    mf->files[++(mf->cur_file)].top = -1;
	    mf->files[mf->cur_file].dir = dp;
	    mf->files[mf->cur_file].number =
		mf->files[mf->cur_file-1].number + 1;
	    mf->files[mf->cur_file].blocks =
	    	mf->files[mf->cur_file].bytes = 0;
	    mf->files[mf->cur_file].fd = -1;
	    sprintf(tmp, "%s/%s-%d.mf", dp->name, mf->name,
		mf->files[mf->cur_file].number);
	    mf->files[mf->cur_file].path = xstrdup(tmp);
	    mf->no_files++;
	    /* open new file and position at beginning */
	    if ((ps = file_position(mf, no, offset)) < 0)
	    	exit(1);
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
    	logf (LOG_FATAL|LOG_ERRNO, "Write failed for file %s part %d",
		mf->name, mf->cur_file);
    	exit(1);
    }
    return 0;
}

/*
 * Destroy a metafile, unlinking component files. File must be open.
 */
int mf_unlink(MFile mf)
{
    int i;

    for (i = 0; i < mf->no_files; i++)
        unlink (mf->files[i].path);
    return 0;
}

/*
 * Unlink the file by name, rather than MFile-handle. File should be closed.
 */
int mf_unlink_name(MFile_area ma, const char *name)
{
    abort();
    return 0;
}
