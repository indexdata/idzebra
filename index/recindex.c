/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recindex.c,v $
 * Revision 1.1  1995-11-15 14:46:20  adam
 * Started work on better record management system.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "recindex.h"

#define REC_HEAD_MAGIC "rechead"

static void rec_write_head (Records p)
{
    int r;

    assert (p);
    assert (p->fd != -1);
    if (lseek (p->fd, (off_t) 0, SEEK_SET) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "lseek to 0 in %s", p->fname);
        exit (1);
    }
    r = write (p->fd, &p->head, sizeof(p->head));    
    switch (r)
    {
    case -1:
        logf (LOG_FATAL|LOG_ERRNO, "write head of %s", p->fname);
        exit (1);
    case sizeof(p->head):
        break;
    default:
        logf (LOG_FATAL, "write head of %s. wrote %d", p->fname, r);
        exit (1);
    }
}

Records rec_open (int rw)
{
    Records p;
    int r;

    if (!(p = malloc (sizeof(*p))))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    p->fname = "recindex";
    p->fd = open (p->fname, rw ? (O_RDWR|O_CREAT) : O_RDONLY, 0666);
    if (p->fd == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", p->fname);
        exit (1);
    }
    r = read (p->fd, &p->head, sizeof(p->head));
    switch (r)
    {
    case -1:
        logf (LOG_FATAL|LOG_ERRNO, "read %s", p->fname);
        exit (1);
    case 0:
        p->head.no_records = 0;
        p->head.freelist = 0;
        if (rw)
            rec_write_head (p);
        break;
    case sizeof(p->head):
        if (memcmp (p->head.magic, REC_HEAD_MAGIC, sizeof(p->head.magic)))
        {
            logf (LOG_FATAL, "read %s. bad header", p->fname);
            exit (1);
        }
        break;
    default:
        logf (LOG_FATAL, "read head of %s. expected %d. got %d",
	      p->fname, sizeof(p->head), r);
        exit (1);
    }
    return p;
}

void rec_close (Records p)
{
    if (p->fd != -1)
        close (p->fd);
    free (p);
}

Record rec_get (Records p, int sysno)
{
    assert (p);
    return NULL;
}

Record rec_new (Records p)
{
    assert (p);
    return NULL;
}

void rec_put (Records p, Record rec)
{
    assert (p);
}
