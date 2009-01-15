/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include <charmap.h>
#include <yaz/test.h>
#include <stdlib.h>

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
    YAZ_CHECK(tab);
    chrmaptab_destroy(tab);
}

void tst2(void)
{
    /* open non-existing nonexist.chr */
    chrmaptab tab = chrmaptab_create(get_srcdir() /* tabpath */,
				     "nonexist.chr" /* file */,
				     0 /* tabroot */ );
    YAZ_CHECK(!tab);
    chrmaptab_destroy(tab);
}

void tst3(void)
{
    /* open empty emptycharmap.chrr */
    chrmaptab tab = chrmaptab_create(get_srcdir() /* tabpath */,
				     "emptycharmap.chr" /* file */,
				     0 /* tabroot */ );
    YAZ_CHECK(!tab);
    chrmaptab_destroy(tab);
}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();

    tst1();
    tst2();
    tst3();

    YAZ_CHECK_TERM;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

