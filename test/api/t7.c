/* $Id: t7.c,v 1.13 2006-05-30 22:03:13 adam Exp $
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

/** t7.c sorting  */

#include "testlib.h"
#include <yaz/sortspec.h>

const char *recs[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};

static void tst(int argc, char **argv)
{
    const char *setname1 = "set1";
    const char *setname2 = "set2";
    const char *setname3 = "set3";
    int status;
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open (zs, 0);
    ODR odr_input = odr_createmem (ODR_DECODE);    
    ODR odr_output = odr_createmem (ODR_ENCODE);    
    YAZ_PQF_Parser parser = yaz_pqf_create();
    Z_RPNQuery *query = yaz_pqf_parse(parser, odr_input, "@attr 1=4 my");
    Z_SortKeySpecList *spec = yaz_sort_spec (odr_output, "1=4 <!");
    zint hits;

    YAZ_CHECK(tl_init_data(zh, recs));

    YAZ_CHECK(zebra_begin_trans(zh, 0) == ZEBRA_OK);
        
    YAZ_CHECK(zebra_search_RPN(zh, odr_input, query, setname1, &hits) ==
	      ZEBRA_OK);

    YAZ_CHECK(zebra_sort(zh, odr_output, 1, &setname1, setname2, spec,
			 &status)
	      == ZEBRA_OK);
    YAZ_CHECK(zebra_sort(zh, odr_output, 1, &setname2, setname3, spec, 
			 &status) == ZEBRA_OK);

    spec = yaz_sort_spec(odr_output, "1=5 <!"); /* invalid sort spec */

    YAZ_CHECK(zebra_sort(zh, odr_output, 1, &setname1, setname2, spec,
			 &status) == ZEBRA_FAIL);

    YAZ_CHECK(zebra_end_trans(zh) == ZEBRA_OK);

    yaz_pqf_destroy(parser);

    /*
     zebra_deleteResultSet(zh, Z_DeleteRequest_list,
                          1, &setnamep, &status);
    */  
    odr_destroy(odr_input);
    odr_destroy(odr_output);

    zebra_commit(zh);

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

