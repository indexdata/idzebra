/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res-test.c,v $
 * Revision 1.5  1995-09-04 12:34:05  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.4  1994/10/04  17:47:11  adam
 * Function options now returns arg with error option.
 *
 * Revision 1.3  1994/08/18  11:02:27  adam
 * Implementation of res_write.
 *
 * Revision 1.2  1994/08/18  10:02:01  adam
 * Module alexpath moved from res.c to alexpath.c. Minor changes in res-test.c
 *
 * Revision 1.1  1994/08/18  09:43:51  adam
 * Development of resource manager. Only missing is res_write.
 *
 */

#include <stdio.h>
#include <alexutil.h>

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
