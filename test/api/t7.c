/* $Id: t7.c,v 1.6 2005-01-15 19:38:35 adam Exp $
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

/** t7.c sorting  */

#include "testlib.h"
#include <yaz/sortspec.h>

const char *recs[] = {
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n",
        0};


int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle  zh = zebra_open (zs);

    const char *setname1="set1";
    const char *setname2="set2";
    const char *setname3="set3";
    int status;
    int rc;
    ODR odr_input = odr_createmem (ODR_DECODE);    
    ODR odr_output = odr_createmem (ODR_ENCODE);    
    YAZ_PQF_Parser parser = yaz_pqf_create();
    Z_RPNQuery *query = yaz_pqf_parse(parser, odr_input, 
                                      "@attr 1=4 my");
    Z_SortKeySpecList *spec = 
          yaz_sort_spec (odr_output, "@attr 1=4 id");
    int hits;

    init_data(zh, recs);

    zebra_begin_trans(zh, 0);
        
    zebra_search_RPN(zh, odr_input, query, setname1, &hits);

    rc=zebra_sort(zh, odr_output, 1, &setname1, setname2, spec, &status);
    if (rc) 
    { 
        printf("sort A returned %d %d \n",rc,status); 
        exit(1);
    }
    
    rc=zebra_sort(zh, odr_output, 1, &setname2, setname3, spec, &status);
    if (rc) 
    { 
        printf("sort B returned %d %d \n",rc,status); 
        exit(1);
    }

    zebra_end_trans(zh);
    yaz_pqf_destroy(parser);

    /*
     zebra_deleleResultSet(zh, Z_DeleteRequest_list,
                          1, &setnamep, &status);
    */  
    odr_destroy(odr_input);
    odr_destroy(odr_output);

    zebra_commit(zh);

    return close_down(zh, zs, 0);
}
