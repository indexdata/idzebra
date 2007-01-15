/* $Id: tstmfile1.c,v 1.3 2007-01-15 15:10:14 adam Exp $
   Copyright (C) 1995-2007
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

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <yaz/test.h>
#include "mfile.h"

#define BLOCK_SIZE 16

void tst1(void)
{
    MFile_area a = mf_init("main", 0 /* spec */, 0 /* base */, 0 /* only sh */);
    YAZ_CHECK(a);
    mf_destroy(a);
}

void tst2(void)
{
    char buf[BLOCK_SIZE];
    MFile_area a = mf_init("main", 0 /* spec */, 0 /* base */, 0 /* only sh */);
    MFile f;

    YAZ_CHECK(a);

    mf_reset(a, 1);

    f = mf_open(a, "mymfile", BLOCK_SIZE, 1);
    YAZ_CHECK(f);

    YAZ_CHECK_EQ(mf_read(f, 0, 0, 0, buf), 0);
    
    memset(buf, 'a', BLOCK_SIZE);
    YAZ_CHECK_EQ(mf_write(f, 0, 0, 0, buf), 0);

    YAZ_CHECK_EQ(mf_read(f, 0, 0, 0, buf), 1);

    mf_close(f);

    mf_destroy(a);
}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    tst1();
    tst2();
    YAZ_CHECK_TERM;
}

