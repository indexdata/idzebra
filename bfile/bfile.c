/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.c,v $
 * Revision 1.3  1994-08-17 14:10:41  quinn
 * *** empty log message ***
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

int bf_close (BFile bf)
{
    close(bf->fd);
    xfree(bf);
    return(0);
}

BFile bf_open (const char *name, int block_size, int wflag)
{
    BFile tmp = xmalloc(sizeof(BFile_struct));

    if ((tmp->fd = open(name, wflag ? O_RDWR : O_RDONLY, 0666)) < 0)
    {
        log(LOG_FATAL, "open: %s"); 
	return(0);
    }
    return(tmp);
}

int bf_read (BFile bf, int no, int offset, int num, void *buf)
{
}

int bf_write (BFile bf, int no, int offset, int num, const void *buf)
{
}
