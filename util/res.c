/* $Id: res.c,v 1.37 2004-07-26 13:59:25 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
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
#include <yaz/yaz-util.h>

struct res_entry {
    char *name;
    char *value;
    struct res_entry *next;
};

struct res_struct {
    struct res_entry *first, *last;
    char *name;
    int  init;
    Res def_res;
    Res over_res;
};

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

static char *xstrdup_env(const char *src)
{
    int i = 0;
    int j = 0;
    char *dst;
    int env_strlen = 0;

    while (src[i])
    {
	if (src[i] == '$' && src[i+1] == '{')
	{
	    char envname[128];
	    char *env_val;
	    int k = 0;
	    i = i + 2;
	    while (k < 127 && src[i] && !strchr(":}\n\r\f", src[i]))
		envname[k++] = src[i++];
	    envname[k] = '\0';

	    env_val = getenv(envname);
	    if (env_val)
		env_strlen += 1 + strlen(env_val);
	    else
		env_strlen++;
	    while (src[i] && !strchr("}\n\r\f", src[i]))
		i++;
	    if (src[i] == '}')
		i++;
	}
	else
	    i++;
    }
    dst = xmalloc(1 + env_strlen + i);
    i = 0;
    while (src[i])
    {
	if (src[i] == '$' && src[i+1] == '{')
	{
	    char envname[128];
	    char *env_val;
	    int k = 0;
	    i = i + 2;
	    while(k < 127 && src[i] && !strchr(":}\n\r\f", src[i]))
		envname[k++] = src[i++];
	    envname[k] = '\0';
	    env_val = getenv(envname);
	    if (env_val)
	    {
		strcpy(dst+j, env_val);
		j += strlen(env_val);
	    }
	    else if (src[i] == ':' && src[i+1] == '-') 
	    {
		i = i + 2;
		while (src[i] && !strchr("}\n\r\f", src[i]))
		    dst[j++] = src[i++];
	    }
	    while (src[i] && !strchr("}\n\r\f", src[i]))
		i++;
	    if (src[i] == '}')
		i++;
	}
	else
	    dst[j++] = src[i++];
    }
    dst[j] = '\0';
    return dst;
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

    if (!r->name)
	return; 

    fr = fopen (r->name, "r");
    if (!fr)
    {
        logf (LOG_WARN|LOG_ERRNO, "Cannot open `%s'", r->name);
	return ;
    }
    val_buf = (char*) xmalloc (val_max);
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
                    val_buf[val_size] = '\0';
		    resp->value = xstrdup_env(val_buf);
                    logf (LOG_DEBUG, "(name=%s,value=%s)",
                         resp->name, resp->value);
                    break;
                }
                else if (fr_buf[no] == '\\' && strchr ("\n\r\f", fr_buf[no+1]))
                {
                    line = fgets (fr_buf, sizeof(fr_buf)-1, fr);
                    if (!line)
                    {
			val_buf[val_size] = '\0';
			resp->value = xstrdup_env(val_buf);
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

Res res_open (const char *name, Res def_res, Res over_res)
{
    Res r;

    if (name)
    {
#ifdef WIN32
        if (access (name, 4))
#else
        if (access (name, R_OK))
#endif
        {
            logf (LOG_WARN|LOG_ERRNO, "Cannot open `%s'", name);
	    return 0;
        }
    }
    r = (Res) xmalloc (sizeof(*r));
    r->init = 0;
    r->first = r->last = NULL;
    if (name)
        r->name = xstrdup (name);
    else
	r->name=0;
    r->def_res = def_res;
    r->over_res = over_res;
    return r;
}

void res_close (Res r)
{
    if (!r)
        return;
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

const char *res_get_prefix (Res r, const char *name, const char *prefix,
			    const char *def)
{
    const char *v = 0;;
    if (prefix)
    {
	char rname[128];
	
	if (strlen(name) + strlen(prefix) >= (sizeof(rname)-2))
	    return 0;
	strcpy(rname, prefix);
	strcat(rname, ".");
	strcat(rname, name);
	v = res_get(r, rname);
    }
    if (!v)
	v = res_get(r, name);
    if (!v)
	v = def;
    return v;
}

const char *res_get (Res r, const char *name)
{
    struct res_entry *re;
    const char *v;

    if (!r)
	return 0;
    
    v = res_get(r->over_res, name);
    if (v)
	return v;

    if (!r->init)
        reread (r);
    for (re = r->first; re; re=re->next)
        if (re->value && !yaz_matchstr (re->name, name))
            return re->value;

    return res_get (r->def_res, name);
}

const char *res_get_def (Res r, const char *name, const char *def)
{
    const char *t;

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

    if (!cn)
	cn = s;
    if (cn && !yaz_matchstr (cn, value))
        return 1;
    return 0;
}

void res_set (Res r, const char *name, const char *value)
{
    struct res_entry *re;
    assert (r);
    if (!r->init)
        reread (r);

    for (re = r->first; re; re=re->next)
        if (re->value && !yaz_matchstr (re->name, name))
        {
            xfree (re->value);
            re->value = xstrdup_env (value);
            return;
        }
    re = add_entry (r);
    re->name = xstrdup (name);
    re->value = xstrdup_env (value);
}

int res_trav (Res r, const char *prefix, void *p,
	      void (*f)(void *p, const char *name, const char *value))
{
    struct res_entry *re;
    int l = 0;
    int no = 0;
    
    if (!r)
        return 0;
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
    if (!no)
        return res_trav (r->def_res, prefix, p, f);
    return no;
}


int res_write (Res r)
{
    struct res_entry *re;
    FILE *fr;

    assert (r);
    if (!r->init)
        reread (r);
    if (!r->name)
	return 0; /* ok, this was not from a file */
    fr = fopen (r->name, "w");
    if (!fr)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Cannot create `%s'", r->name);
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

