/* $Id: dictext.c,v 1.10 2004-11-19 10:26:55 heikki Exp $
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



#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <zebrautl.h>

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
                yaz_log (YLOG_FATAL, "too many files specified\n");
                exit (1);
            }
        }
        else if (ret == 'v')
        {
            yaz_log_init (yaz_log_mask_str(arg), prog, NULL);
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
            yaz_log (YLOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (inputfile)
    {
        ipf = fopen (inputfile, "r");
        if (!ipf)
        {
            yaz_log (YLOG_FATAL|YLOG_ERRNO, "cannot open '%s'", inputfile);
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



