/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.c,v $
 * Revision 1.4  1994-08-18 10:02:01  adam
 * Module alexpath moved from res.c to alexpath.c. Minor changes in res-test.c
 *
 * Revision 1.3  1994/08/18  09:43:51  adam
 * Development of resource manager. Only missing is res_write.
 *
 * Revision 1.2  1994/08/18  08:23:26  adam
 * Res.c now use handles. xmalloc defines xstrdup.
 *
 * Revision 1.1  1994/08/17  15:34:23  adam
 * Initial version of resource manager.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

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
        log (LOG_FATAL|LOG_ERRNO, "cannot open %s", path);
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
                    log (LOG_DEBUG, "(name=%s,value=%s)",
                         resp->name, resp->value);
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
                    val_buf[val_size++] = fr_buf[no++];
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
    if (r->init)
    {
        struct res_entry *re, *re1;
        for (re = r->first; re; re=re1)
        {
            if (re->name)
                xfree (re->name);
            if (re->value)
                xfree (re->value);
            re1 = re->next;
            xfree (re);
        }
    }
    xfree (r);
}

const char *res_get (Res r, const char *name)
{
    struct res_entry *re;
    if (!r->init)
        reread (r);
    
    for (re = r->first; re; re=re->next)
        if (re->value && !strcmp (re->name, name))
            return re->value;
    return NULL;
}

void res_put (Res r, const char *name, const char *value)
{
    struct res_entry *re;
    if (!r->init)
        reread (r);

    for (re = r->first; re; re=re->next)
        if (re->value && !strcmp (re->name, name))
        {
            xfree (re->value);
            re->value = xstrdup (value);
            return;
        }
    if (!r->first)
        re = r->last = r->first = xmalloc (sizeof(*re));
    else
    {
        re = xmalloc (sizeof(*re));
        r->last->next = re;
        r->last = re;
    }
    re->next = NULL;
    re->name = xstrdup (name);
    re->value = xstrdup (value);
}

void res_trav (Res r, const char *prefix, 
               void (*f)(const char *name, const char *value))
{
    struct res_entry *re;
    int l = 0;

    if (prefix)
        l = strlen(prefix);
    if (!r->init)
        reread (r);
    for (re = r->first; re; re=re->next)
        if (re->value)
            if (l==0 || !memcmp (re->name, prefix, l))
                (*f)(re->name, re->value);
}


int res_write (Res r)
{
    if (!r->init)
        reread (r);
    return 0;
}



