/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: res.c,v $
 * Revision 1.26  1999-10-07 09:48:36  adam
 * Allow res_get / res_get_def with NULL res.
 *
 * Revision 1.25  1999/05/26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.24  1999/02/02 14:51:42  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.23  1998/10/28 15:18:55  adam
 * Fix for DOS-formatted configuration files.
 *
 * Revision 1.22  1998/01/12 15:04:32  adam
 * Removed exit - call.
 *
 * Revision 1.21  1997/11/18 10:04:03  adam
 * Function res_trav returns number of 'hits'.
 *
 * Revision 1.20  1997/10/31 12:39:15  adam
 * Resouce name can be terminated with either white-space or colon.
 *
 * Revision 1.19  1997/10/27 14:27:59  adam
 * Fixed memory leak.
 *
 * Revision 1.18  1997/09/17 12:19:24  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.17  1997/09/09 13:38:19  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.16  1996/10/29 13:47:49  adam
 * Implemented res_get_match. Updated to use zebrautl instead of alexpath.
 *
 * Revision 1.15  1996/05/22 08:23:43  adam
 * Bug fix: trailing blanks in resource values where not removed.
 *
 * Revision 1.14  1996/04/26 11:51:20  adam
 * Resource names are matched by the yaz_matchstr routine instead of strcmp.
 *
 * Revision 1.13  1995/09/04  12:34:05  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.12  1995/01/24  16:40:32  adam
 * Bug fix.
 *
 * Revision 1.11  1994/10/05  16:54:52  adam
 * Minor changes.
 *
 * Revision 1.10  1994/10/05  10:47:31  adam
 * Small bug fix.
 *
 * Revision 1.9  1994/09/16  14:41:12  quinn
 * Added log warning to res_get_def
 *
 * Revision 1.8  1994/09/16  14:37:12  quinn
 * added res_get_def
 *
 * Revision 1.7  1994/09/06  13:01:03  quinn
 * Removed const from declaration of res_get
 *
 * Revision 1.6  1994/09/01  17:45:14  adam
 * Work on resource manager.
 *
 * Revision 1.5  1994/08/18  11:02:28  adam
 * Implementation of res_write.
 *
 * Revision 1.4  1994/08/18  10:02:01  adam
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
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <zebrautl.h>
#include <yaz-util.h>

static struct res_entry *add_entry (Res r)
{
    struct res_entry *resp;

    if (!r->first)
        resp = r->last = r->first =
	    (struct res_entry *) xmalloc (sizeof(*resp));
    else
    {
        resp = (struct res_entry *) xmalloc (sizeof(*resp));
        r->last->next = resp;
        r->last = resp;
    }
    resp->next = NULL;
    return resp;
}

static void reread (Res r)
{
    struct res_entry *resp;
    char *line;
    char *val_buf;
    int val_size, val_max = 256;
    char fr_buf[1024];
    FILE *fr;

    assert (r);
    r->init = 1;

    val_buf = (char*) xmalloc (val_max);

    fr = fopen (r->name, "r");
    if (!fr)
    {
        logf (LOG_WARN|LOG_ERRNO, "Cannot open %s", r->name);
	return ;
    }
    while (1)
    {
        line = fgets (fr_buf, sizeof(fr_buf)-1, fr);
        if (!line)
            break;
        if (*line == '#')
        {
            int no = 0;

            while (fr_buf[no] && fr_buf[no] != '\n')
                no++;
            fr_buf[no] = '\0';

            resp = add_entry (r);
            resp->name = (char*) xmalloc (no+1);
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
                    no = 0;
                    break;
                }
                if (strchr (": \t", fr_buf[no]))
                    break;
                no++;
            }
            if (!no)
                continue;
            fr_buf[no++] = '\0';
            resp = add_entry (r);
            resp->name = (char*) xmalloc (no);
            strcpy (resp->name, fr_buf);
            
            while (strchr (" \t", fr_buf[no]))
                no++;
            val_size = 0;
            while (1)
            {
                if (fr_buf[no] == '\0' || strchr("\n\r\f", fr_buf[no]))
                {
                    while (val_size > 0 &&
                              (val_buf[val_size-1] == ' ' ||
                               val_buf[val_size-1] == '\t'))
                        val_size--;
                    val_buf[val_size++] = '\0';
                    resp->value = (char*) xmalloc (val_size);
                    strcpy (resp->value, val_buf);
                    logf (LOG_DEBUG, "(name=%s,value=%s)",
                         resp->name, resp->value);
                    break;
                }
                else if (fr_buf[no] == '\\' && strchr ("\n\r\f", fr_buf[no+1]))
                {
                    line = fgets (fr_buf, sizeof(fr_buf)-1, fr);
                    if (!line)
                    {
                        resp->value = (char*) xmalloc (val_size);
                        strcpy (resp->value, val_buf);
                        break;
                    }
                    no = 0;
                }
                else
                {
                    val_buf[val_size++] = fr_buf[no++];
                    if (val_size+1 >= val_max)
                    {
                        char *nb;

                        nb = (char*) xmalloc (val_max+=1024);
                        memcpy (nb, val_buf, val_size);
                        xfree (val_buf);
                        val_buf = nb;
                    }
                }
            }
        }
    }                
    xfree (val_buf);
    fclose (fr);
}

Res res_open (const char *name)
{
    Res r;
#ifdef WIN32
    if (access (name, 4))
#else
    if (access (name, R_OK))
#endif
    {
        logf (LOG_LOG|LOG_ERRNO, "Cannot access resource file `%s'", name);
	return NULL;
    }
    r = (Res) xmalloc (sizeof(*r));
    r->init = 0;
    r->first = r->last = NULL;
    r->name = xstrdup (name);
    return r;
}

void res_close (Res r)
{
    assert (r);
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
    xfree (r->name);
    xfree (r);
}

char *res_get (Res r, const char *name)
{
    struct res_entry *re;

    if (!r)
	return NULL;
    if (!r->init)
        reread (r);
    for (re = r->first; re; re=re->next)
        if (re->value && !yaz_matchstr (re->name, name))
            return re->value;
    return NULL;
}

char *res_get_def (Res r, const char *name, char *def)
{
    char *t;

    if (!(t = res_get (r, name)))
    {
    	logf (LOG_DEBUG, "CAUTION: Using default resource %s:%s", name, def);
    	return def;
    }
    else
    	return t;
}

int res_get_match (Res r, const char *name, const char *value, const char *s)
{
    const char *cn = res_get (r, name);

    if (cn && !yaz_matchstr (cn, value))
        return 1;
    return 0;
}

void res_put (Res r, const char *name, const char *value)
{
    struct res_entry *re;
    assert (r);
    if (!r->init)
        reread (r);

    for (re = r->first; re; re=re->next)
        if (re->value && !yaz_matchstr (re->name, name))
        {
            xfree (re->value);
            re->value = xstrdup (value);
            return;
        }
    re = add_entry (r);
    re->name = xstrdup (name);
    re->value = xstrdup (value);
}

int res_trav (Res r, const char *prefix, void *p,
	      void (*f)(void *p, const char *name, const char *value))
{
    struct res_entry *re;
    int l = 0;
    int no = 0;
    
    assert (r);
    if (prefix)
        l = strlen(prefix);
    if (!r->init)
        reread (r);
    for (re = r->first; re; re=re->next)
        if (re->value)
            if (l==0 || !memcmp (re->name, prefix, l))
	    {
                (*f)(p, re->name, re->value);
		no++;
	    }
    return no;
}


int res_write (Res r)
{
    struct res_entry *re;
    FILE *fr;

    assert (r);
    if (!r->init)
        reread (r);
    fr = fopen (r->name, "w");
    if (!fr)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Cannot create %s", r->name);
        exit (1);
    }

    for (re = r->first; re; re=re->next)
    {
        int no = 0;
        int lefts = strlen(re->name)+2;

        if (!re->value)
            fprintf (fr, "%s\n", re->name);
        else
        {
            fprintf (fr, "%s: ", re->name);
            while (lefts + strlen(re->value+no) > 78)
            {
                int i = 20;
                int ind = no+ 78-lefts;
                while (--i >= 0)
                {
                    if (re->value[ind] == ' ')
                        break;
                    --ind;
                }
                if (i<0)
                    ind = no + 78 - lefts;
                for (i = no; i != ind; i++)
                    putc (re->value[i], fr);
                fprintf (fr, "\\\n");
                no=ind;
                lefts = 0;
            }
            fprintf (fr, "%s\n", re->value+no);
        }
    }
    fclose (fr);
    return 0;
}



