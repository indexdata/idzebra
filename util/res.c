/* $Id: res.c,v 1.48 2006-03-26 14:05:19 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <yaz/yaz-util.h>
#include <idzebra/res.h>

#define YLOG_RES 0

struct res_entry {
    char *name;
    char *value;
    struct res_entry *next;
};

struct res_struct {
    struct res_entry *first, *last;
    Res def_res;
    Res over_res;
};

static struct res_entry *add_entry(Res r)
{
    struct res_entry *resp;

    if (!r->first)
        resp = r->last = r->first =
	    (struct res_entry *) xmalloc(sizeof(*resp));
    else
    {
        resp = (struct res_entry *) xmalloc(sizeof(*resp));
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

ZEBRA_RES res_read_file(Res r, const char *fname)
{
    struct res_entry *resp;
    char *line;
    char *val_buf;
    int val_size, val_max = 256;
    char fr_buf[1024];
    FILE *fr;

    assert(r);

    fr = fopen(fname, "r");
    if (!fr)
    {
        yaz_log(YLOG_WARN|YLOG_ERRNO, "Cannot open `%s'", fname);
	return ZEBRA_FAIL;
    }
    val_buf = (char*) xmalloc(val_max);
    while (1)
    {
        line = fgets(fr_buf, sizeof(fr_buf)-1, fr);
        if (!line)
            break;
        if (*line != '#')
        {
            int no = 0;
            while (1)
            {
                if (fr_buf[no] == 0 || fr_buf[no] == '\n' )
                {
                    no = 0;
                    break;
                }
                if (strchr(": \t", fr_buf[no]))
                    break;
                no++;
            }
            if (!no)
                continue;
            fr_buf[no++] = '\0';
            resp = add_entry(r);
            resp->name = (char*) xmalloc(no);
            strcpy(resp->name, fr_buf);
            
            while (strchr(" \t", fr_buf[no]))
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
                    yaz_log(YLOG_DEBUG, "(name=%s,value=%s)",
                         resp->name, resp->value);
                    break;
                }
                else if (fr_buf[no] == '\\' && strchr("\n\r\f", fr_buf[no+1]))
                {
                    line = fgets(fr_buf, sizeof(fr_buf)-1, fr);
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

                        nb = (char*) xmalloc(val_max+=1024);
                        memcpy(nb, val_buf, val_size);
                        xfree(val_buf);
                        val_buf = nb;
                    }
                }
            }
        }
    }                
    xfree(val_buf);
    fclose(fr);
    return ZEBRA_OK;
}
Res res_open(Res def_res, Res over_res)
{
    Res r;
    r = (Res) xmalloc(sizeof(*r));
    r->first = r->last = NULL;
    r->def_res = def_res;
    r->over_res = over_res;
    return r;
}

void res_clear(Res r)
{
    struct res_entry *re, *re1;
    for (re = r->first; re; re=re1)
    {
	if (re->name)
	    xfree(re->name);
	if (re->value)
	    xfree(re->value);
	re1 = re->next;
	xfree(re);
    }
    r->first = r->last = NULL;
}

void res_close(Res r)
{
    if (!r)
        return;
    res_clear(r);

    xfree(r);
}

const char *res_get_prefix(Res r, const char *name, const char *prefix,
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

const char *res_get(Res r, const char *name)
{
    struct res_entry *re;
    const char *v;

    if (!r)
	return 0;
    
    v = res_get(r->over_res, name);
    if (v)
	return v;

    for (re = r->first; re; re=re->next)
        if (re->value && !yaz_matchstr(re->name, name))
            return re->value;

    return res_get(r->def_res, name);
}

const char *res_get_def(Res r, const char *name, const char *def)
{
    const char *t;

    if (!(t = res_get(r, name)))
    {
    	yaz_log(YLOG_DEBUG, "CAUTION: Using default resource %s:%s", name, def);
    	return def;
    }
    else
    	return t;
}

int res_get_match(Res r, const char *name, const char *value, const char *s)
{
    const char *cn = res_get(r, name);

    if (!cn)
	cn = s;
    if (cn && !yaz_matchstr(cn, value))
        return 1;
    return 0;
}

void res_set(Res r, const char *name, const char *value)
{
    struct res_entry *re;
    assert(r);

    for (re = r->first; re; re=re->next)
        if (re->value && !yaz_matchstr(re->name, name))
        {
            xfree(re->value);
            re->value = xstrdup_env(value);
            return;
        }
    re = add_entry(r);
    re->name = xstrdup(name);
    re->value = xstrdup_env(value);
}

int res_trav(Res r, const char *prefix, void *p,
	      void (*f)(void *p, const char *name, const char *value))
{
    struct res_entry *re;
    int l = 0;
    int no = 0;
    
    if (!r)
        return 0;
    no = res_trav(r->over_res, prefix, p, f);
    if (no)
	return no;
    if (prefix)
        l = strlen(prefix);
    for (re = r->first; re; re=re->next)
        if (re->value)
            if (l==0 || !memcmp(re->name, prefix, l))
	    {
                (*f)(p, re->name, re->value);
		no++;
	    }
    if (!no)
        return res_trav(r->def_res, prefix, p, f);
    return no;
}


ZEBRA_RES res_write_file(Res r, const char *fname)
{
    struct res_entry *re;
    FILE *fr;

    assert(r);
    fr = fopen(fname, "w");
    if (!fr)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "Cannot create `%s'", fname);
	return ZEBRA_FAIL;
    }

    for (re = r->first; re; re=re->next)
    {
        int no = 0;
        int lefts = strlen(re->name)+2;

        if (!re->value)
            fprintf(fr, "%s\n", re->name);
        else
        {
            fprintf(fr, "%s: ", re->name);
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
                    putc(re->value[i], fr);
                fprintf(fr, "\\\n");
                no=ind;
                lefts = 0;
            }
            fprintf(fr, "%s\n", re->value+no);
        }
    }
    fclose(fr);
    return ZEBRA_OK;
}

ZEBRA_RES res_get_int(Res r, const char *name, int *val)
{
    const char *cp = res_get(r, name);
    if (cp)
    {
	if (sscanf(cp, "%d", val) == 1)
	    return ZEBRA_OK;
	yaz_log(YLOG_WARN, "Expected integer for resource %s", name);
    }
    return ZEBRA_FAIL;
}

/* == pop ================================================================= */
Res res_add_over (Res p, Res t)
{
    if ((!p) || (!t))
	return (0);
    
    while (p->over_res)
	p = p->over_res;
    
    p->over_res = t;
    return (p);
}

void res_remove_over (Res r)
{
    if (!r)
        return;
    r->over_res = 0;
}

void res_close_over (Res r)
{
    if (!r)
        return;
    if (r->over_res)
	res_close(r->over_res);
    r->over_res = 0;
}

void res_add (Res r, const char *name, const char *value)
{
    struct res_entry *re;
    assert (r);
    if ((name) && (value)) 
	yaz_log (YLOG_RES, "res_add res=%p, name=%s, value=%s", r, name, value);
    
    re = add_entry (r);
    re->name = xstrdup (name);
    re->value = xstrdup_env (value);
}

char **res_2_array (Res r)
{
    struct res_entry *re;
    int i = 0;
    char **list;
    
    if (!r)
	return 0;
    
    list = xmalloc(sizeof(char *));
    
    for (re = r->first; re; re=re->next) {
	list = xrealloc(list, ((i+3) * sizeof(char *)));
	list[i++] = strdup(re->name);
	if (re->value) 
	    list[i++] = strdup(re->value);
	else
	    list[i++] = strdup("");
	yaz_log(YLOG_RES, "res2array: %s=%s",re->name, re->value);
    }
    list[i++] = 0;
    return (list);
}

char **res_get_array(Res r, const char* name)
{
    struct res_entry *re;
    int i = 0;
    char **list;
    
    if (!r)
	return 0;
    
    list = xmalloc(sizeof(char *));
    
    for (re = r->first; re; re=re->next)
	if (re->value && !yaz_matchstr (re->name, name))
	{
	    list = xrealloc(list, (i+2) * sizeof(char *));
	    list[i++] = xstrdup(re->value);
	}
    
    if (i == 0)
	return (res_get_array(r->def_res, name));
    
    list[i++] = 0;
    return (list);
}

void res_dump (Res r, int level) 
{
    struct res_entry *re;
    
    if (!r)
	return;
    
    for (re = r->first; re; re=re->next) {
	printf("%*s - %s:='%s'\n",level * 4,"",re->name,re->value);
    }
    
    if (r->def_res) {
	printf ("%*s DEF ",level * 4,"");
	res_dump (r->def_res, level + 1);
    }
    
    if (r->over_res) {
	printf ("%*s OVER ",level * 4,"");
	res_dump (r->over_res, level + 1);
    }
}
