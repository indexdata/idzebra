/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dictext.c,v $
 * Revision 1.5  1996-01-31 21:03:59  adam
 * Extra options.
 *
 * Revision 1.4  1995/09/04  12:33:31  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.3  1994/10/04  17:46:54  adam
 * Function options now returns arg with error option.
 *
 * Revision 1.2  1994/09/28  13:07:08  adam
 * Use log_mask_str now.
 *
 * Revision 1.1  1994/09/16  15:39:11  adam
 * Initial code of lookup - not tested yet.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <alexutil.h>

char *prog;

int main (int argc, char **argv)
{
    char *arg;
    int ret;
    int use8 = 0;
    char *inputfile = NULL;
    FILE *ipf = stdin;
    char ipf_buf[1024];

    prog = *argv;
    while ((ret = options ("8vh", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!inputfile)
                inputfile = arg;
            else
            {
                logf (LOG_FATAL, "too many files specified\n");
                exit (1);
            }
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else if (ret == 'h')
        {
            fprintf (stderr, "usage:\n"
                     "  %s [-8] [-h] [-v n] [file]\n", prog);
            exit (1);
        }
        else if (ret == '8')
            use8 = 1;
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (inputfile)
    {
        ipf = fopen (inputfile, "r");
        if (!ipf)
        {
            logf (LOG_FATAL|LOG_ERRNO, "cannot open '%s'", inputfile);
            exit (1);
        }
    }
    while (fgets (ipf_buf, 1023, ipf))
    {
        char *ipf_ptr = ipf_buf;
        for (;*ipf_ptr && *ipf_ptr != '\n';ipf_ptr++)
        {
            if ((use8 && *ipf_ptr<0)
                || (*ipf_ptr > 0 && isalpha(*ipf_ptr))
                || *ipf_ptr == '_')
            {
                int i = 1;
                while (ipf_ptr[i] && ((use8 && ipf_ptr[i] < 0)
                                      || (ipf_ptr[i]>0 && isalnum(ipf_ptr[i]))
                                      || ipf_ptr[i] == '_'))
                    i++;
                if (ipf_ptr[i])
                    ipf_ptr[i++] = '\0';
                printf ("%s\n", ipf_ptr);
                ipf_ptr += (i-1);
            }
        }
    }
    return 0;
}



