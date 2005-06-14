/* $Id: charmap1.c,v 1.7 2005-06-14 12:42:19 adam Exp $
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
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs);
    char path[256];

    check_filter(zs, "grs.xml");

    zebra_select_database(zh, "Default");

    zebra_init(zh);

    zebra_begin_trans(zh, 1);
    sprintf(path, "%.200s/x.xml", get_srcdir());
    zebra_repository_update(zh, path);
    zebra_end_trans(zh);
    zebra_commit(zh);

    do_query(__LINE__, zh, "@term string æ", 1);

    /* search for UNICODE 1E25 - letter h with dot below */
    do_query(__LINE__, zh, "@term string ḥ", 1);

    /* search for UNICODE A ring */
    do_query(__LINE__, zh, "@term string lås", 1);

    /* search for aa  */
    do_query(__LINE__, zh, "@term string laas", 1);

    /* search for aa regular-1 */
    do_query(__LINE__, zh, "@attr 5=102 @term string lås", 1);

    /* search for aa regular-2 */
    do_query(__LINE__, zh, "@attr 5=103 @term string lås", 1);

    /* search for aa trunc=104 */
    do_query(__LINE__, zh, "@attr 5=104 @term string laas", 1);

    /* search for aa trunc=105 */
    do_query(__LINE__, zh, "@attr 5=104 @term string laas", 1);

    /* search for aaa  */
    do_query(__LINE__, zh, "@term string laaas", 0);
    
    /* search ABC in title:0 .  */
    do_query(__LINE__, zh, "@attr 4=3 @attr 1=4 ABC", 1);
    
    /* search DEF in title:0 .  */
    do_query(__LINE__, zh, "@attr 4=3 @attr 1=4 DEF", 0);
    
    /* search [ in title:0 .  */
    do_query(__LINE__, zh, "@attr 4=3 @attr 1=4 [", 1);
    
    /* search \ in title:0 .  */
    do_query(__LINE__, zh, "@attr 4=3 @attr 1=4 \\\\\\\\", 1);

    /* search { in title:0 .  */
    do_query(__LINE__, zh, "@attr 4=3 @attr 1=4 \\{", 1);

    return close_down(zh, zs, 0);
}
