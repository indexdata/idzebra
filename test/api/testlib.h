/* $Id: testlib.h,v 1.2 2004-10-28 15:24:36 heikki Exp $
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

/** testlib - utilities for the api tests */

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>


/** read zebra.cfg from env var srcdir if it exists; otherwise current dir */
ZebraService start_service(char *cfgfile);

/** initialises the zebra base and inserts some test data in it */
void init_data( ZebraHandle zh, const char **recs);


/** makes a query, and compares the number of hits to the expected */
void Query(int lineno, ZebraHandle zh, char *query, int exphits);


/** 
 * makes a query, checks number of hits, and for the first hit, that 
 * it contains the given string, and that it gets the right score
 */
void RankingQuery(int lineno, ZebraHandle zh, char *query, 
           int exphits, char *firstrec, int firstscore );

