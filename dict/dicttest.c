/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dicttest.c,v $
 * Revision 1.3  1994-09-01 17:44:06  adam
 * depend include change.
 * CVS ----------------------------------------------------------------------
 *
 * Revision 1.2  1994/08/18  12:40:54  adam
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
    const char *base = NULL;
    int rw = 0;
    int infosize = 2;
    int cache = 10;
    int ret;
    char *arg;
    
    prog = argv[0];
    if (argc < 2)
    {
        fprintf (stderr, "usage:\n"
                 "  %s [-s n] [-v n] [-i f] [-w] [-c n] base file\n",
                 prog);
        exit (1);
    }
    while ((ret = options ("s:v:i:wc:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!base)
                base = arg;
            else if (!name)
                name = arg;
            else
            {
                log (LOG_FATAL, "too many files specified\n");
                exit (1);
            }
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
        else if (ret == 's')
        {
            infosize = atoi(arg);
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
    if (!base || !name)
    {
        log (LOG_FATAL, "no base and/or dictionary specified");
        exit (1);
    }
    common_resource = res_open (base);
    if (!common_resource)
    {
        log (LOG_FATAL, "cannot open resource `%s'", base);
        exit (1);
    }
    dict = dict_open (name, cache, rw);
    if (!dict)
    {
        log (LOG_FATAL, "dict_open fail of `%s'", name);
        exit (1);
    }
    if (inputfile)
    {
        FILE *ipf;
        char ipf_buf[256];
        char word[256];
        int line = 1;
        char infobytes[120];
        memset (infobytes, 0, 120);

        if (!(ipf = fopen(inputfile, "r")))
        {
            log (LOG_FATAL|LOG_ERRNO, "cannot open %s", inputfile);
            exit (1);
        }
        
        while (fgets (ipf_buf, 255, ipf))
        {
            if (sscanf (ipf_buf, "%s", word) == 1)
            {
                sprintf (infobytes, "%d", line);
                dict_insert (dict, word, infosize, infobytes);
            }
            ++line;
        }
        fclose (ipf);
    }
    dict_close (dict);
    res_close (common_resource);
    return 0;
}
