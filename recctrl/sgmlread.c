/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: sgmlread.c,v $
 * Revision 1.1  1996-10-11 10:57:32  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "grsread.h"

data1_node *grs_read_sgml (struct grs_read_info *p)
{
    return data1_read_record (p->readf, p->fh, p->mem);
}
