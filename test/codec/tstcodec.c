/* $Id: tstcodec.c,v 1.3 2004-09-15 08:13:51 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

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
	key.mem[0] = i >> 8;
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
	    if (key.mem[0] != (i>>8))
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

int main(int argc, char **argv)
{
    int num = 0;
    prog = *argv;
    if (argc > 1)
	num = atoi(argv[1]);
    if (num < 1 || num > 100000000)
	num = 10000;
    exit(tst_encode(num));
}
    
