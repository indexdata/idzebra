/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kdump.c,v $
 * Revision 1.2  1995-09-04 12:33:42  adam
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
    char key_string[1000];
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
        printf ("%s sysno=%d op=%d\n", key_string, k.sysno, *key_info);
    }
    if (fclose (inf))
    {
        logf (LOG_FATAL|LOG_ERRNO, "fclose %s", key_fname);
        exit (1);
    }
    
    exit (0);
}
