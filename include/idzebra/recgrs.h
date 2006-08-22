/* $Id: recgrs.h,v 1.6 2006-08-22 13:39:25 adam Exp $
   Copyright (C) 1995-2006
   Index Data ApS

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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef RECGRS_H
#define RECGRS_H

#include <idzebra/recctrl.h>

YAZ_BEGIN_CDECL

struct grs_read_info {
    struct ZebraRecStream *stream;
    void *clientData;
    NMEM mem;
    data1_handle dh;
};

YAZ_EXPORT
int zebra_grs_extract(void *clientData, struct recExtractCtrl *p,
		      data1_node *(*grs_read)(struct grs_read_info *));

YAZ_EXPORT
int zebra_grs_retrieve(void *clientData, struct recRetrieveCtrl *p,
		       data1_node *(*grs_read)(struct grs_read_info *));


YAZ_EXPORT
int grs_extract_tree(struct recExtractCtrl *p, data1_node *n);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

