/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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



#include <stdio.h>

#include <zebrautl.h>

static void res_print (const char *name, const char *value)
{
    yaz_log (YLOG_LOG, "%s=%s", name, value);
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

    log_init (YLOG_DEFAULT_LEVEL, prog, NULL);
    while ((ret = options ("wp:v", argv, argc, &arg)) != -2)
        if (ret == 0)
            resfile = arg;
        else if (ret == 'v')
            log_init (YLOG_ALL, prog, NULL);
        else if (ret == 'p')
            prefix = arg;
        else if (ret == 'w')
            write_flag = 1;
        else
        {
            yaz_log (YLOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    if (!resfile)
    {
        yaz_log (YLOG_FATAL, "No resource file given.");
        exit (1);
    }
    res = res_open (resfile);
    res_trav (res, prefix, res_print);
    if (write_flag)
        res_write (res);
    res_close (res);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

