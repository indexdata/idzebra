/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: main.c,v $
 * Revision 1.9  1995-09-14 07:48:24  adam
 * Record control management.
 *
 * Revision 1.8  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.7  1995/09/05  15:28:39  adam
 * More work on search engine.
 *
 * Revision 1.6  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.5  1995/09/04  09:10:39  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 * Revision 1.4  1995/09/01  14:06:36  adam
 * Split of work into more files.
 *
 * Revision 1.3  1995/09/01  10:57:07  adam
 * Minor changes.
 *
 * Revision 1.2  1995/09/01  10:30:24  adam
 * More work on indexing. Not working yet.
 *
 * Revision 1.1  1995/08/31  14:50:24  adam
 * New simple file index tool.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <alexutil.h>
#include "index.h"

char *prog;

int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *base_name = NULL;
    char *base_path = NULL;

    prog = *argv;
    while ((ret = options ("r:v:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!base_name)
            {
                base_name = arg;

                common_resource = res_open (base_name);
                if (!common_resource)
                {
                    logf (LOG_FATAL, "Cannot open resource `%s'", base_name);
                    exit (1);
                }
            }
            else if(cmd == 0) /* command */
            {
                if (!strcmp (arg, "add"))
                {
                    cmd = 'a';
                }
                else if (!strcmp (arg, "del"))
                {
                    cmd = 'd';
                }
                else
                {
                    logf (LOG_FATAL, "Unknown command: %s", arg);
                    exit (1);
                }
            }
            else
            {
                unlink ("keys.tmp");
                key_open ("keys.tmp");
                repository (cmd, arg, base_path);
                cmd = 0;
            }
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else if (ret == 'r')
        {
            base_path = arg;
        }
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!base_name)
    {
        fprintf (stderr, "index [-v log] [-r repository] "
                 "base cmd1 dir1 cmd2 dir2 ...\n");
        exit (1);
    }
    if (!key_close ())
        exit (0);
    logf (LOG_LOG, "Sorting");
    if (!key_sort ("keys.tmp", 3000000))
        exit (0);
    logf (LOG_LOG, "Input");
    key_input (FNAME_WORD_DICT, FNAME_WORD_ISAM, "keys.tmp", 60);
    exit (0);
}

