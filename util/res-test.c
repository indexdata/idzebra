/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res-test.c,v $
 * Revision 1.1  1994-08-18 09:43:51  adam
 * Development of resource manager. Only missing is res_write.
 *
 */

#include <stdio.h>
#include <util.h>

static void res_print (const char *name, const char *value)
{
    printf ("%s=%s\n", name, value);
}

int main(int argc, char **argv)
{
    char *arg;
    char *resfile = NULL;
    int ret;
    int verboselevel = LOG_DEFAULT_LEVEL;
    char *prog = *argv;
    Res res;


    while ((ret = options ("v", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
            resfile = arg;
        else if (ret == 'v')
            verboselevel = LOG_ALL;
    }
    log_init (verboselevel, prog, NULL);

    if (!resfile)
    {
        log (LOG_FATAL, "Now resource file given.");
        exit (1);
    }
    res = res_open (resfile);
    res_trav (res, "p", res_print);
    res_close (res);
    return 0;
}
