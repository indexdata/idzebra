/* $Id: t1.c,v 1.9 2004-10-28 15:24:36 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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

/** t1 - just start and stop */

#include <stdlib.h>
#include <yaz/log.h>
#include <idzebra/api.h>
#include "testlib.h"

	
int main(int argc, char **argv)
{
    ZebraService zs;
    ZebraHandle zh;

    yaz_log_init_file("t1.log");
    nmem_init();
    zs = start_service(0);
    zh = zebra_open (zs);
    
    zebra_close (zh);
    zebra_stop (zs);
    nmem_exit ();
    xmalloc_trav ("x");
    logf(LOG_LOG,"================ All tests OK ");
    exit (0);
}
