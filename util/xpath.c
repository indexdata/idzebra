/* $Id: xpath.c,v 1.1 2003-02-04 12:06:48 pop Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <yaz/nmem.h>
#include <zebra_xpath.h>

char *get_xp_part (char **strs, NMEM mem) {
  char *str = *strs;
  char *res = '\0';
  char *cp = str;
  char *co;
  int quoted = 0;

  /* ugly */
  char *sep = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\" ";

  while (*cp == ' ') {cp++; str++;}
  if (!strchr("><=] ", *cp)) sep = "><=] ";

  while (*cp && !(strchr(sep,*cp) && !quoted) && (*cp != ']')) {
    if (*cp =='"') quoted = 1 - quoted;
    cp++;
  }  
  /* removing leading and trailing " */
  co = cp;
  if (*str == '"') str++;
  if (*(cp-1) == '"') cp--;
  if (str < co) {
    res = nmem_malloc(mem, cp - str + 1);
    memcpy (res, str, (cp-str));
    *(res + (cp-str)) = '\0';
    *strs = co;
  }

  return (res);
}


struct xpath_predicate *get_xpath_predicate(char *predicates, NMEM mem) {
  char *p1;
  char *p2;
  char *p3;
  char *p4;

  struct xpath_predicate *r1;
  struct xpath_predicate *r2;
  struct xpath_predicate *res = 0;

  char *pr = predicates;

  if ((p1 = get_xp_part(&pr, mem))) {
    if ((p2 = get_xp_part(&pr, mem))) {
      if (!strcmp (p2, "and") || !strcmp (p2, "or") || !strcmp (p2, "not")) {
	r1=nmem_malloc(mem, sizeof(struct xpath_predicate));
	r1->which = XPATH_PREDICATE_RELATION;
	r1->u.relation.name = p1;
	r1->u.relation.op = "";
	r1->u.relation.value = "";
	
	r2 = get_xpath_predicate (pr, mem);
      
	res = nmem_malloc(mem, sizeof(struct xpath_predicate));
	res->which = XPATH_PREDICATE_BOOLEAN;
	res->u.boolean.op = p2;
	res->u.boolean.left = r1;
	res->u.boolean.right = r2;

	return (res);
      }

      if (strchr("><=] ", *p2)) {
	r1 = nmem_malloc(mem, sizeof(struct xpath_predicate));
	
	r1->which = XPATH_PREDICATE_RELATION;
	r1->u.relation.name = p1;
	r1->u.relation.op = p2;

	if ((p3 = get_xp_part(&pr, mem))) {
	  r1->u.relation.value = p3;
	} else {
	  /* error */
	}
      }
      
      if ((p4 = get_xp_part(&pr, mem))) {
	if (!strcmp (p4, "and") || !strcmp (p4, "or") || !strcmp (p4, "not")) {

	  r2 = get_xpath_predicate (pr, mem);

	  res = nmem_malloc(mem, sizeof(struct xpath_predicate));
	  res->which = XPATH_PREDICATE_BOOLEAN;
	  res->u.boolean.op = p4;
	  res->u.boolean.left = r1;
	  res->u.boolean.right = r2;
	  return (res);
	} else {
	  /* error */
	}
      } else {
	return (r1);
      }
	  
    } else {
	r1 = nmem_malloc(mem, sizeof(struct xpath_predicate));
	
	r1->which = XPATH_PREDICATE_RELATION;
	r1->u.relation.name = p1;
	r1->u.relation.op = "";
 	r1->u.relation.value = "";

	return (r1);
    }
  }
  return 0;
}

int parse_xpath_str(const char *xpath_string,
		    struct xpath_location_step *xpath, NMEM mem)
{
    const char *cp;
    char *a;

    int no = 0;
    
    if (!xpath_string || *xpath_string != '/')
        return -1;
    cp = xpath_string;

    while (*cp)
    {
        int i = 0;
        while (*cp && !strchr("/[",*cp))
        {
            i++;
            cp++;
        }
        xpath[no].predicate = 0;
        xpath[no].part = nmem_malloc (mem, i+1);
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
    return no;
}

void dump_xp_predicate (struct xpath_predicate *p) {
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

void dump_xp_steps (struct xpath_location_step *xpath, int no) {
  int i;
  for (i=0; i<no; i++) {
    fprintf (stderr, "Step %d: %s   ",i,xpath[i].part);
    dump_xp_predicate(xpath[i].predicate);
    fprintf (stderr, "\n");
  }
}

