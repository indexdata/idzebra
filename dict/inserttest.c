/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <yaz/log.h>
#include <yaz/test.h>
#include <yaz/options.h>
#include <idzebra/dict.h>

int main(int argc, char **argv)
{
    BFiles bfs = 0;
    Dict dict = 0;
    long no = 100000000L;

    YAZ_CHECK_INIT(argc, argv);

    bfs = bfs_create(".:40G", 0);
    YAZ_CHECK(bfs);
    if (bfs)
    {
        bf_reset(bfs);
        dict = dict_open(bfs, "dict", 50, 1, 0, 4096);
        YAZ_CHECK(dict);
    }

    srandom(9);
    if (dict)
    {
        long i;
        for (i = 0; i < no; i++)
        {
            char lex[100];
            char userinfo[100];
            int userlen = 4;
            int sz = 1 + (random() % 150);
            int j;
            for (j = 0; j < sz; j++)
            {
                lex[j] = 1 + (random() & 127L);
            }
            lex[j] = 0;
            for (j = 0; j < userlen; j++)
            {
                userinfo[j] = sz;
            }
            dict_insert(dict, lex, userlen, userinfo);
            if ((i % 1000000L) == 0L)
            {
                printf("[%ld]", i);
                fflush(stdout);
            }
        }
        dict_close(dict);
    }

    if (bfs)
        bfs_destroy(bfs);
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
