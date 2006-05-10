/* $Id: charmap1.c,v 1.10 2006-05-10 08:13:36 adam Exp $
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

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open(zs, 0);
    char path[256];

    tl_check_filter(zs, "grs.xml");

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_begin_trans(zh, 1) == ZEBRA_OK);

    sprintf(path, "%.200s/x.xml", tl_get_srcdir());
    zebra_repository_update(zh, path);
    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);
    zebra_commit(zh);

    YAZ_CHECK(tl_query(zh, "@term string æ", 1));

    /* search for UNICODE 1E25 - letter h with dot below */
    YAZ_CHECK(tl_query(zh, "@term string ḥ", 1));

    /* search for UNICODE A ring */
    YAZ_CHECK(tl_query(zh, "@term string lås", 1));

    /* search for aa  */
    YAZ_CHECK(tl_query(zh, "@term string laas", 1));

    /* search for aa regular-1 */
    YAZ_CHECK(tl_query(zh, "@attr 5=102 @term string lås", 1));

    /* search for aa regular-2 */
    YAZ_CHECK(tl_query(zh, "@attr 5=103 @term string lås", 1));

    /* search for aa trunc=104 */
    YAZ_CHECK(tl_query(zh, "@attr 5=104 @term string laas", 1));

    /* search for aa trunc=105 */
    YAZ_CHECK(tl_query(zh, "@attr 5=104 @term string laas", 1));

    /* search for aaa  */
    YAZ_CHECK(tl_query(zh, "@term string laaas", 0));
    
    /* search ABC in title:0 .  */
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=4 ABC", 1));
    
    /* search DEF in title:0 .  */
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=4 DEF", 0));
    
    /* search [ in title:0 .  */
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=4 [", 1));
    
    /* search \ in title:0 .  */
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=4 \\\\\\\\", 1));

    /* search { in title:0 .  */
    YAZ_CHECK(tl_query(zh, "@attr 4=3 @attr 1=4 \\{", 1));
    
    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

