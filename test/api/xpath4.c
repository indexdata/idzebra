

/* $Id: xpath4.c,v 1.1 2004-10-29 14:16:22 heikki Exp $
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

#include "testlib.h"


/** xpath4.c - Attributes */

const char *myrec[] = {
    "<record> \n"
    "  <title>foo</title> \n"
    "  <title>bar</title> \n"
    "  <author>gryf</author> \n"
    "</record> \n",
    
    "<record> \n"
    "  <title>foo bar</title> \n"
    "  <author>gryf</author> \n"
    "</record> \n",
   
    "<record> \n"
    "  <title lang=en>foo gryf</title> \n"
    "  <author>grunt</author> \n"
    "</record> \n",
  
    "<record> \n"
    "  <title lang=da>foo grunt</title> \n"
    "  <value>bar</value> \n"
    "</record> \n",
 
    "<record> \n"
    "  <title>hamlet</title> \n"
    "  <author>foo bar grunt</author> \n"
    "</record> \n",

    0};


int main(int argc, char **argv)
{
    ZebraService zs = start_up("zebraxpath.cfg", argc, argv);
    ZebraHandle zh = zebra_open (zs);
    init_data(zh,myrec);

#define q(qry,hits) do_query(__LINE__,zh,qry,hits)

    q("@attr 1=/record/title foo",4);
    q("@attr 1=/record/title bar",2);
    q("@attr 1=/record/title[@lang='da'] foo",1);
    q("@attr 1=/record/title[@lang='en'] foo",1);
    q("@attr 1=/record/title @and foo bar",2);
    /* The last one returns two hits, as the and applies to the whole
    record, so it matches <title>foo</title><title>bar</title>
    This might not have to be like that, but currently that is what
    zebra does.  */
    q("@and @attr 1=/record/title foo @attr 1=/record/title bar ",2);
    return close_down(zh, zs, 0);
}
