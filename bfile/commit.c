/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: commit.c,v $
 * Revision 1.8  1996-02-07 14:03:49  adam
 * Work on flat indexed shadow files.
 *
 * Revision 1.7  1996/02/07  10:08:46  adam
 * Work on flat shadow (not finished yet).
 *
 * Revision 1.6  1995/12/15  12:36:53  adam
 * Moved hash file information to union.
 * Renamed commit files.
 *
 * Revision 1.5  1995/12/12  15:57:55  adam
 * Implemented mf_unlink. cf_unlink uses mf_unlink.
 *
 * Revision 1.4  1995/12/11  09:03:55  adam
 * New function: cf_unlink.
 * New member of commit file head: state (0) deleted, (1) hash file.
 *
 * Revision 1.3  1995/12/01  16:24:29  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.2  1995/12/01  11:37:24  adam
 * Cached/commit files implemented as meta-files.
 *
 * Revision 1.1  1995/11/30  08:33:13  adam
 * Started work on commit facility.
 *
 */

#include <assert.h>
#include <stdlib.h>

#include <alexutil.h>
#include <mfile.h>
#include "cfile.h"

void cf_unlink (CFile cf)
{
    if (cf->bucket_in_memory)
    {
        logf (LOG_FATAL, "Cannot unlink potential dirty cache");
        exit (1);
    }
    cf->head.state = 0;
    cf->dirty = 1;
    mf_unlink (cf->block_mf);
    mf_unlink (cf->hash_mf);
}

   
static void cf_commit_hash (CFile cf)
{ 
    int i, bucket_no;
    int hash_bytes;
    struct CFile_ph_bucket *p;

    p = xmalloc (sizeof(*p));
    hash_bytes = cf->head.hash_size * sizeof(int);
    bucket_no = cf->head.first_bucket;
    for (; bucket_no < cf->head.next_bucket; bucket_no++)
    {
        if (!mf_read (cf->hash_mf, bucket_no, 0, 0, p))
        {
            logf (LOG_FATAL, "read commit hash");
            exit (1);
        }
        for (i = 0; i<HASH_BUCKET && p->vno[i]; i++)
        {
            if (!mf_read (cf->block_mf, p->vno[i], 0, 0, cf->iobuf))
            {
                logf (LOG_FATAL, "read commit block");
                exit (1);
            }
            mf_write (cf->rmf, p->no[i], 0, 0, cf->iobuf);
        }
    }
    xfree (p);
}

static void cf_commit_flat (CFile cf)
{
    int *fp;
    int hno;
    int i, vno = 0;

    fp = xmalloc (HASH_BSIZE);
    for (hno = cf->head.next_bucket; hno < cf->head.flat_bucket; hno++)
    {
        mf_read (cf->hash_mf, hno, 0, 0, fp);
        for (i = 0; i < (HASH_BSIZE/sizeof(int)); i++)
        {
            if (fp[i])
            {
                if (!mf_read (cf->block_mf, fp[i], 0, 0, cf->iobuf))
                {
                    logf (LOG_FATAL, "read commit block at %d (->%d)",
                          fp[i], vno);
                    exit (1);
                }
                mf_write (cf->rmf, vno, 0, 0, cf->iobuf);
            }
            vno++;
        }
    }
    xfree (fp);
}

void cf_commit (CFile cf)
{

    if (cf->bucket_in_memory)
    {
        logf (LOG_FATAL, "Cannot commit potential dirty cache");
        exit (1);
    }
    if (cf->head.state == 1)
        cf_commit_hash (cf);
    else if (cf->head.state == 2)
        cf_commit_flat (cf);
}

