/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#ifndef RANK_H
#define RANK_H

#include <idzebra/api.h>

YAZ_BEGIN_CDECL

struct rank_control {
    char *name;
    void *(*create)(ZebraHandle zh);
    void (*destroy)(struct zebra_register *reg, void *class_handle);
    void *(*begin)(struct zebra_register *reg, 
                   void *class_handle, RSET rset, NMEM nmem,
                   TERMID *terms, int numterms);
    /* ### Could add parameters to begin:
     *	char *index;	// author, title, etc.
     *	int dbsize;	// number of records in database
     *	int rssize;	// number of records in result set (estimate?)
     */
    void (*end)(struct zebra_register *reg, void *set_handle);
    int (*calc)(void *set_handle, zint sysno, zint staticrank,
		int *stop_flag);
    void (*add)(void *set_handle, int seqno, TERMID term);
};

void zebraRankInstall (struct zebra_register *reg, struct rank_control *ctrl);
ZebraRankClass zebraRankLookup (ZebraHandle zh, const char *name);
void zebraRankDestroy (struct zebra_register *reg);

/* declaring externally defined rank class structures */
/* remember to install rank classes in zebraapi.c as well!! */
extern struct rank_control *rank_1_class;
extern struct rank_control *rank_2_class;
extern struct rank_control *rank_zv_class;
extern struct rank_control *rank_static_class;
extern struct rank_control *rank_similarity_class;



YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

