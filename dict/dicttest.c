/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dicttest.c,v $
 * Revision 1.2  1994-08-18 12:40:54  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.1  1994/08/16  16:26:47  adam
 * Added dict.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dict.h>

char *prog;
Dict dict;

int main (int argc, char **argv)
{
    const char *name = NULL;
    const char *inputfile = NULL;
    int rw = 0;
    int cache = 10;
    int ret;
    char *arg;
    
    prog = argv[0];
    log_init (LOG_DEFAULT_LEVEL, prog, NULL);
    if (argc < 2)
    {
        fprintf (stderr, "usage:\n"
                         "  %s [-v n] [-i f] [-w] [-c n] file\n", prog);
        exit (1);
    }
    while ((ret = options ("v:i:wc:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (name)
            {
                log (LOG_FATAL, "too many files specified\n");
                exit (1);
            }
            name = arg;
        }
        else if (ret == 'c')
        {
            cache = atoi(arg);
            if (cache<2)
                cache = 2;
        }
        else if (ret == 'w')
            rw = 1;
        else if (ret == 'i')
        {
            inputfile = arg;
            rw = 1;
        }
        else if (ret == 'v')
        {
            log_init (atoi(arg), prog, NULL);
        }
        else
        {
            log (LOG_FATAL, "unknown option");
            exit (1);
        }
    }
    if (!name)
    {
        log (LOG_FATAL, "no dictionary file given");
        exit (1);
    }
    dict = dict_open (name, cache, rw);
    if (!dict)
    {
        log (LOG_FATAL, "dict_open fail");
        exit (1);
    }
    if (inputfile)
    {
        FILE *ipf;
        char ipf_buf[256];
        char word[256];
        int i, line = 1;

        if (!(ipf = fopen(inputfile, "r")))
        {
            log (LOG_FATAL|LOG_ERRNO, "cannot open %s", inputfile);
            exit (1);
        }
        
        while (fgets (ipf_buf, 255, ipf))
        {
            for (i=0; i<255; i++)
                if (ipf_buf[i] > ' ')
                    word[i] = ipf_buf[i];
                else
                    break;
            word[i] = 0;
            if (i)
                dict_insert (dict, word, &line);
            ++line;
        }
        fclose (ipf);
    }
    dict_close (dict);
    return 0;
}
