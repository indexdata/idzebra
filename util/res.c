/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.c,v $
 * Revision 1.2  1994-08-18 08:23:26  adam
 * Res.c now use handles. xmalloc defines xstrdup.
 *
 * Revision 1.1  1994/08/17  15:34:23  adam
 * Initial version of resource manager.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

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

static void reread (Res r)
{
    struct res_entry *resp;
    char *line;
    char *val_buf;
    int val_size, val_max = 1024;
    char path[256];
    char fr_buf[1024];
    FILE *fr;

    r->init = 1;

    val_buf = xmalloc (val_max);

    strcpy (path, alex_path(r->name));

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

            if (!r->first)
                resp = r->last = r->first = xmalloc (sizeof(*resp));
            else
            {
                resp = xmalloc (sizeof(*resp));
                r->last->next = resp;
                r->last = resp;
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
            if (!r->first)
                resp = r->last = r->first = xmalloc (sizeof(*resp));
            else
            {
                resp = xmalloc (sizeof(*resp));
                r->last->next = resp;
                r->last = resp;
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

Res res_open (const char *name)
{
    Res r = xmalloc (sizeof(*r));
    r->init = 0;
    r->name = xstrdup (name);
    return r;
}

void res_close (Res r)
{
    /* more to xfree... */
    xfree (r);
}

const char *res_get (Res r, const char *name)
{
    if (!r->init)
        reread (r);
    return NULL;
}

const char *res_put (Res r, const char *name, const char *value)
{
    if (!r->init)
        reread (r);
    return NULL;
}
    
int res_write (Res r)
{
    if (!r->init)
        reread (r);
    return 0;
}



