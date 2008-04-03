/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

#include <zebra_strmap.h>
#include <yaz/test.h>
#include <string.h>
#include <stdlib.h>

static void test1(void)
{
    {
        zebra_strmap_t sm = zebra_strmap_create();
        YAZ_CHECK(sm);
        zebra_strmap_destroy(sm);
    }
    {
        int v = 1;
        void *data_buf;
        size_t data_len;
        zebra_strmap_t sm = zebra_strmap_create();
        YAZ_CHECK(!zebra_strmap_lookup(sm, "a", 0, 0));
        
        zebra_strmap_add(sm, "a", &v, sizeof v);
        data_buf = zebra_strmap_lookup(sm, "a", 0, &data_len);
        YAZ_CHECK(data_buf && data_len == sizeof v 
                  && v == *((int*) data_buf));

        zebra_strmap_remove(sm, "a");
        data_buf = zebra_strmap_lookup(sm, "a", 0, &data_len);
        YAZ_CHECK(data_buf == 0);

        v = 1;
        zebra_strmap_add(sm, "a", &v, sizeof v);

        v = 2;
        zebra_strmap_add(sm, "b", &v, sizeof v);

        v = 3;
        zebra_strmap_add(sm, "c", &v, sizeof v);

        {
            zebra_strmap_it it = zebra_strmap_it_create(sm);
            const char *name;
            int no = 0;
            while ((name = zebra_strmap_it_next(it, &data_buf, &data_len)))
            {
                YAZ_CHECK(!strcmp(name, "a") || !strcmp(name, "b") ||
                          !strcmp(name, "c"));
                no++;
            }
            YAZ_CHECK_EQ(no, 3);
            zebra_strmap_it_destroy(it);
        }
        zebra_strmap_destroy(sm);
    }
}

static void test2(int no_iter)
{
    zebra_strmap_t sm = zebra_strmap_create();
    {
        int i;
        srand(12);
        for (i = 0; i < no_iter; i++)
        {
            char str[8];
            int j;
            int v = i;

            for (j = 0; j < sizeof(str)-1; j++)
                str[j] = rand() & 255;
            str[j] = '\0';

            zebra_strmap_add(sm, str, &v, sizeof v);
        }
    }
    {
        int failed = 0;
        int i;
        srand(12);
        for (i = 0; i < no_iter; i++)
        {
            char str[8];
            int j;
            int v = i;
            void *data_buf;
            size_t data_len;

            for (j = 0; j < sizeof(str)-1; j++)
                str[j] = rand() & 255;
            str[j] = '\0';

            j = 0;
            while ((data_buf = zebra_strmap_lookup(sm, str, j, &data_len)))
            {
                if (data_len == sizeof v && v == *((int*) data_buf))
                    break;
                j++;
            }
            if (!(data_buf && data_len == sizeof v
                  && v == *((int*) data_buf)))
                failed++;
        }
        if (failed)
            YAZ_CHECK_EQ(failed, 0);
    }
    zebra_strmap_destroy(sm);
}

int main (int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    test1();
    test2(50000);
    YAZ_CHECK_TERM;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

