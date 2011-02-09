/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

#ifndef D1_ABSYN_H
#define D1_ABSYN_H 1

#define ENHANCED_XELM 1
#define OPTIMIZE_MELM 1

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
#if OPTIMIZE_MELM
    const char *regexp;
#endif
    int match_state;
    struct data1_xpelement *match_next;
} data1_xpelement;

struct data1_absyn
{
    char *name;
    Odr_oid *oid;
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
    enum DATA1_XPATH_INDEXING xpath_indexing;
};

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

