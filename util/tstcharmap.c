/* $Id: tstcharmap.c,v 1.6 2006-10-29 17:20:02 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <charmap.h>
#include <yaz/log.h>

/* use env srcdir as base directory - or current directory if unset */
const char *get_srcdir(void)
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir=".";
    return srcdir;

}

void tst1(void)
{
    /* open existing map chrmaptab.chr */
    chrmaptab tab = chrmaptab_create(get_srcdir() /* tabpath */,
				     "tstcharmap.chr" /* file */,
				     0 /* tabroot */ );
    if (!tab)
	exit(1);
    
    chrmaptab_destroy(tab);
}

void tst2(void)
{
    /* open non-existing nonexist.chr */
    chrmaptab tab = chrmaptab_create(get_srcdir() /* tabpath */,
				     "nonexist.chr" /* file */,
				     0 /* tabroot */ );
    
    if (tab)
	exit(0);
}

int main(int argc, char **argv)
{
    char logname[2048];
    sprintf(logname, "%s.log", argv[0]);
    yaz_log_init_file(logname);

    tst1();
    tst2();

    exit(0);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

