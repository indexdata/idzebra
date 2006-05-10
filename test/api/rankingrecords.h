/* $Id: rankingrecords.h,v 1.3 2006-05-10 08:13:35 adam Exp $
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

/** rankingrecords.h - some test data for t9, and t10 */

const char *recs[] = {
        "<gils>\n"
        "  <title>The first title</title>\n"
        "  <abstract> \n"
        "    The first common word is the: the the the \n"
        "    The second common word is word \n"
        "    but all have the foo bar \n"
        "  </abstract>\n"
        "</gils>\n",

        "<gils>\n"
        "  <title>The second title</title>\n"
        "  <abstract> \n"
        "    The first common word is the: the \n"
        "    The second common word is foo: foo foo \n"
        "    but all have the foo bar \n"
        "  </abstract>\n"
        "</gils>\n",

        "<gils>\n"
        "  <title>The third title</title>\n"
        "  <abstract> \n"
        "    The first common word is the: the \n"
        "    The third common word is bar: bar \n"
        "    but all have the foo bar \n"
        "  </abstract>\n"
        "</gils>\n",
    
        0 };


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

