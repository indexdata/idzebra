/* $Id: grsread.h,v 1.15 2004-09-27 10:44:50 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#ifndef GRSREAD_H
#define GRSREAD_H

#include <data1.h>
#include <recctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct grs_read_info {
    void *clientData;
    int (*readf)(void *, char *, size_t);
    off_t (*seekf)(void *, off_t);
    off_t (*tellf)(void *);
    void (*endf)(void *, off_t);
    void *fh;
    off_t offset;
    NMEM mem;
    data1_handle dh;
};

int zebra_grs_extract(void *clientData, struct recExtractCtrl *p,
		      data1_node *(*grs_read)(struct grs_read_info *));

int zebra_grs_retrieve(void *clientData, struct recRetrieveCtrl *p,
		       data1_node *(*grs_read)(struct grs_read_info *));

#ifdef __cplusplus
}
#endif

#endif
