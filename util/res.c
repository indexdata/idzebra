/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.c,v $
 * Revision 1.1  1994-08-17 15:34:23  adam
 * Initial version of resource manager.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

struct res {
    char *name;
    char *value;
    struct res *next;
};

static struct res *first = NULL, *last = NULL;
static int res_init = 0;

static FILE *fr;

const char *alex_path (const char *name)
{
    static char path[256];
    char *alex_prefix;

    if (!(alex_prefix = getenv ("ALEXPREFIX")))
        alex_prefix = "./";
    
    if (*alex_prefix && alex_prefix[strlen(alex_prefix)-1] == '/')
        sprintf (path, "%s%s", alex_prefix, name);
    else
        sprintf (path, "%s/%s", alex_prefix, name);
    return path;
}

static void reread (void)
{
    struct res *resp;
    char *line;
    char *alex_base;
    char *val_buf;
    int val_size, val_max = 1024;
    char path[256];
    char fr_buf[1024];

    res_init = 1;

    val_buf = xmalloc (val_max);

    if (!(alex_base = getenv ("ALEXBASE")))
        alex_base = "base";

    strcpy (path, alex_path(alex_base));

    fr = fopen (path, "r");
    if (!fr)
    {
        log (LOG_FATAL, "cannot open %s", path);
        exit (1);
    }
    while (1)
    {
        line = fgets (fr_buf, 1023, fr);
        if (!line)
            break;
        if (*line == '#')
        {
            int no = 0;

            while (fr_buf[no] && fr_buf[no] != '\n')
                no++;
            fr_buf[no] = '\0';

            if (!first)
                resp = last = first = xmalloc (sizeof(*resp));
            else
            {
                resp = xmalloc (sizeof(*resp));
                last->next = resp;
                last = resp;
            }
            resp->next = NULL;
            resp->name = xmalloc (no+1);
            resp->value = NULL;
            strcpy (resp->name, fr_buf);
        }
        else
        {
            int no = 0;
            while (1)
            {
                if (fr_buf[no] == 0 || fr_buf[no] == '\n' )
                {
                    no = -1;
                    break;
                }
                if (fr_buf[no] == ':')
                    break;
                no++;
            }
            if (no < 0)
                continue;
            fr_buf[no++] = '\0';
            if (!first)
                resp = last = first = xmalloc (sizeof(*resp));
            else
            {
                resp = xmalloc (sizeof(*resp));
                last->next = resp;
                last = resp;
            }
            resp->next = NULL;
            resp->name = xmalloc (no);
            strcpy (resp->name, fr_buf);
            
            while (fr_buf[no] == ' ')
                no++;
            val_size = 0;
            while (1)
            {
                if (fr_buf[no] == '\0' || fr_buf[no] == '\n')
                {
                    val_buf[val_size++] = '\0';
                    resp->value = xmalloc (val_size);
                    strcpy (resp->value, val_buf);
                    break;
                }
                else if (fr_buf[no] == '\\' && fr_buf[no+1] == '\n')
                {
                    line = fgets (fr_buf, 1023, fr);
                    if (!line)
                    {
                        resp->value = xmalloc (val_size);
                        strcpy (resp->value, val_buf);
                        break;
                    }
                    no = 0;
                }
                else
                    val_buf[val_size] = fr_buf[no++];
            }
        }
    }                
    xfree (val_buf);
    fclose (fr);
}


const char *res_get (const char *name)
{
    if (!res_init)
        reread ();
    return NULL;
}

const char *res_put (const char *name, const char *value)
{
    if (!res_init)
        reread ();
    return NULL;
}
    
int res_write (void)
{
    if (!res_init)
        reread ();
    return 0;
}



