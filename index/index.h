/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: index.h,v $
 * Revision 1.1  1995-08-31 14:50:24  adam
 * New simple file index tool.
 *
 */

#include <util.h>
#include <dict.h>
#include <isam.h>

struct it_key {
    int sysno;
    int seqno;
    int field;
};
