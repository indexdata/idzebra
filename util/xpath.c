/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <yaz/nmem.h>
#include <zebra_xpath.h>

static char *get_xp_part (char **strs, NMEM mem, int *literal)
{
    char *cp = *strs;
    char *str = 0;
    char *res = 0;

    *literal = 0;
    while (*cp == ' ')
        cp++;
    str = cp;
    if (strchr("()", *cp))
        cp++;
    else if (strchr("><=", *cp))
    {
        while (strchr("><=", *cp))
            cp++;
    }
    else if (*cp == '"' || *cp == '\'')
    {
        int sep = *cp;
        str++;
        cp++;
        while (*cp && *cp != sep)
            cp++;
        res = nmem_malloc(mem, cp - str + 1);
        if ((cp - str))
            memcpy (res, str, (cp-str));
        res[cp-str] = '\0';
        if (*cp)
            cp++;
        *literal = 1;
    }
    else
    {
        while (*cp && !strchr("><=()]\" ", *cp))
            cp++;
    }
    if (!res)
    {
        res = nmem_malloc(mem, cp - str + 1);
        if ((cp - str))
            memcpy (res, str, (cp-str));
        res[cp-str] = '\0';
    }
    *strs = cp;
    return res;
}

static struct xpath_predicate *get_xpath_boolean(char **pr, NMEM mem,
                                                 char **look, int *literal);

static struct xpath_predicate *get_xpath_relation(char **pr, NMEM mem,
                                                  char **look, int *literal)
{
    struct xpath_predicate *res = 0;
    if (!*literal && !strcmp(*look, "("))
    {
        *look = get_xp_part(pr, mem, literal);
        res = get_xpath_boolean(pr, mem, look, literal);
        if (!strcmp(*look, ")"))
            *look = get_xp_part(pr, mem, literal);
        else
            res = 0; /* error */
    }
    else
    {
        res=nmem_malloc(mem, sizeof(struct xpath_predicate));
        res->which = XPATH_PREDICATE_RELATION;
        res->u.relation.name = *look;

        *look = get_xp_part(pr, mem, literal);
        if (*look && !*literal && strchr("><=", **look))
        {
            res->u.relation.op = *look;

            *look = get_xp_part(pr, mem, literal);
            if (!*look)
                return 0;  /* error */
            res->u.relation.value = *look;
            *look = get_xp_part(pr, mem, literal);
        }
        else
        {
            res->u.relation.op = "";
            res->u.relation.value = "";
        }
    }
    return res;
}

static struct xpath_predicate *get_xpath_boolean(char **pr, NMEM mem,
                                                 char **look, int *literal)
{
    struct xpath_predicate *left = 0;
    
    left = get_xpath_relation(pr, mem, look, literal);
    if (!left)
        return 0;
    
    while (*look && !*literal &&
           (!strcmp(*look, "and") || !strcmp(*look, "or") || 
            !strcmp(*look, "not")))
    {
        struct xpath_predicate *res, *right;

        res = nmem_malloc(mem, sizeof(struct xpath_predicate));
        res->which = XPATH_PREDICATE_BOOLEAN;
        res->u.boolean.op = *look;
        res->u.boolean.left = left;

        *look = get_xp_part(pr, mem, literal); /* skip the boolean name */
        right = get_xpath_relation(pr, mem, look, literal);

        res->u.boolean.right = right;

        left = res;
    }
    return left;
}

static struct xpath_predicate *get_xpath_predicate(char *predicate, NMEM mem)
{
    int literal;
    char **pr = &predicate;
    char *look = get_xp_part(pr, mem, &literal);

    if (!look)
        return 0;
    return get_xpath_boolean(pr, mem, &look, &literal);
}

int zebra_parse_xpath_str(const char *xpath_string,
                          struct xpath_location_step *xpath, int max, NMEM mem)
{
    const char *cp;
    char *a;
    
    int no = 0;
    
    if (!xpath_string || *xpath_string != '/')
        return -1;
    cp = xpath_string;
    
    while (*cp && no < max)
    {
        int i = 0;
        while (*cp && !strchr("/[",*cp))
        {
            i++;
            cp++;
        }
        xpath[no].predicate = 0;
        xpath[no].part = nmem_malloc (mem, i+1);
        if (i)
            memcpy (xpath[no].part,  cp - i, i);
        xpath[no].part[i] = 0;

        if (*cp == '[')
        {
            cp++;
            while (*cp == ' ')
                cp++;

	    a = (char *)cp;
	    xpath[no].predicate = get_xpath_predicate(a, mem);
	    while(*cp && *cp != ']') {
	      cp++;
	    }
	    if (*cp == ']')
                cp++;
        } /* end of ] predicate */
        no++;
        if (*cp != '/')
            break;
        cp++;
    }

/* for debugging .. */
#if 0
    dump_xp_steps(xpath, no);
#endif

    return no;
}

void dump_xp_predicate (struct xpath_predicate *p)
{
    if (p) {
        if (p->which == XPATH_PREDICATE_RELATION &&
            p->u.relation.name[0]) {
            fprintf (stderr, "%s,%s,%s", 
                     p->u.relation.name,
                     p->u.relation.op,
                     p->u.relation.value);
        } else {
            fprintf (stderr, "(");
            dump_xp_predicate(p->u.boolean.left);
            fprintf (stderr, ") %s (", p->u.boolean.op);
            dump_xp_predicate(p->u.boolean.right);
            fprintf (stderr, ")");
        }
    }
}

void dump_xp_steps (struct xpath_location_step *xpath, int no)
{
    int i;
    for (i=0; i<no; i++) {
        fprintf (stderr, "Step %d: %s   ",i,xpath[i].part);
        dump_xp_predicate(xpath[i].predicate);
        fprintf (stderr, "\n");
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

