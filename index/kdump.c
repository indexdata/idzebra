/* $Id: kdump.c,v 1.25 2004-08-04 08:35:23 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <assert.h>

#include <charmap.h>
#include "index.h"

char *prog;

#if IT_KEY_NEW
int main(int argc, char **argv)
{
    exit(0);
}
#else
int key_file_decode (FILE *f)
{
    int c, d;

    c = getc (f);
    switch (c & 192) 
    {
    case 0:
        d = c;
        break;
    case 64:
        d = ((c&63) << 8) + (getc (f) & 0xff);
        break;
    case 128:
        d = ((c&63) << 8) + (getc (f) & 0xff);
        d = (d << 8) + (getc (f) & 0xff);
        break;
    case 192:
        d = ((c&63) << 8) + (getc (f) & 0xff);
        d = (d << 8) + (getc (f) & 0xff);
        d = (d << 8) + (getc (f) & 0xff);
        break;
    default:
        d = 0;
        assert (0);
    }
    return d;
}


static int read_one (FILE *inf, char *name, char *key, struct it_key *prevk)
{
    int c;
    int i = 0;
    struct it_key itkey;
    do
    {
        if ((c=getc(inf)) == EOF)
            return 0;
        name[i++] = c;
    } while (c);
    if (i > 1)
        prevk->sysno = 0;
    c = key_file_decode (inf);
    key[0] = c & 1;
    c = c >> 1;
    itkey.sysno = c + prevk->sysno;
    if (c)
    {
        prevk->sysno = itkey.sysno;
        prevk->seqno = 0;
    }
    c = key_file_decode (inf);
    itkey.seqno = c + prevk->seqno;
    prevk->seqno = itkey.seqno;

    memcpy (key+1, &itkey, sizeof(itkey));
    return 1;
}


int main (int argc, char **argv)
{
    int ret;
    char *arg;
    char *key_fname = NULL;
    char key_string[IT_MAX_WORD];
    char key_info[256];
    ZebraMaps zm;
    FILE *inf;
    Res res = NULL;
    struct it_key prevk;

    prevk.sysno = 0;
    prevk.seqno = 0;

    prog = *argv;
    while ((ret = options ("c:v:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            key_fname = arg;
        }
        else if (ret == 'v')
        {
            yaz_log_init (yaz_log_mask_str(arg), prog, NULL);
        }
	else if (ret == 'c')
	{
	    if (!(res = res_open (arg, 0, 0)))
            {
		logf(LOG_FATAL, "Failed to open resource file %s", arg);
	        exit (1);
	    }
	}
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!key_fname)
    {
        fprintf (stderr, "kdump [-c config] [-v log] file\n");
        exit (1);
    }
    if (!res)
        res = res_open ("zebra.cfg", 0, 0);
    zm = zebra_maps_open (res, 0);
    if (!(inf = fopen (key_fname, "r")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", key_fname);
        exit (1);
    }
    printf ("t  rg op  sysno seqno txt\n");
    while (read_one (inf, key_string, key_info, &prevk))
    {
        struct it_key k;
        int op;
	char keybuf[IT_MAX_WORD+1];
	char *to = keybuf;
	const char *from = key_string;
        int usedb_type = from[0];
        int reg_type = from[1];

        op = key_info[0];
        memcpy (&k, 1+key_info, sizeof(k));

	from += 2;  
	while (*from)
	{
	    const char *res = zebra_maps_output (zm, reg_type, &from);
	    if (!res)
		*to++ = *from++;
	    else
		while (*res)
		    *to++ = *res++;
	}
	*to = '\0';
        printf ("%c %3d %c %7d %5d %s\n", reg_type, usedb_type, op ? 'i':'d',
		k.sysno, k.seqno, keybuf);
    }
    zebra_maps_close (zm);
    if (fclose (inf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fclose %s", key_fname);
        exit (1);
    }
    exit (0);
}
#endif    
