/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: commit.c,v $
 * Revision 1.1  1995-11-30 08:33:13  adam
 * Started work on commit facility.
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <alexutil.h>

#include <mfile.h>
#include "cfile.h"

void cf_commit (CFile cf)
{
    int i, r, bucket_no;
    int hash_bytes;
    struct CFile_ph_bucket *p;

    if (cf->bucket_in_memory)
    {
        logf (LOG_FATAL, "Cannot commit potential dirty cache");
        exit (1);
    }
    p = xmalloc (sizeof(*p));
    hash_bytes = cf->head.hash_size * sizeof(int);
    bucket_no = (hash_bytes+sizeof(cf->head))/HASH_BSIZE + 2;
    if (lseek (cf->hash_fd, bucket_no * HASH_BSIZE, SEEK_SET) < 0)
    {
        logf (LOG_FATAL|LOG_ERRNO, "seek commit");
        exit (1);
    }
    for (; bucket_no < cf->head.next_bucket; bucket_no++)
    {
        r = read (cf->hash_fd, p, HASH_BSIZE);
        if (r != HASH_BSIZE)
        {
            logf (LOG_FATAL, "read commit hash");
            exit (1);
        }
        for (i = 0; i<HASH_BUCKET && p->vno[i]; i++)
        {
            if (lseek (cf->block_fd, p->vno[i]*cf->head.block_size,
                SEEK_SET) < 0)
            {
                logf (LOG_FATAL, "lseek commit block");
                exit (1);
            }
            r = read (cf->block_fd, cf->iobuf, cf->head.block_size);
            if (r != cf->head.block_size)
            {
                logf (LOG_FATAL, "read commit block");
                exit (1);
            }
            mf_write (cf->mf, p->no[i], 0, 0, cf->iobuf);
        }
    }
    xfree (p);
}

