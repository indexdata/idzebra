/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kcompare.c,v $
 * Revision 1.1  1995-09-04 09:10:36  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "index.h"

int key_compare (const void *p1, const void *p2)
{
    struct it_key i1, i2;
    memcpy (&i1, p1, sizeof(i1));
    memcpy (&i2, p2, sizeof(i2));
    if ( i1.sysno != i2.sysno)
        return i1.sysno - i2.sysno;
    if ( i1.seqno != i2.seqno)
        return i1.seqno - i2.seqno;
    return i1.field - i2.field;
}

int key_compare_x (const struct it_key *i1, const struct it_key *i2)
{
    if ( i1->sysno != i2->sysno)
        return i1->sysno - i2->sysno;
    if ( i1->seqno != i2->seqno)
        return i1->seqno - i2->seqno;
    return i1->field - i2->field;
}

