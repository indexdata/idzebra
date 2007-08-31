/* $Id: tstres.c,v 1.3 2007-08-31 21:12:51 mike Exp $
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

#include <string.h>
#include <idzebra/res.h>
#include <yaz/test.h>

static void tst_res_open(void)
{
    Res res = 0;

    res_close(res);

    res = res_open(0, 0);
    YAZ_CHECK(res);
    res_close(res);
}


static void tst_res_read_file(void)
{
    Res res = res_open(0, 0);

    YAZ_CHECK(res);
    if (res)
    {
        int r = res_read_file(res, "notfound");
        YAZ_CHECK_EQ(r, ZEBRA_FAIL);
    }
    if (res)
    {
        const char *v;
        int r = res_read_file(res, "tstres.cfg");
        YAZ_CHECK_EQ(r, ZEBRA_OK);

        v = res_get_def(res, "register", "none");
        YAZ_CHECK(!strcmp(v, "a:b"));

        v = res_get_def(res, "name", "none");
        YAZ_CHECK(!strcmp(v, "c d"));

        v = res_get_def(res, "here", "none");
        YAZ_CHECK(!strcmp(v, "_"));

        v = res_get_def(res, "namex", "none");
        YAZ_CHECK(!strcmp(v, "none"));
    }
    res_close(res);
}

int main (int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    tst_res_open();
    tst_res_read_file();
    YAZ_CHECK_TERM;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

