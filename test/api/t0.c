/* $Id: t0.c,v 1.1 2006-02-21 15:23:11 adam Exp $
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

/** t0 - test zebra_start .. */

#include <stdlib.h>
#include "testlib.h"
	
int main(int argc, char **argv)
{
    ZebraService zs;

    start_log(argc, argv);

    zs = zebra_start("xxxxpoiasdfasfd.cfg");
    if (zs)
	exit(1);
    return 0;
}
