/* $Id: t13.c,v 1.2 2005-09-13 11:51:07 adam Exp $
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

/** test resources */
#include <stdlib.h>
#include <string.h>
#include <idzebra/api.h>

void tst()
{
    Res default_res;  /* default resources */
    Res temp_res;     /* temporary resources */
    ZebraService zs;
    ZebraHandle zh;
    const char *val;
    int i;

    default_res = res_open(0, 0, 0); /* completely empty */
    if (!default_res)
	exit(1);
    res_set(default_res, "name1", "value1");

    temp_res = res_open(0, 0, 0); /* completely empty */
    if (!temp_res)
	exit(1);
    
    zs = zebra_start_res(0, default_res, temp_res);
    if (!zs)
	exit(2);
    
    zh = zebra_open(zs, 0);

    zebra_select_database(zh, "Default");

    /* run this a few times so we can see leaks easier */
    for (i = 0; i<10; i++)
    {
	/* we should find the name1 from default_res */
	val = zebra_get_resource(zh, "name1", 0);
	if (!val || strcmp(val, "value1"))
	    exit(3);

	/* make local override */
	res_set(temp_res, "name1", "value2");
	res_set(temp_res, "name4", "value4");

	/* we should find the name1 from temp_res */
	val = zebra_get_resource(zh, "name1", 0);
	if (!val || strcmp(val, "value2"))
	    exit(4);
	
	val = zebra_get_resource(zh, "name4", 0);
	if (!val || strcmp(val, "value4"))
	    exit(4);
	
	res_clear(temp_res);
    }

    zebra_close(zh);
    zebra_stop(zs);

    res_close(temp_res);
    res_close(default_res);
}

int main(int argc, char **argv)
{
    tst();
    exit(0);
}
