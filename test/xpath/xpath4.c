/* $Id: xpath4.c,v 1.6 2006-05-10 08:13:41 adam Exp $
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


static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);

#if 0
    yaz_log_init_level( yaz_log_mask_str_x("xpath4,rsbetween", LOG_DEFAULT_LEVEL));
#endif

    YAZ_CHECK(tl_init_data(zh, myrec));

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title foo",4));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title bar",2));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title[@lang='da'] foo",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title[@lang='en'] foo",1));

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title[@lang='en'] english",1)); 
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title[@lang='da'] english",0)); 
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title[@lang='da'] danish",1));  
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title[@lang='en'] danish",0));  

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/title @and foo bar",2));
    /* The previous one returns two hits, as the and applies to the whole
    record, so it matches <title>foo</title><title>bar</title>
    This might not have to be like that, but currently that is what
    zebra does.  */
    YAZ_CHECK(tl_query(zh, "@and @attr 1=/record/title foo @attr 1=/record/title bar ",2));

    /* check we get all the occureences for 'grunt' */
    /* this can only be seen in the log, with debugs on. bug #202 */
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/author grunt",3));

    /* check nested tags */
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested before",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested early",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested middle",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested late",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested after",0));

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested/nested before",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested/nested early",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested/nested middle",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested/nested late",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nested/nested after",0));

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='outer'] before",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='outer'] early",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='outer'] middle",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='outer'] late",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='outer'] after",0));

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='inner'] before",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='inner'] early",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='inner'] middle",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='inner'] late",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr[@level='inner'] after",0));

    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr/nestattr[@level='inner'] before",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr/nestattr[@level='inner'] early",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr/nestattr[@level='inner'] middle",1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr/nestattr[@level='inner'] late",0));
    YAZ_CHECK(tl_query(zh, "@attr 1=/record/nestattr/nestattr[@level='inner'] after",0));

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

