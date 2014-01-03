/* This file is part of the Zebra server.
   Copyright (C) Index Data

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


#if HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>

#if HAVE_CRYPT_H
#include <crypt.h>
#endif

#include <assert.h>
#include <yaz/log.h>
#include <yaz/xmalloc.h>

#include <passwddb.h>

struct passwd_entry {
    int encrypt_flag;
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

static int passwd_db_file_int(Passwd_db db, const char *fname,
			      int encrypt_flag)
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
	pe->encrypt_flag = encrypt_flag;
	pe->next = db->entries;
	db->entries = pe;
    }
    fclose (f);
    return 0;
}

void passwd_db_close(Passwd_db db)
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

void passwd_db_show(Passwd_db db)
{
    struct passwd_entry *pe;
    for (pe = db->entries; pe; pe = pe->next)
	yaz_log (YLOG_LOG,"%s:%s", pe->name, pe->des);
}

int passwd_db_auth(Passwd_db db, const char *user, const char *pass)
{
    struct passwd_entry *pe;

    assert(db);
    for (pe = db->entries; pe; pe = pe->next)
	if (user && !strcmp (user, pe->name))
	    break;
    if (!pe)
	return -1;
    if (!pass)
	return -2;
    if (pe->encrypt_flag)
    {
#if HAVE_CRYPT_H
	const char *des_try;
        assert(pe->des);
	if (strlen (pe->des) < 3)
	    return -3;

        if (pe->des[0] != '$') /* Not MD5? (assume DES) */
        {
            if (strlen(pass) > 8) /* maximum key length is 8 */
                return -2;
        }
	des_try = crypt (pass, pe->des);

        assert(des_try);
	if (strcmp (des_try, pe->des))
	    return -2;
#else
	return -2;
#endif
    }
    else
    {
        assert(pass);
        assert(pe->des);
	if (strcmp (pe->des, pass))
	    return -2;
    }
    return 0;
}

int passwd_db_file_crypt(Passwd_db db, const char *fname)
{
#if HAVE_CRYPT_H
    return passwd_db_file_int(db, fname, 1);
#else
    return -1;
#endif
}

int passwd_db_file_plain(Passwd_db db, const char *fname)
{
    return passwd_db_file_int(db, fname, 0);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

