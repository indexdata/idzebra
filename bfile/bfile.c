/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.c,v $
 * Revision 1.9  1994-08-24 08:45:48  quinn
 * Using mfile.
 *
 * Revision 1.8  1994/08/23  15:03:34  quinn
 * *** empty log message ***
 *
 * Revision 1.7  1994/08/23  14:25:45  quinn
 * Added O_CREAT because some geek wanted it. Sheesh.
 *
 * Revision 1.6  1994/08/23  14:21:38  quinn
 * Fixed call to log
 *
 * Revision 1.5  1994/08/18  08:10:08  quinn
 * Minimal changes
 *
 * Revision 1.4  1994/08/17  14:27:32  quinn
 * last mods
 *
 * Revision 1.2  1994/08/17  14:09:32  quinn
 * Compiles cleanly (still only dummy).
 *
 * Revision 1.1  1994/08/17  13:55:08  quinn
 * New blocksystem. dummy only
 *
 */

#include <util.h>
#include <bfile.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int bf_close (BFile bf)
{
    mf_close(bf->mf);
    xfree(bf);
    return(0);
}

BFile bf_open (const char *name, int block_size, int wflag)
{
    BFile tmp = xmalloc(sizeof(BFile_struct));

    if (!(tmp->mf = mf_open(0, name, block_size, wflag)))
    {
        log(LOG_FATAL|LOG_ERRNO, "mfopen %s", name); 
	return(0);
    }
    return(tmp);
}

int bf_read (BFile bf, int no, int offset, int num, void *buf)
{
    return mf_read(bf->mf, no, offset, num, buf);
}

int bf_write (BFile bf, int no, int offset, int num, const void *buf)
{
    return mf_write(bf->mf, no, offset, num, buf);
}
