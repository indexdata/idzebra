/* $Id: xpath4.c,v 1.2 2004-12-15 13:07:07 adam Exp $
   Copyright (C) 2003,2004
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

#include "../api/testlib.h"

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
    "  <title lang=en>double english</title> \n"
    "  <title lang=da>double danish</title> \n"
    "  <author>grunt</author> \n"
    "</record> \n",

    "<record> \n"
    "  <title>hamlet</title> \n"
    "  <author>foo bar grunt grunt grunt</author> \n"
    "</record> \n",

    "<record> \n"
    "  before \n"
    "  <nested> \n"
    "     early \n"
    "     <nested> \n"
    "        middle \n"
    "     </nested> \n"
    "     late \n"
    "  </nested> \n"
    "  after \n"
    "</record> \n",

    "<record> \n"
    "  before \n"
    "  <nestattr level=outer> \n"
    "     early \n"
    "     <nestattr level=inner> \n"
    "        middle \n"
    "     </nestattr> \n"
    "     late \n"
    "  </nestattr> \n"
    "  after \n"
    "</record> \n",
    0};


int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs);

#if 0
    yaz_log_init_level( yaz_log_mask_str_x("xpath4,rsbetween", LOG_DEFAULT_LEVEL));
#endif

    init_data(zh, myrec);

#define q(qry,hits) do_query(__LINE__,zh,qry,hits)

    q("@attr 1=/record/title foo",4);
    q("@attr 1=/record/title bar",2);
    q("@attr 1=/record/title[@lang='da'] foo",1);
    q("@attr 1=/record/title[@lang='en'] foo",1);

    q("@attr 1=/record/title[@lang='en'] english",1); 
    q("@attr 1=/record/title[@lang='da'] english",0); 
    q("@attr 1=/record/title[@lang='da'] danish",1);  
    q("@attr 1=/record/title[@lang='en'] danish",0);  

    q("@attr 1=/record/title @and foo bar",2);
    /* The previous one returns two hits, as the and applies to the whole
    record, so it matches <title>foo</title><title>bar</title>
    This might not have to be like that, but currently that is what
    zebra does.  */
    q("@and @attr 1=/record/title foo @attr 1=/record/title bar ",2);

    /* check we get all the occureences for 'grunt' */
    /* this can only be seen in the log, with debugs on. bug #202 */
    q("@attr 1=/record/author grunt",3);

    /* check nested tags */
    q("@attr 1=/record/nested before",0);
    q("@attr 1=/record/nested early",1);
    q("@attr 1=/record/nested middle",1);
    q("@attr 1=/record/nested late",1);
    q("@attr 1=/record/nested after",0);

    q("@attr 1=/record/nested/nested before",0);
    q("@attr 1=/record/nested/nested early",0);
    q("@attr 1=/record/nested/nested middle",1);
    q("@attr 1=/record/nested/nested late",0);
    q("@attr 1=/record/nested/nested after",0);

    q("@attr 1=/record/nestattr[@level='outer'] before",0);
    q("@attr 1=/record/nestattr[@level='outer'] early",1);
    q("@attr 1=/record/nestattr[@level='outer'] middle",1);
    q("@attr 1=/record/nestattr[@level='outer'] late",1);
    q("@attr 1=/record/nestattr[@level='outer'] after",0);

    q("@attr 1=/record/nestattr[@level='inner'] before",0);
    q("@attr 1=/record/nestattr[@level='inner'] early",0);
    q("@attr 1=/record/nestattr[@level='inner'] middle",0);
    q("@attr 1=/record/nestattr[@level='inner'] late",0);
    q("@attr 1=/record/nestattr[@level='inner'] after",0);

    q("@attr 1=/record/nestattr/nestattr[@level='inner'] before",0);
    q("@attr 1=/record/nestattr/nestattr[@level='inner'] early",0);
    q("@attr 1=/record/nestattr/nestattr[@level='inner'] middle",1);
    q("@attr 1=/record/nestattr/nestattr[@level='inner'] late",0);
    q("@attr 1=/record/nestattr/nestattr[@level='inner'] after",0);

    return close_down(zh, zs, 0);
}
