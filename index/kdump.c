/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kdump.c,v $
 * Revision 1.5  1995-09-11 13:09:35  adam
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

#include <alexutil.h>
#include "index.h"

char *prog;

static int read_one (FILE *inf, char *name, char *key)
{
    int c;
    int i = 0;
    name[0] = 0;
    do
    {
        if ((c=getc(inf)) == EOF)
            return 0;
        name[i++] = c;
    } while (c);
    for (i = 0; i<sizeof(struct it_key)+1; i++)
        ((char *)key)[i] = getc (inf);
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

    prog = *argv;
    while ((ret = options ("v:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            key_fname = arg;
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!key_fname)
    {
        fprintf (stderr, "kdump [-v log] file\n");
        exit (1);
    }
    if (!(inf = fopen (key_fname, "r")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fopen %s", key_fname);
        exit (1);
    }
    while (read_one (inf, key_string, key_info))
    {
        struct it_key k;

        memcpy (&k, key_info+1, sizeof(k));
#if IT_KEY_HAVE_SEQNO
        printf ("%7d op=%d s=%-5d %s\n", k.sysno, *key_info, k.seqno,
                key_string);
#else
        printf ("%7d op=%d f=%-3d %s\n", k.sysno, *key_info, k.freq,
                key_string);

#endif
    }
    if (fclose (inf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fclose %s", key_fname);
        exit (1);
    }
    
    exit (0);
}
