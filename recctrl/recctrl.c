/* $Id: recctrl.c,v 1.24 2006-05-05 07:34:26 adam Exp $
   Copyright (C) 1995-2006
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
#include <assert.h>
#include <string.h>
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <direntz.h>
#include <idzebra/util.h>
#include <idzebra/recctrl.h>

struct recTypeClass {
    RecType recType;
    struct recTypeClass *next;
    void *module_handle;
};

struct recTypeInstance {
    RecType recType;
    struct recTypeInstance *next;
    int init_flag;
    void *clientData;
};

struct recTypes {
    data1_handle dh;
    struct recTypeInstance *entries;
};

static void recTypeClass_add (struct recTypeClass **rts, RecType *rt,
			      NMEM nmem, void *module_handle);


RecTypeClass recTypeClass_create (Res res, NMEM nmem)
{
    struct recTypeClass *rts = 0;

#ifdef IDZEBRA_STATIC_GRS_SGML
    if (1)
    {
	extern RecType idzebra_filter_grs_sgml[];
	recTypeClass_add (&rts, idzebra_filter_grs_sgml, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_TEXT
    if (1)
    {
	extern RecType idzebra_filter_text[];
	recTypeClass_add (&rts, idzebra_filter_text, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_GRS_XML
#if HAVE_EXPAT_H
    if (1)
    {
	extern RecType idzebra_filter_grs_xml[];
	recTypeClass_add (&rts, idzebra_filter_grs_xml, nmem, 0);
    }
#endif
#endif

#ifdef IDZEBRA_STATIC_GRS_REGX
    if (1)
    {
	extern RecType idzebra_filter_grs_regx[];
	recTypeClass_add (&rts, idzebra_filter_grs_regx, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_GRS_MARC
    if (1)
    {
	extern RecType idzebra_filter_grs_marc[];
	recTypeClass_add (&rts, idzebra_filter_grs_marc, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_GRS_CSV
    if (1)
    {
	extern RecType idzebra_filter_grs_csv[];
	recTypeClass_add (&rts, idzebra_filter_grs_csv, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_GRS_DANBIB
    if (1)
    {
	extern RecType idzebra_filter_grs_danbib[];
	recTypeClass_add (&rts, idzebra_filter_grs_danbib, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_SAFARI
    if (1)
    {
	extern RecType idzebra_filter_safari[];
	recTypeClass_add (&rts, idzebra_filter_safari, nmem, 0);
    }
#endif

#ifdef IDZEBRA_STATIC_ALVIS
#if HAVE_XSLT
    if (1)
    {
	extern RecType idzebra_filter_alvis[];
	recTypeClass_add (&rts, idzebra_filter_alvis, nmem, 0);
    }
#endif
#endif

#ifdef IDZEBRA_STATIC_XSLT
#if HAVE_XSLT
    if (1)
    {
	extern RecType idzebra_filter_xslt[];
	recTypeClass_add (&rts, idzebra_filter_xslt, nmem, 0);
    }
#endif
#endif
    return rts;
}

void recTypeClass_load_modules(RecTypeClass *rts, NMEM nmem,
			       const char *module_path)
{
#if HAVE_DLFCN_H
    if (module_path)
    {
	DIR *dir = opendir(module_path);
	yaz_log(YLOG_LOG, "searching filters in %s", module_path);
	if (dir)
	{
	    struct dirent *de;

	    while ((de = readdir(dir)))
	    {
		size_t dlen = strlen(de->d_name);
		if (dlen >= 5 &&
		    !memcmp(de->d_name, "mod-", 4) &&
		    !strcmp(de->d_name + dlen - 3, ".so"))
		{
		    void *mod_p, *fl;
		    char fname[FILENAME_MAX*2+1];
		    sprintf(fname, "%.*s/%.*s",
			    FILENAME_MAX, module_path,
			    FILENAME_MAX, de->d_name);
		    mod_p = dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
		    if (mod_p && (fl = dlsym(mod_p, "idzebra_filter")))
		    {
			yaz_log(YLOG_LOG, "Loaded filter module %s", fname);
			recTypeClass_add(rts, fl, nmem, mod_p);
		    }
		    else if (mod_p)
		    {
			const char *err = dlerror();
			yaz_log(YLOG_WARN, "dlsym failed %s %s",
				fname, err ? err : "none");
			dlclose(mod_p);
		    }
		    else
		    {
			const char *err = dlerror();
			yaz_log(YLOG_WARN, "dlopen failed %s %s",
				fname, err ? err : "none");
			
		    }
		}
	    }
	    closedir(dir);
	}
    }
#endif
}

static void recTypeClass_add(struct recTypeClass **rts, RecType *rt,
			     NMEM nmem, void *module_handle)
{
    while (*rt)
    {
	struct recTypeClass *r = (struct recTypeClass *)
	    nmem_malloc (nmem, sizeof(*r));
	
	r->next = *rts;
	*rts = r;

	yaz_log(YLOG_LOG, "Adding filter %s", (*rt)->name);
	r->module_handle = module_handle;
	module_handle = 0; /* so that we only store module_handle once */
	r->recType = *rt;

	rt++;
    }
}

void recTypeClass_info(RecTypeClass rtc, void *cd,
		       void (*cb)(void *cd, const char *s))
{
    for (; rtc; rtc = rtc->next)
	(*cb)(cd, rtc->recType->name);
}

void recTypeClass_destroy(RecTypeClass rtc)
{
    for (; rtc; rtc = rtc->next)
    {
#if HAVE_DLFCN_H
	if (rtc->module_handle)
	    dlclose(rtc->module_handle);
#endif
    }
}

RecTypes recTypes_init(RecTypeClass rtc, data1_handle dh)
{
    RecTypes rts = (RecTypes) nmem_malloc(data1_nmem_get(dh), sizeof(*rts));

    struct recTypeInstance **rti = &rts->entries;
    
    rts->dh = dh;

    for (; rtc; rtc = rtc->next)
    {
	*rti = nmem_malloc(data1_nmem_get(dh), sizeof(**rti));
	(*rti)->recType = rtc->recType;
	(*rti)->init_flag = 0;
	rti = &(*rti)->next;
    }
    *rti = 0;
    return rts;
}

void recTypes_destroy (RecTypes rts)
{
    struct recTypeInstance *rti;

    for (rti = rts->entries; rti; rti = rti->next)
    {
	if (rti->init_flag)
	    (*(rti->recType)->destroy)(rti->clientData);
    }
}

RecType recType_byName (RecTypes rts, Res res, const char *name,
			void **clientDataP)
{
    struct recTypeInstance *rti;

    for (rti = rts->entries; rti; rti = rti->next)
    {
	size_t slen = strlen(rti->recType->name);
	if (!strncmp (rti->recType->name, name, slen)
	    && (name[slen] == '\0' || name[slen] == '.'))
	{
	    if (!rti->init_flag)
	    {
		rti->init_flag = 1;
		rti->clientData =
		    (*(rti->recType)->init)(res, rti->recType);
	    }
	    *clientDataP = rti->clientData;
	    if (name[slen])
		slen++;  /* skip . */

	    if (rti->recType->config)
	    {
		if ((*(rti->recType)->config)
		    (rti->clientData, res, name+slen) != ZEBRA_OK)
		    return 0;
	    }
	    return rti->recType;
	}
    }
    return 0;
}

