/* $Id: res-test.c,v 1.8 2002-08-02 19:26:57 adam Exp $
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

#include <zebrautl.h>

static void res_print (const char *name, const char *value)
{
    logf (LOG_LOG, "%s=%s", name, value);
}

int main(int argc, char **argv)
{
    char *arg;
    char *resfile = NULL;
    char *prefix = NULL;
    int ret;
    char *prog = *argv;
    Res res;
    int write_flag = 0;

    log_init (LOG_DEFAULT_LEVEL, prog, NULL);
    while ((ret = options ("wp:v", argv, argc, &arg)) != -2)
        if (ret == 0)
            resfile = arg;
        else if (ret == 'v')
            log_init (LOG_ALL, prog, NULL);
        else if (ret == 'p')
            prefix = arg;
        else if (ret == 'w')
            write_flag = 1;
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    if (!resfile)
    {
        logf (LOG_FATAL, "No resource file given.");
        exit (1);
    }
    res = res_open (resfile);
    res_trav (res, prefix, res_print);
    if (write_flag)
        res_write (res);
    res_close (res);
    return 0;
}
