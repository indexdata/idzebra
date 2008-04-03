/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <yaz/tokenizer.h>
#include <yaz/yaz-util.h>
#include <idzebra/res.h>

#define YLOG_RES 0

struct res_entry {
    char *name;
    char *value;
    struct res_entry *next;
};

struct res_struct {
    int ref_count;
    struct res_entry *first, *last;
    Res def_res;
    Res over_res;
};

static Res res_incref(Res r)
{
    if (r)
        r->ref_count++;
    return r;
}

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
    FILE *fr;
    int errors = 0;

    assert(r);

    fr = fopen(fname, "r");
    if (!fr)
    {
        yaz_log(YLOG_WARN|YLOG_ERRNO, "Cannot open `%s'", fname);
        errors++;
    }
    else
    {
        char fr_buf[1024];
        char *line;
        int lineno = 1;
        WRBUF wrbuf_val = wrbuf_alloc();
        yaz_tok_cfg_t yt = yaz_tok_cfg_create();
        
        while ((line = fgets(fr_buf, sizeof(fr_buf)-1, fr)))
        {
            yaz_tok_parse_t tp = yaz_tok_parse_buf(yt, line);
            int t = yaz_tok_move(tp);
            
            if (t == YAZ_TOK_STRING)
            {
                size_t sz;
                struct res_entry *resp;
                const char *cp = yaz_tok_parse_string(tp);
                const char *cp1 = strchr(cp, ':');

                if (!cp1)
                {
                    yaz_log(YLOG_FATAL, "%s:%d missing colon after '%s'",
                            fname, lineno, cp);
                    errors++;
                    break;
                }
                resp = add_entry(r);
                sz = cp1 - cp;
                resp->name = xmalloc(sz + 1);
                memcpy(resp->name, cp, sz);
                resp->name[sz] = '\0';

                wrbuf_rewind(wrbuf_val);

                if (cp1[1])
                {
                    /* name:value  */
                    wrbuf_puts(wrbuf_val, cp1+1);
                }
                else
                {
                    /* name:   value */
                    t = yaz_tok_move(tp);
                    
                    if (t != YAZ_TOK_STRING)
                    {
                        resp->value = xstrdup("");
                        yaz_log(YLOG_FATAL, "%s:%d missing value after '%s'",
                                fname, lineno, resp->name);
                        errors++;
                        break;
                    }
                    wrbuf_puts(wrbuf_val, yaz_tok_parse_string(tp));
                }
                while ((t=yaz_tok_move(tp)) == YAZ_TOK_STRING)
                {
                    wrbuf_putc(wrbuf_val, ' ');
                    wrbuf_puts(wrbuf_val, yaz_tok_parse_string(tp));
                }
                resp->value = xstrdup_env(wrbuf_cstr(wrbuf_val));
                /* printf("name=%s value=%s\n", resp->name, resp->value); */
            }
            lineno++;
            yaz_tok_parse_destroy(tp);
        }         
        fclose(fr);
        yaz_tok_cfg_destroy(yt);
        wrbuf_destroy(wrbuf_val);
    }
    if (errors)
        return ZEBRA_FAIL;
    return ZEBRA_OK;
}

Res res_open(Res def_res, Res over_res)
{
    Res r;
    r = (Res) xmalloc(sizeof(*r));

    r->ref_count = 1;
    r->first = r->last = NULL;
    r->def_res = res_incref(def_res);
    r->over_res = res_incref(over_res);
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
    if (r && --(r->ref_count) == 0)
    {
        res_clear(r);
        res_close(r->def_res);
        res_close(r->over_res);
        xfree(r);
    }
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
	if (def)
    	    yaz_log(YLOG_DEBUG, "Using default resource %s:%s", name, def);
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

    if (!value)
        return;
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


void res_add(Res r, const char *name, const char *value)
{
    struct res_entry *re;
    assert(r);
    assert(name);
    assert(value);
    yaz_log(YLOG_RES, "res_add res=%p, name=%s, value=%s", r, name, value);
    re = add_entry(r);
    re->name = xstrdup(name);
    re->value = xstrdup_env(value);
}

void res_dump(Res r, int level) 
{
    struct res_entry *re;
    
    if (!r)
	return;
    
    for (re = r->first; re; re=re->next) {
	printf("%*s - %s:='%s'\n",level * 4,"",re->name,re->value);
    }
    
    if (r->def_res) {
	printf("%*s DEF ",level * 4,"");
	res_dump(r->def_res, level + 1);
    }
    
    if (r->over_res) {
	printf("%*s OVER ",level * 4,"");
	res_dump(r->over_res, level + 1);
    }
}

int res_check(Res r_i, Res r_v)
{
    struct res_entry *e_i;
    int errors = 0;
    
    for (e_i = r_i->first; e_i; e_i = e_i->next)
    {
        struct res_entry *e_v;
        for (e_v = r_v->first; e_v; e_v = e_v->next)
        {
            int prefix_allowed = 0;
            int suffix_allowed = 0;
            const char *name = e_i->name;
            size_t name_len = strlen(e_i->name);
            char namez[32];
            const char *first_dot = 0;
            const char *second_dot = 0;

            if (strchr(e_v->value, 'p'))
                prefix_allowed = 1;
            if (strchr(e_v->value, 's'))
                suffix_allowed = 1;
            
            first_dot = strchr(name, '.');
            if (prefix_allowed && first_dot)
            {
                name = first_dot+1;
                name_len = strlen(name);
            }
            second_dot = strchr(name, '.');
            if (suffix_allowed && second_dot)
            {
                name_len = second_dot - name;
            }
            if (name_len < sizeof(namez)-1)
            {
                memcpy(namez, name, name_len);
                namez[name_len] = '\0';
                if (!yaz_matchstr(namez, e_v->name))
                    break;
            }
            /* for case 'a.b' we have to check 'a' as well */
            if (prefix_allowed && suffix_allowed && first_dot && !second_dot)
            {
                name = e_i->name;
                name_len = first_dot - name;
                if (name_len < sizeof(namez)-1)
                {
                    memcpy(namez, name, name_len);
                    namez[name_len] = '\0';
                    if (!yaz_matchstr(namez, e_v->name))
                        break;
                }
            }
        }
        if (!e_v)
        {
            yaz_log(YLOG_WARN, "The following setting is unrecognized: %s",
                    e_i->name);
            errors++;
        }
    }
    return errors;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

