/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rsisam.c,v $
 * Revision 1.3  1994-11-22 13:15:37  quinn
 * Simple
 *
 * Revision 1.2  1994/11/04  14:53:12  quinn
 * Work
 *
 */

/* TODO: Memory management
   LINK DELETE TO CLOSE!  */

#include <rsisam.h>
#include <util.h>

rset_control *r_create(const struct rset_control *sel, void *parms);
static int r_open(rset_control *ct, int wflag);
static void r_close(rset_control *ct);
static void r_delete(rset_control *ct);
static void r_rewind(rset_control *ct);
static int r_count(rset_control *ct);
static int r_read();
static int r_write();

static const rset_control control = 
{
    "ISAM set type",
    0,
    r_create,
    r_open,
    r_close,
    r_delete,
    r_rewind,
    r_count,
    r_read,
    r_write
};

const rset_control *rset_kind_isam = &control;

rset_control *r_create(const struct rset_control *sel, void *parms)
{
    rset_control *newct;
    rset_isam_parms *pt = parms;

    log(LOG_DEBUG, "risam_create(%s)", sel->desc);
    newct = xmalloc(sizeof(*newct));
    memcpy(newct, sel, sizeof(*sel));
    if (!(newct->buf = (char*) is_position(pt->is, pt->pos)))
    	return 0;
    return newct;
}

static int r_open(rset_control *ct, int wflag)
{
    log(LOG_DEBUG, "risam_open");
    if (wflag)
    {
	log(LOG_FATAL, "ISAM set type is read-only");
	return -1;
    }
    r_rewind(ct);
    return 0;
}

static void r_close(rset_control *ct)
{
    /* NOP */
}

static void r_delete(rset_control *ct)
{
    log(LOG_DEBUG, "risam_delete");
    is_pt_free((ISPT) ct->buf);
    xfree(ct);
}

static void r_rewind(rset_control *ct)
{
    log(LOG_DEBUG, "risam_rewind");
    is_rewind((ISPT) ct->buf);
}

static int r_count(rset_control *ct)
{return 0;}

static int r_read(rset_control *ct, void *buf)
{
    return is_readkey((ISPT) ct->buf, buf);
}

static int r_write()
{
    log(LOG_FATAL, "ISAM set type is read-only");
    return -1;
}
