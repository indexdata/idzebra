/* $Id: d1_absyn.h,v 1.4 2006-05-19 13:49:34 adam Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#ifndef D1_ABSYN_H
#define D1_ABSYN_H 1

#define ENHANCED_XELM 1

#include <zebra_xpath.h>
#include <idzebra/data1.h>
#include <dfa.h>

typedef struct data1_xpelement
{
    char *xpath_expr;
#ifdef ENHANCED_XELM 
    struct xpath_location_step xpath[XPATH_STEP_COUNT];
    int xpath_len;
#endif
    struct DFA *dfa;  
    data1_termlist *termlists;
    struct data1_xpelement *next;
} data1_xpelement;

struct data1_absyn
{
    char *name;
    oid_value reference;
    data1_tagset *tagset;
    data1_varset *varset;
    data1_esetname *esetnames;
    data1_maptab *maptabs;
    data1_marctab *marc;
    data1_sub_elements *sub_elements;
    data1_element *main_elements;
    struct data1_xpelement *xp_elements; /* pop */
    struct data1_systag *systags;
    char *encoding;
    int  enable_xpath_indexing;
};

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

