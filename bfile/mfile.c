/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: mfile.c,v $
 * Revision 1.5  1994-09-12 08:01:51  quinn
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

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <assert.h>

#include <util.h>
#include <mfile.h>

static MFile_area_struct *open_areas = 0;
static MFile_area_struct *default_area = 0;

static int scan_areadef(MFile_area ma, const char *name)
{
    const char *ad = res_get(common_resource, name);
    int offset = 0, rs, size, multi, rd;
    char dirname[FILENAME_MAX+1], unit; 
    mf_dir **dp = &ma->dirs, *dir = *dp;

    if (!ad)
    {
    	log(LOG_FATAL, "Could not find resource '%s'", name);
    	return -1;
    }
    for (;;)
    {
        rs = sscanf(ad + offset, "%[^:]:%d%c %n", dirname, &size, &unit, &rd);
        if (rs <= 1)
	    break;
        if (rs != 3)
        {
	    log(LOG_FATAL, "Illegal directory description: %s", ad + offset);
	    return -1;
	}
	switch (unit)
	{
	    case 'B': case 'b': multi = 1; break;
	    case 'K': case 'k': multi = 1024; break;
	    case 'M': case 'm': multi = 1048576; break;
	    default:
	        log(LOG_FATAL, "Illegal unit: %c in %s", unit, ad + offset);
	        return -1;
	}
	*dp = dir = xmalloc(sizeof(mf_dir));
	dir->next = 0;
	strcpy(dir->name, dirname);
	dir->max_bytes = dir->avail_bytes = size * multi;
	dp = &dir->next;
	offset += rd;
    }
    return 0;
}

static int file_position(MFile mf, int pos)
{
    int off = 0, c = mf->cur_file, ps;

    if ((c > 0 && pos <= mf->files[c-1].top) ||
	(c < mf->no_files -1 && pos > mf->files[c].top))
    {
	c = 0;
	while (mf->files[c].top >= 0 && mf->files[c].top < pos)
	{
	    off += mf->files[c].blocks;
	    c++;
	}
	assert(c < mf->no_files);
    }
    else
    	off = c ? (mf->files[c-1].top + 1) : 0;
    if (mf->files[c].fd < 0 && (mf->files[c].fd = open(mf->files[c].path,
	mf->wr ? O_RDWR|O_CREAT : O_RDONLY, 0666)) < 0)
    {
    	log(LOG_FATAL|LOG_ERRNO, "Failed to open %s", mf->files[c].path);
    	return -1;
    }
    if (lseek(mf->files[c].fd, (ps = pos - off) * mf->blocksize, SEEK_SET) < 0)
    {
    	log(LOG_FATAL|LOG_ERRNO, "Failed to seek in %s", mf->files[c].path);
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
MFile_area mf_init(const char *name)
{
    MFile_area ma = xmalloc(sizeof(MFile_area_struct)), mp;
    mf_dir *dirp;
    meta_file *meta_f;
    part_file *part_f;
    DIR *dd;
    struct dirent *dent;
    int fd, number;
    char metaname[FILENAME_MAX+1], tmpnam[FILENAME_MAX+1];

    log(LOG_DEBUG, "mf_init(%s)", name);
    for (mp = open_areas; mp; mp = mp->next)
    	if (!strcmp(name, mp->name))
    	    abort();
    strcpy(ma->name, name);
    ma->next = open_areas;
    open_areas = ma;
    ma->mfiles = 0;
    ma->dirs = 0;
    if (scan_areadef(ma, name) < 0)
    {
    	log(LOG_FATAL, "Failed to access description of '%s'", name);
    	return 0;
    }
    /* look at each directory */
    for (dirp = ma->dirs; dirp; dirp = dirp->next)
    {
    	if (!(dd = opendir(dirp->name)))
    	{
    	    log(LOG_FATAL|LOG_ERRNO, "Failed to open %s", dirp->name);
    	    return 0;
	}
	/* look at each file */
	while ((dent = readdir(dd)))
	{
	    if (*dent->d_name == '.')
	    	continue;
	    if (sscanf(dent->d_name, "%[^.].%d", metaname, &number) != 2)
	    {
	    	log(LOG_FATAL, "Failed to resolve part-name %s", dent->d_name);
	    	return 0;
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
	    	meta_f = xmalloc(sizeof(*meta_f));
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
	    if ((fd = open(part_f->path, O_RDONLY)) < 0)
	    {
	    	log(LOG_FATAL|LOG_ERRNO, "Failed to access %s", dent->d_name);
	    	return 0;
	    }
	    if ((part_f->bytes = lseek(fd, 0, SEEK_END)) < 0)
	    {
	    	log(LOG_FATAL|LOG_ERRNO, "Failed to seek in %s", dent->d_name);
	    	return 0;
	    }
	    close(fd);
	    dirp->avail_bytes -= part_f->bytes;
	}
	closedir(dd);
    }
    for (meta_f = ma->mfiles; meta_f; meta_f = meta_f->next)
    {
    	log(LOG_DEBUG, "mf_init: %s consists of %d part(s)", meta_f->name,
    	    meta_f->no_files);
    	qsort(meta_f->files, meta_f->no_files, sizeof(part_file),
    	    cmp_part_file);
    }
    return ma;
}

/*
 * Open a metafile.
 * If !ma, Use MF_DEFAULT_AREA.
 */
MFile mf_open(MFile_area ma, const char *name, int block_size, int wflag)
{
    struct meta_file *new;
    int i;
    char tmp[FILENAME_MAX+1];
    mf_dir *dp;

    log(LOG_LOG, "mf_open(%s)", name);
    if (!ma)
    {
        if (!default_area && !(default_area = mf_init(MF_DEFAULT_AREA)))
        {
            log(LOG_FATAL, "Failed to open default area.");
            return 0;
	}
	ma = default_area;
    }
    for (new = ma->mfiles; new; new = new->next)
    	if (!strcmp(name, new->name))
    	    if (new->open)
    	        abort();
	    else
	        break;
    if (!new)
    {
    	new = xmalloc(sizeof(*new));
    	strcpy(new->name, name);
    	/* allocate one, empty file */
    	new->no_files = 1;
    	new->files[0].bytes = new->files[0].blocks = 0;
    	new->files[0].top = -1;
    	new->files[0].number = 0;
    	new->files[0].fd = -1;
    	new->min_bytes_creat = MF_MIN_BLOCKS_CREAT * block_size;
    	for (dp = ma->dirs; dp && dp->avail_bytes < new->min_bytes_creat;
	    dp = dp->next);
	if (!dp)
	{
	    log(LOG_FATAL, "Insufficient space for new mfile.");
	    return 0;
	}
	new->files[0].dir = dp;
    	sprintf(tmp, "%s/%s.%d", dp->name, new->name, 0);
    	new->files[0].path = xstrdup(tmp);
    	new->ma = ma;
    }
    else
    {
    	assert(!new->open);
    }
    new->blocksize = block_size;
    new->min_bytes_creat = MF_MIN_BLOCKS_CREAT * block_size;
    new->wr=wflag;
    new->cur_file = 0;
    new->open = 1;

    for (i = 0; i < new->no_files; i++)
    {
    	new->files[i].blocks = new->files[i].bytes / new->blocksize;
    	if (i == new->no_files)
	    new->files[i].top = -1;
	else
	    new->files[i].top = i ? (new->files[i-1].top + new->files[i].blocks)
	        : (new->files[i].blocks - 1);
    }
    return new;
}

/*
 * Close a metafile.
 */
int mf_close(MFile mf)
{
    int i;

    assert(mf->open);
    for (i = 0; i < mf->no_files; i++)
    	if (mf->files[i].fd >= 0)
    	    close(mf->files[i].fd);
    mf->open = 0;
    return 0;
}

/*
 * Read one block from a metafile. Interface mirrors bfile.
 */
int mf_read(MFile mf, int no, int offset, int num, void *buf)
{
    int rd;

    if (file_position(mf, no) < 0)
    	exit(1);
    if ((rd = read(mf->files[mf->cur_file].fd, buf, mf->blocksize)) < 0)
    {
    	log(LOG_FATAL|LOG_ERRNO, "Read failed");
    	exit(1);
    }
    else if (rd < mf->blocksize)
    	return 0;
    else
    	return 1;
}

/*
 * Write.
 */
int mf_write(MFile mf, int no, int offset, int num, const void *buf)
{
    int ps, nblocks;
    mf_dir *dp;
    char tmp[FILENAME_MAX+1];
    unsigned char dummych = '\xff';

    if ((ps = file_position(mf, no)) < 0)
	exit(1);
    /* file needs to grow */
    while (ps >= mf->files[mf->cur_file].blocks)
    {
    	log(LOG_DEBUG, "File grows");
    	/* file overflow - allocate new file */
    	if ((ps - mf->files[mf->cur_file].blocks + 1) * mf->blocksize >
	    mf->files[mf->cur_file].dir->avail_bytes)
	{
	    /* cap off file? */
	    if ((nblocks = mf->files[mf->cur_file].dir->avail_bytes /
		mf->blocksize) > 0)
	    {
	    	log(LOG_DEBUG, "Capping off file %s at pos %d",
		    mf->files[mf->cur_file].path, nblocks);
	    	if ((ps = file_position(mf,
		    (mf->cur_file ? mf->files[mf->cur_file-1].top : 0) +
		    mf->files[mf->cur_file].blocks + nblocks)) < 0)
			exit(1);
		if (write(mf->files[mf->cur_file].fd, &dummych, 1) < 1)
		{
		    log(LOG_ERRNO|LOG_FATAL, "write dummy");
		    exit(1);
		}
		mf->files[mf->cur_file].blocks += nblocks;
		mf->files[mf->cur_file].bytes += nblocks * mf->blocksize;
		mf->files[mf->cur_file].dir->avail_bytes -= nblocks *
		    mf->blocksize;
	    }
	    /* get other bit */
    	    log(LOG_DEBUG, "Creating new file.");
    	    for (dp = mf->ma->dirs; dp && dp->avail_bytes < mf->min_bytes_creat;
		dp = dp->next);
	    if (!dp)
	    {
	    	log(LOG_FATAL, "Cannot allocate more space for %s", mf->name);
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
	    sprintf(tmp, "%s/%s.%d", dp->name, mf->name,
		mf->files[mf->cur_file].number);
	    mf->files[mf->cur_file].path = xstrdup(tmp);
	    mf->no_files++;
	    /* open new file and position at beginning */
	    if ((ps = file_position(mf, no)) < 0)
	    	exit(1);
	}
	else
	{
	    nblocks = ps - mf->files[mf->cur_file].blocks + 1;
	    mf->files[mf->cur_file].blocks += nblocks;
	    mf->files[mf->cur_file].bytes += nblocks * mf->blocksize;
	    mf->files[mf->cur_file].dir->avail_bytes -= nblocks * mf->blocksize;
	}
    }
    if (write(mf->files[mf->cur_file].fd, buf, mf->blocksize) < mf->blocksize)
    {
    	log(LOG_FATAL|LOG_ERRNO, "Write failed");
    	exit(1);
    }
    return 0;
}

/*
 * Destroy a metafile, unlinking component files. File must be open.
 */
int mf_unlink(MFile mf)
{
    abort();
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
