/* $Id: danbibr.c,v 1.5 2004-09-27 10:44:50 adam Exp $
   Copyright (C) 2004
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <yaz/log.h>

#include "grsread.h"

#include <yaz/xmalloc.h>
#include <yaz/log.h>
#include <data1.h>

#define READ_CHUNK 200

struct danbibr_info {
    WRBUF rec_buf;
    char read_buf[READ_CHUNK+1];  /* space for \0 */
};

static void *init_danbib(Res res, RecType rt)
{
    struct danbibr_info *p = (struct danbibr_info *) xmalloc (sizeof(*p));

    p->rec_buf = wrbuf_alloc();
    wrbuf_puts(p->rec_buf, "");
    return p;
}

static int read_rec(struct grs_read_info *p)
{
    struct danbibr_info *info = p->clientData;
    
    wrbuf_rewind(info->rec_buf);
    while(1)
    {
	char *cp_split = 0;
	int r = (*p->readf)(p->fh, info->read_buf, READ_CHUNK);
	if (r <= 0)
	{
	    if (wrbuf_len(info->rec_buf) > 0)
		return 1;
	    else
		return 0;
	}
	info->read_buf[r] = '\0';
	wrbuf_puts(info->rec_buf, info->read_buf);

	cp_split = strstr(wrbuf_buf(info->rec_buf), "\n$");
	if (cp_split)
	{
	    cp_split++; /* now at $ */
	    if (p->endf)
		(*p->endf)(p->fh, p->offset + 
			   (cp_split - wrbuf_buf(info->rec_buf)));
	    
	    cp_split[0] = '\0';
	    return 1;
	}
    }
}

static data1_node *mk_tree(struct grs_read_info *p, const char *rec_buf)
{
    data1_node *root = data1_mk_root(p->dh, p->mem, "danbib");
    data1_node *root_tag = data1_mk_tag(p->dh, p->mem, "danbib", 0, root);
    const char *cp = rec_buf;

    if (1)  /* <text> all </text> */
    {
	data1_node *text_node = data1_mk_tag(p->dh, p->mem, "text", 0, root_tag);
	data1_mk_text_n(p->dh, p->mem, rec_buf, strlen(rec_buf), text_node);
    }
    while (*cp)
    {
	const char *start_tag = cp;
	const char *start_text;
	if (*cp == '\n')
	{
	    cp++;
	    continue;
	}
	else if (*cp == ' ')  /* bad continuation */
	{
	    while (*cp && *cp != '\n')
		cp++;
	}
	else if (*cp == '$')  /* header */
	{
	    int no = 1;
	    cp++;
	    start_text = cp;
	    for(start_text = cp; *cp && *cp != '\n'; cp++)
		if (*cp == ':')
		{
		    if (start_text != cp)
		    {
			char elemstr[20];
			data1_node *hnode;
			sprintf(elemstr, "head%d", no);

			hnode = data1_mk_tag(p->dh, p->mem, elemstr, 0, root_tag);
			data1_mk_text_n(p->dh, p->mem, start_text,
					cp - start_text, hnode);
			start_text = cp+1;
		    }
		    no++;
		}
	}
	else /* other */
	{
	    while (*cp != ' ' && *cp && *cp != '\n')
		cp++;
	    if (*cp == ' ')
	    {
		data1_node *tag_node =
		    data1_mk_tag_n(p->dh, p->mem,
				   start_tag, cp - start_tag, 0, root_tag);
		cp++;
		start_text = cp;
		while (*cp != '\n' && *cp)
		{
		    if (*cp == '*' && cp[1]) /* subfield */
		    {
			data1_node *sub_tag_node;
			if (start_text != cp)
			    data1_mk_text_n(p->dh, p->mem, start_text,
					    cp-start_text, tag_node);
			cp++;
			sub_tag_node =
			    data1_mk_tag_n(p->dh, p->mem, cp, 1, 0, tag_node);
			cp++;
			start_text = cp;
			while (*cp)
			{
			    if (*cp == '\n' && cp[1] == ' ')
			    {
				cp++;
				if (start_text != cp)
				    data1_mk_text_n(p->dh, p->mem, start_text,
						    cp-start_text, sub_tag_node);
				while (*cp == ' ')
				    cp++;
				start_text = cp;
			    }
			    else if (*cp == '\n')
				break;
			    else if (*cp == '*')
				break;
			    else
				cp++;
			}
			if (start_text != cp)
			    data1_mk_text_n(p->dh, p->mem, start_text,
					    cp-start_text, sub_tag_node);
			start_text = cp;
		    }
		    else
			cp++;
		}
		if (start_text != cp)
		    data1_mk_text_n(p->dh, p->mem, start_text,
				    cp-start_text, tag_node);
	    }
	}
    }
    return root;
}

static data1_node *read_danbib (struct grs_read_info *p)
{
    struct danbibr_info *info = p->clientData;

    if (read_rec(p)) 
	return mk_tree(p, wrbuf_buf(info->rec_buf));
    return 0;
}

static void destroy_danbib(void *clientData)
{
    struct danbibr_info *p = (struct danbibr_info *) clientData;

    wrbuf_free(p->rec_buf, 1);
    xfree (p);
}


static int extract_danbib(void *clientData, struct recExtractCtrl *ctrl)
{
    return zebra_grs_extract(clientData, ctrl, read_danbib);
}

static int retrieve_danbib(void *clientData, struct recRetrieveCtrl *ctrl)
{
    return zebra_grs_retrieve(clientData, ctrl, read_danbib);
}

static struct recType danbib_type = {
    "grs.danbib",
    init_danbib,
    0,
    destroy_danbib,
    extract_danbib,
    retrieve_danbib,
};

RecType
#ifdef IDZEBRA_STATIC_GRS_DANBIB
idzebra_filter_grs_danbib
#else
idzebra_filter
#endif

[] = {
    &danbib_type,
    0,
};
    


