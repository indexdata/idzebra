/*
 * Copyright (C) 1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zserver.c,v $
 * Revision 1.1  1995-09-04 09:10:41  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <util.h>
#include "index.h"

char *prog;

int main (int argc, char **argv)
{
    int ret;
    char *arg;
    char *base_name = NULL;

    prog = *argv;
    while ((ret = options ("v:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!base_name)
            {
                base_name = arg;

                common_resource = res_open (base_name);
                if (!common_resource)
                {
                    log (LOG_FATAL, "Cannot open resource `%s'", base_name);
                    exit (1);
                }
            }
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else
        {
            log (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!base_name)
    {
        fprintf (stderr, "search [-v log] base ...\n");
        exit (1);
    }
    exit (0);
}
