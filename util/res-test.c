/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res-test.c,v $
 * Revision 1.2  1994-08-18 10:02:01  adam
 * Module alexpath moved from res.c to alexpath.c. Minor changes in res-test.c
 *
 * Revision 1.1  1994/08/18  09:43:51  adam
 * Development of resource manager. Only missing is res_write.
 *
 */

#include <stdio.h>
#include <util.h>

static void res_print (const char *name, const char *value)
{
    log (LOG_LOG, "%s=%s", name, value);
}

int main(int argc, char **argv)
{
    char *arg;
    char *resfile = NULL;
    char *prefix = NULL;
    int ret;
    char *prog = *argv;
    Res res;

    log_init (LOG_DEFAULT_LEVEL, prog, NULL);
    while ((ret = options ("p:v", argv, argc, &arg)) != -2)
        if (ret == 0)
            resfile = arg;
        else if (ret == 'v')
            log_init (LOG_ALL, prog, NULL);
        else if (ret == 'p')
            prefix = arg;
        else
        {
            log (LOG_FATAL, "unknown option");
            exit (1);
        }
    if (!resfile)
    {
        log (LOG_FATAL, "Now resource file given.");
        exit (1);
    }
    res = res_open (resfile);
    res_trav (res, prefix, res_print);
    res_close (res);
    return 0;
}
