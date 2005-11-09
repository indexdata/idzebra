/* $Id: xpath6.c,v 1.6 2005-11-09 08:09:44 adam Exp $
   Copyright (C) 1995-2005
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

#include "../api/testlib.h"

int main(int argc, char **argv)
{
    int i;
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);
    char path[256];

    zebra_select_database(zh, "Default");

    zebra_init(zh);

    check_filter(zs, "grs.xml");

    zebra_set_resource(zh, "recordType", "grs.xml");

    zebra_begin_trans(zh, 1);
    for (i = 1; i <= 2; i++)
    {
        sprintf(path, "%.200s/rec%d.xml", get_srcdir(), i);
        zebra_repository_update(zh, path);
    }
    zebra_end_trans(zh);
    zebra_commit(zh);

    do_query(__LINE__, zh, "@attr 5=1 @attr 6=3  @attr 4=1 @attr 1=/assembled/basic/names/CASno \"367-93-1\"", 2);

    do_query(__LINE__, zh, "@attr 5=1 @attr 6=3  @attr 4=1 @attr 1=18 \"367-93-1\"", 2);

    do_query(__LINE__, zh, "@attr 1=/assembled/orgs/org 0", 1);
    
    do_query(__LINE__, zh, 
	     "@and @attr 1=/assembled/orgs/org 0 @attr 5=1 @attr 6=3 @attr 4=1 "
	     "@attr 1=/assembled/basic/names/CASno \"367-93-1\"", 1);

    do_query(__LINE__, zh,
	     "@and @attr 1=/assembled/orgs/org 1 @attr 5=1 @attr 6=3  @attr 4=1 "
	     "@attr 1=/assembled/basic/names/CASno 367-93-1", 2);

    /* bug #317 */
    do_query(__LINE__, zh, "@attr 1=1010 46", 2);

    /* bug #431 */
    do_query(__LINE__, zh, "@attr 1=1021 0", 1);

    /* bug #431 */
    do_query(__LINE__, zh, "@attr 1=1021 46", 1);

    /* bug #431 */
    do_query(__LINE__, zh, "@attr 1=1021 1", 0);

    return close_down(zh, zs, 0);
}
