/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

/** testing of scan */
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "testlib.h"

const char *myrec[] = {
        "<gils>\n<title>a b</title>\n</gils>\n",
        "<gils>\n<title>c d</title>\n</gils>\n",
        "<gils>\n<title>e f</title>\n</gils>\n" ,
	0} ;

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(zh);

    YAZ_CHECK(tl_init_data(zh, myrec));

    /*
      int tl_scan
      (
      ZebraHandle zh, const char *query,
      int pos, int num,
      int exp_pos, int exp_num, int exp_partial,
      const char **exp_entries
      )
    */


    {
	/* bad string use attrite, bug #647 */
	YAZ_CHECK(tl_scan(zh, "@attr 1=bad 0", 1, 1, 1, 1, 0, 0));
    }

    {
	/* bad numeric use attributes, bug #647 */
	YAZ_CHECK(tl_scan(zh, "@attr 1=1234 0", 1, 1, 1, 1, 0, 0));
    }

    {
	/* scan before. nothing must be returned */
	const char *ent[] = { "a", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 1, 1, 1, 1, 0, ent));
    }

    {
	/* scan after. nothing must be returned */
	const char *ent[] = { 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 m", 1, 1, 1, 0, 1, ent));
    }

    {
	const char *ent[] = { "a", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", 1, 1, 1, 1, 0, ent));
    }

    {
	const char *ent[] = { "b", "c", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 aa", 1, 2, 1, 2, 0, ent));
    }

    {
	const char *ent[] = { "b", "c", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 aa", 1, 2, 1, 2, 0, ent));
    }

    {
	const char *ent[] = { "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 e", 1, 3, 1, 2, 1, ent));
    }

    {
	const char *ent[] = { "c", "d", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -1, 2, -1, 2, 0, ent));
    }

    {
	const char *ent[] = { "d", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -2, 1, -2, 1, 0, ent));
    }

    {
	const char *ent[] = { "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -4, 1, -4, 1, 0, ent));
    }

    {
	const char *ent[] = { "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -5, 1, -5, 0, 1, ent));
    }

    {
	const char *ent[] = { "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -2, 3, -2, 3, 0, ent));
    }

    {
	const char *ent[] = { "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 a", -2, 4, -2, 3, 1, ent));
    }

    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 2, 100, 1, 6, 1, ent));
    }

    {
	const char *ent[] = { "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 0, 100, 0, 5, 1, ent));
    }

    {
	const char *ent[] = { "a", "b", "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 10, 100, 1, 6, 1, ent));
    }


    {
	const char *ent[] = { 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 0", 22, 10, 1, 0, 1, ent));
    }

    {
	const char *ent[] = { "a", "b", "c", "d", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 f", 6, 4, 6, 4, 0, ent));
    }

    {
	const char *ent[] = { "a", "b", "c", "d", "e", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 f", 6, 5, 6, 5, 0, ent));
    }

    {
	const char *ent[] = { "a", "b", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 c", 6, 5, 3, 2, 1, ent));
    }

    {
	const char *ent[] = { "c", "d", "e", "f", 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 c", 1, 6, 1, 4, 1, ent));
    }

    {
	const char *ent[] = { 0 };
	YAZ_CHECK(tl_scan(zh, "@attr 1=4 z", -22, 10, -22, 0, 1, ent));
    }

    {
	const char *ent[] = { "c", "d", 0 };

        YAZ_CHECK(tl_query(zh, "@attr 1=4 c", 1));

        /* must fail, because x is not a result set */
	YAZ_CHECK(tl_scan(zh, "@attr 8=x @attr 1=4 a", 1, 3, 0, 0, 0, 0));

        /* bug 1114 */
	YAZ_CHECK(tl_scan(zh, "@attr 8=rsetname @attr 1=4 0",
                          1, 20, 1, 2, 1, ent));

    }
    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

