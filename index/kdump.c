/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kdump.c,v $
 * Revision 1.11  1996-10-29 14:06:49  adam
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
#include <assert.h>
#include <unistd.h>
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
    FILE *inf;
    struct it_key prevk;
    chrmaptab *map = 0;

    prevk.sysno = 0;
    prevk.seqno = 0;

    prog = *argv;
    while ((ret = options ("m:v:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            key_fname = arg;
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
	else if (ret == 'm')
	{
	    if (!(map = chr_read_maptab(arg)))
	    {
		logf(LOG_FATAL, "Failed to open maptab");
		exit(1);
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
        fprintf (stderr, "kdump [-m maptab -v log] file\n");
        exit (1);
    }
    if (!(inf = fopen (key_fname, "r")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", key_fname);
        exit (1);
    }
    while (read_one (inf, key_string, key_info, &prevk))
    {
        struct it_key k;
        int op;
	char keybuf[IT_MAX_WORD+1];

        op = key_info[0];
        memcpy (&k, 1+key_info, sizeof(k));
	if (map)
	{
	    char *to = keybuf, *from = key_string;

	    while (*from)
	    {
		char *res = (char*)map->output[(unsigned char) *(from++)];
		while (*res)
		    *(to++) = *(res++);
	    }
	    *to = '\0';
	}
	else
	    strcpy(keybuf, key_string);
        printf ("%7d op=%d s=%-5d %s\n", k.sysno, op, k.seqno,
                keybuf);
    }
    if (fclose (inf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fclose %s", key_fname);
        exit (1);
    }
    
    exit (0);
}
