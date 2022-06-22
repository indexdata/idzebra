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
#include "../../index/index.h"

char *prog = "";

int tst_encode(int num)
{
    struct it_key key;
    int i;
    void *codec_handle =iscz1_start();
    char *dst_buf = malloc(200 + num * 10);
    char *dst = dst_buf;
    if (!dst_buf)
    {
        printf ("%s: out of memory (num=%d)\n", prog, num);
        return 10;
    }

    for (i = 0; i<num; i++)
    {
        const char *src = (const char *) &key;

        key.len = 2;
        key.mem[0] = i << 8;
        key.mem[1] = i & 255;
        iscz1_encode (codec_handle, &dst, &src);
        if (dst > dst_buf + num*10)
        {
            printf ("%s: i=%d size overflow\n", prog, i);
            return 1;
        }
    }
    iscz1_stop(codec_handle);

    codec_handle =iscz1_start();

    if (1)
    {
        const char *src = dst_buf;
        for (i = 0; i<num; i++)
        {
            char *dst = (char *) &key;
            const char *src0 = src;
            iscz1_decode(codec_handle, &dst, &src);

            if (key.len != 2)
            {
                printf ("%s: i=%d key.len=%d expected 2\n", prog,
                        i, key.len);
                while (src0 != src)
                {
                    printf (" %02X (%d decimal)", *src0, *src0);
                    src0++;
                }
                printf ("\n");
                return 2;
            }
            if (key.mem[0] != (i<<8))
            {
                printf ("%s: i=%d mem[0]=" ZINT_FORMAT " expected "
                        "%d\n", prog, i, key.mem[0], i>>8);
                while (src0 != src)
                {
                    printf (" %02X (%d decimal)", *src0, *src0);
                    src0++;
                }
                printf ("\n");
                return 3;
            }
            if (key.mem[1] != (i&255))
            {
                printf ("%s: i=%d mem[0]=" ZINT_FORMAT " expected %d\n",
                        prog, i, key.mem[1], i&255);
                while (src0 != src)
                {
                    printf (" %02X (%d decimal)", *src0, *src0);
                    src0++;
                }
                printf ("\n");
                return 4;
            }
        }
    }

    iscz1_stop(codec_handle);
    free(dst_buf);
    return 0;
}

void tstcodec1(void)
{
    char buf[100];
    char *dst = buf;
    const char *src;
    struct it_key key1;
    struct it_key key2;
    void *codec_handle =iscz1_start();

    memset(&key1, 0, sizeof(key1));
    memset(&key2, 0, sizeof(key2));

    key1.len = 4;
    key1.mem[0] = 4*65536+1016;
    key1.mem[1] = 24339;
    key1.mem[2] = 125060;
    key1.mem[3] = 1;

    src = (char*) &key1;
    dst = buf;
    iscz1_encode(codec_handle, &dst, &src);

    iscz1_stop(codec_handle);

    codec_handle =iscz1_start();

    dst = (char*) &key2;
    src = buf;

    iscz1_decode(codec_handle, &dst, &src);

    iscz1_stop(codec_handle);

    if (memcmp(&key1, &key2, sizeof(key1)))
    {
        const char *cp1 = (char*) &key1;
        const char *cp2 = (char*) &key2;
        int i;
        for (i = 0; i<sizeof(key1); i++)
            printf ("offset=%d char1=%d char2=%d\n", i, cp1[i], cp2[i]);
    }
}

int tstcodec2(int num)
{
    int errors = 0;
    int i;
    int max = 2048;
    int neg = 0;  /* iscz1_{en,de}code does not handle negative numbers */
    void *encode_handle =iscz1_start();
    void *decode_handle =iscz1_start();

    srand(12);
    for (i = 0; i<num; i++)
    {
        struct it_key ar1, ar2;
        int j;
        ar1.len = 1;

        for (j = 0; j<ar1.len; j++)
        {
            int r = (rand() % max) - neg;
            ar1.mem[j] = r;
        }
        if (1)
        {
            char dstbuf[100];
            const char *src = (const char *) &ar1;
            char *dst = dstbuf;
            iscz1_encode(encode_handle, &dst, &src);

            src = dstbuf;
            dst = (char *) &ar2;
            iscz1_decode(decode_handle, &dst, &src);
        }

        if (ar1.len != ar2.len)
        {
            printf("tstcodec2: length does not match\n");
            errors++;
        }
        for (j = 0; j<ar1.len; j++)
        {
            if (ar1.mem[j] != ar2.mem[j])
            {
                printf("diff:\n");
                for (j = 0; j<ar1.len; j++)
                    printf(" %d " ZINT_FORMAT " " ZINT_FORMAT "\n",
                           j, ar1.mem[j], ar2.mem[j]);
                errors++;
                break;
            }
        }
    }
    iscz1_stop(encode_handle);
    iscz1_stop(decode_handle);
    return errors;
}


int main(int argc, char **argv)
{
    int num = 0;
    int ret;
    prog = *argv;
    if (argc > 1)
        num = atoi(argv[1]);
    if (num < 1 || num > 100000000)
        num = 10000;
    tstcodec1();
    ret = tstcodec2(500);
    ret = tst_encode(num);
    exit(ret);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

