/* $Id: passwddb.c,v 1.11 2005-05-12 10:10:32 adam Exp $
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


#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>

#if HAVE_CRYPT_H
#include <crypt.h>
#endif

#include <yaz/log.h>
#include <yaz/xmalloc.h>

#include <passwddb.h>

struct passwd_entry {
    char *name;
    char *des;
    struct passwd_entry *next;
};

struct passwd_db {
    struct passwd_entry *entries;
};

Passwd_db passwd_db_open (void)
{
    struct passwd_db *p = (struct passwd_db *) xmalloc (sizeof(*p));
    p->entries = 0;
    return p;
}

static int get_entry (const char **p, char *dst, int max)
{	
    int i = 0;
    while ((*p)[i] != ':' && (*p)[i])
	i++;
    if (i >= max)
	i = max-1;
    if (i)
	memcpy (dst, *p, i);
    dst[i] = '\0';
    *p += i;
    if (*p)
	(*p)++;
    return i;
}

int passwd_db_file (Passwd_db db, const char *fname)
{
    FILE *f;
    char buf[1024];
    f = fopen (fname, "r");
    if (!f)
	return -1;
    while (fgets (buf, sizeof(buf)-1, f))
    {
	struct passwd_entry *pe;
	char name[128];
	char des[128];
	char *p;
	const char *cp = buf;
	if ((p = strchr (buf, '\n')))
	    *p = '\0';
	get_entry (&cp, name, 128);
	get_entry (&cp, des, 128);

	pe = (struct passwd_entry *) xmalloc (sizeof(*pe));
	pe->name = xstrdup (name);
	pe->des = xstrdup (des);
	pe->next = db->entries;
	db->entries = pe;
    }
    fclose (f);
    return 0;
}

void passwd_db_close (Passwd_db db)
{
    struct passwd_entry *pe = db->entries;
    while (pe)
    {
	struct passwd_entry *pe_next = pe->next;
	
	xfree (pe->name);
	xfree (pe->des);
	xfree (pe);
	pe = pe_next;
    }
    xfree (db);
}

void passwd_db_show (Passwd_db db)
{
    struct passwd_entry *pe;
    for (pe = db->entries; pe; pe = pe->next)
	yaz_log (YLOG_LOG,"%s:%s", pe->name, pe->des);
}

int passwd_db_auth (Passwd_db db, const char *user, const char *pass)
{
    struct passwd_entry *pe;
#if HAVE_CRYPT_H
    char salt[3];
    const char *des_try;
#endif
    for (pe = db->entries; pe; pe = pe->next)
	if (user && !strcmp (user, pe->name))
	    break;
    if (!pe)
	return -1;
#if HAVE_CRYPT_H
    if (strlen (pe->des) < 3)
	return -3;
    if (!pass)
	return -2;
    memcpy (salt, pe->des, 2);
    salt[2] = '\0';	
    des_try = crypt (pass, salt);
    if (strcmp (des_try, pe->des))
	return -2;
#else
    if (strcmp (pe->des, pass))
	return -2;
#endif
    return 0;	
}

