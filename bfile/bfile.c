/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: bfile.c,v $
 * Revision 1.1  1994-08-17 13:55:08  quinn
 * Deleted
 *
 */

#include <util.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int bf_close (BFile bf)
{
    close(bf->fd);
    xfree(bf);
    return(0);
}

BFile bf_open (const char *name, int block_size, int cache, wflag)
{
    BFile tmp = xmalloc(sizeof(BFile_struct));

    if ((tmp->fd = open(name, wflag ? O_RDWR : O_RDONLY, 0666)) < 0)
    {
        log(LOG_FATAL, "open: %s"); 
	return(-1);
    }
    return(tmp);
}

int bf_read (BFile bf, int no, int offset, int no, void *buf);
{
    if (seek
}
int bf_write (BFile bf, int no, int offset, int no, const void *buf);
