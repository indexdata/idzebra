/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kdump.c,v $
 * Revision 1.16  1998-05-20 10:12:17  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.15  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.14  1997/10/27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.13  1997/09/09 13:38:07  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.12  1997/09/05 09:52:32  adam
 * Extra argument added to function chr_read_maptab (tab path).
 *
 * Revision 1.11  1996/10/29 14:06:49  adam
 * Include zebrautl.h instead of alexutil.h.
 *
 * Revision 1.10  1996/06/04 14:56:12  quinn
 * Fix
 *
 * Revision 1.9  1996/06/04  14:18:53  quinn
 * Charmap work
 *
 * Revision 1.8  1996/06/04  10:18:59  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.7  1995/10/10  12:24:38  adam
 * Temporary sort files are compressed.
 *
 * Revision 1.6  1995/09/29  14:01:42  adam
 * Bug fixes.
 *
 * Revision 1.5  1995/09/11  13:09:35  adam
 * More work on relevance feedback.
 *
 * Revision 1.4  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.3  1995/09/06  16:11:17  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/04  12:33:42  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:36  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif
#include <assert.h>

#include <charmap.h>
#include "index.h"

char *prog;


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
            log_init (log_mask_str(arg), prog, NULL);
        }
	else if (ret == 'c')
	{
	    if (!(res = res_open (arg)))
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
        res = res_open ("zebra.cfg");
    zm = zebra_maps_open (res);
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
	    while (*res)
		*(to++) = *(res++);
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
