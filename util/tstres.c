/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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
#include <stdlib.h>
#include <idzebra/res.h>
#include <yaz/test.h>
#include <yaz/snprintf.h>

/* use env srcdir as base directory - or current directory if unset */
const char *get_srcdir(void)
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir=".";
    return srcdir;

}


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
        char path[1024];
        int r;
        
        yaz_snprintf(path, sizeof(path), "%s/tstres.cfg", get_srcdir());
        r = res_read_file(res, path);
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

