/* $Id: marcread.c,v 1.22 2003-11-09 11:49:49 oleg Exp $
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
#include <ctype.h>
#include <assert.h>

#include <yaz/log.h>
#include <yaz/yaz-util.h>
#include <yaz/marcdisp.h>
#include "grsread.h"
#include "marcomp.h"
#include "inline.h"

#define MARC_DEBUG 0
#define MARCOMP_DEBUG 0

static data1_node *grs_read_iso2709 (struct grs_read_info *p, int marc_xml)
{
    char buf[100000];
    int entry_p;
    int record_length;
    int indicator_length;
    int identifier_length;
    int base_address;
    int length_data_entry;
    int length_starting;
    int length_implementation;
    int read_bytes;
#if MARC_DEBUG
    FILE *outf = stdout;
#endif
    data1_node *res_root, *res_top;
    char *absynName;
    data1_marctab *marctab;

    if ((*p->readf)(p->fh, buf, 5) != 5)
        return NULL;
    record_length = atoi_n (buf, 5);
    if (record_length < 25)
    {
        logf (LOG_WARN, "MARC record length < 25, is %d", record_length);
        return NULL;
    }
    /* read remaining part - attempt to read one byte furhter... */
    read_bytes = (*p->readf)(p->fh, buf+5, record_length-4);
    if (read_bytes < record_length-5)
    {
        logf (LOG_WARN, "Couldn't read whole MARC record");
        return NULL;
    }
    if (read_bytes == record_length - 4)
    {
        off_t cur_offset = (*p->tellf)(p->fh);
	if (cur_offset <= 27)
	    return NULL;
	if (p->endf)
	    (*p->endf)(p->fh, cur_offset - 1);
    }
    absynName = p->type;
    res_root = data1_mk_root (p->dh, p->mem, absynName);
    if (!res_root)
    {
        yaz_log (LOG_WARN, "cannot read MARC without an abstract syntax");
        return 0;
    }
    if (marc_xml)
    {
	data1_node *lead;
	const char *attr[] = { "xmlns", "http://www.loc.gov/MARC21/slim", 0};
			 
	res_top = data1_mk_tag (p->dh, p->mem, "record", attr, res_root);

	lead = data1_mk_tag(p->dh, p->mem, "leader", 0, res_top);
	data1_mk_text_n(p->dh, p->mem, buf, 24, lead);
    }
    else
	res_top = data1_mk_tag (p->dh, p->mem, absynName, 0, res_root);

    if ((marctab = res_root->u.root.absyn->marc))
    {
	memcpy(marctab->leader, buf, 24);
        memcpy(marctab->implementation_codes, buf+6, 4);
        marctab->implementation_codes[4] = '\0';
        memcpy(marctab->user_systems, buf+17, 3);
        marctab->user_systems[3] = '\0';
    }

    if (marctab && marctab->force_indicator_length >= 0)
	indicator_length = marctab->force_indicator_length;
    else
	indicator_length = atoi_n (buf+10, 1);
    if (marctab && marctab->force_identifier_length >= 0)
	identifier_length = marctab->force_identifier_length;
    else
	identifier_length = atoi_n (buf+11, 1);
    base_address = atoi_n (buf+12, 4);

    length_data_entry = atoi_n (buf+20, 1);
    length_starting = atoi_n (buf+21, 1);
    length_implementation = atoi_n (buf+22, 1);

    for (entry_p = 24; buf[entry_p] != ISO2709_FS; )
        entry_p += 3+length_data_entry+length_starting;
    base_address = entry_p+1;
    for (entry_p = 24; buf[entry_p] != ISO2709_FS; )
    {
        int data_length;
        int data_offset;
        int end_offset;
        int i, i0;
        char tag[4];
        data1_node *res;
        data1_node *parent = res_top;

        memcpy (tag, buf+entry_p, 3);
        entry_p += 3;
        tag[3] = '\0';

	if (marc_xml)
	    res = parent;
	else
	    res = data1_mk_tag_n (p->dh, p->mem, tag, 3, 0 /* attr */, parent);
	
#if MARC_DEBUG
        fprintf (outf, "%s ", tag);
#endif
        data_length = atoi_n (buf+entry_p, length_data_entry);
        entry_p += length_data_entry;
        data_offset = atoi_n (buf+entry_p, length_starting);
        entry_p += length_starting;
        i = data_offset + base_address;
        end_offset = i+data_length-1;

        if (memcmp (tag, "00", 2) && indicator_length)
        {
            /* generate indicator node */
	    if (marc_xml)
	    {
		const char *attr[10];
		int j;

		attr[0] = "tag";
		attr[1] = tag;
		attr[2] = 0;

		res = data1_mk_tag(p->dh, p->mem, "datafield", attr, res);

		for (j = 0; j<indicator_length; j++)
		{
		    char str1[18], str2[2];
		    sprintf (str1, "ind%d", j+1);
		    str2[0] = buf[i+j];
		    str2[1] = '\0';

		    attr[0] = str1;
		    attr[1] = str2;
		    
		    data1_tag_add_attr (p->dh, p->mem, res, attr);
		}
	    }
	    else
	    {
#if MARC_DEBUG
		int j;
#endif
		res = data1_mk_tag_n (p->dh, p->mem, 
				      buf+i, indicator_length, 0 /* attr */, res);
#if MARC_DEBUG
		for (j = 0; j<indicator_length; j++)
		    fprintf (outf, "%c", buf[j+i]);
#endif
	    }
	    i += indicator_length;
        } 
	else
	{
	    if (marc_xml)
	    {
		const char *attr[10];
		
		attr[0] = "tag";
		attr[1] = tag;
		attr[2] = 0;
		
		res = data1_mk_tag(p->dh, p->mem, "controlfield", attr, res);
	    }
	}
        parent = res;
        /* traverse sub fields */
        i0 = i;
        while (buf[i] != ISO2709_RS && buf[i] != ISO2709_FS && i < end_offset)
        {
	    if (memcmp (tag, "00", 2) && identifier_length)
            {
		data1_node *res;
		if (marc_xml)
		{
		    int j;
		    const char *attr[3];
		    char code[10];
		    
		    for (j = 1; j<identifier_length && j < 9; j++)
			code[j-1] = buf[i+j];
		    code[j-1] = 0;
		    attr[0] = "code";
		    attr[1] = code;
		    attr[2] = 0;
		    res = data1_mk_tag(p->dh, p->mem, "subfield",
				       attr, parent);
		}
		else
		{
		    res = data1_mk_tag_n (p->dh, p->mem,
					   buf+i+1, identifier_length-1, 
					   0 /* attr */, parent);
		}
#if MARC_DEBUG
                fprintf (outf, " $"); 
                for (j = 1; j<identifier_length; j++)
                    fprintf (outf, "%c", buf[j+i]);
                fprintf (outf, " ");
#endif
                i += identifier_length;
                i0 = i;
            	while (buf[i] != ISO2709_RS && buf[i] != ISO2709_IDFS &&
		     buf[i] != ISO2709_FS && i < end_offset)
            	{
#if MARC_DEBUG
            	    fprintf (outf, "%c", buf[i]);
#endif
		    i++;
            	}
            	data1_mk_text_n (p->dh, p->mem, buf + i0, i - i0, res);
		i0 = i;
            }
            else
            {
#if MARC_DEBUG
                fprintf (outf, "%c", buf[i]);
#endif
                i++;
            }
        }
        if (i > i0)
	{
            data1_mk_text_n (p->dh, p->mem, buf + i0, i - i0, parent);
	}
#if MARC_DEBUG
        fprintf (outf, "\n");
        if (i < end_offset)
            fprintf (outf, "-- separator but not at end of field\n");
        if (buf[i] != ISO2709_RS && buf[i] != ISO2709_FS)
            fprintf (outf, "-- no separator at end of field\n");
#endif
    }
    return res_root;
}
/*
 * Locate some data under this node. This routine should handle variants
 * prettily.
 */
static char *get_data(data1_node *n, int *len)
{
    char *r;

    while (n)
    {
        if (n->which == DATA1N_data)
        {
            int i;
            *len = n->u.data.len;

            for (i = 0; i<*len; i++)
                if (!d1_isspace(n->u.data.data[i]))
                    break;
            while (*len && d1_isspace(n->u.data.data[*len - 1]))
                (*len)--;
            *len = *len - i;
            if (*len > 0)
                return n->u.data.data + i;
        }
        if (n->which == DATA1N_tag)
            n = n->child;
	else if (n->which == DATA1N_data)
            n = n->next;
	else
            break;	
    }
    r = "";
    *len = strlen(r);
    return r;
}
static data1_node *lookup_subfield(data1_node *node, const char *name)
{
    data1_node *p;
    
    for (p=node; p; p=p->next)
    {
	if (!yaz_matchstr(p->u.tag.tag, name))
	    return p;
    }
    return 0;
}
static inline_subfield *lookup_inline_subfield(inline_subfield *pisf, const char *name)
{
    inline_subfield *p;
    
    for (p=pisf; p; p=p->next)
    {
	if (!yaz_matchstr(p->name, name))
	    return p;
    }
    return 0;
}
static inline_subfield *cat_inline_subfield(mc_subfield *psf, char *buf, inline_subfield *pisf)
{
    mc_subfield *p;
    
    for (p = psf; p && pisf; p = p->next)
    {
	if (p->which == MC_SF)
	{
	    inline_subfield *found = lookup_inline_subfield(pisf, p->name);
	    
	    if (found)
	    {
	    	if (strcmp(p->prefix, "_"))
		{
		    strcat(strcat(buf, " "), p->prefix);
		}
		if (p->interval.start == -1)
		{
		    strcat(buf, found->data);
		}
		else
		{
		    strncat(buf, found->data+p->interval.start,
			p->interval.end-p->interval.start+1);
		}
	    	if (strcmp(p->suffix, "_"))
		{
		    strcat(strcat(buf, p->suffix), " ");
		}
#if MARCOMP_DEBUG
		logf(LOG_LOG, "cat_inline_subfield(): add subfield $%s", found->name);
#endif		
		pisf = found->next;
	    }
	}
	else if (p->which == MC_SFVARIANT)
	{
	    inline_subfield *next;
	    
	    do {
		next = cat_inline_subfield(p->u.child, buf, pisf);
		if (next == pisf)
		    break;
		pisf = next;
	    } while (pisf);
	}
	else if (p->which == MC_SFGROUP)
	{
	    mc_subfield *pp;
	    int found;
	    
	    for (pp = p->u.child, found = 0; pp; pp = pp->next)
	    {
		if (!yaz_matchstr(pisf->name, p->name))
		{
		    found = 1;
		    break;
		}
	    }
	    if (found)
	    {
		strcat(buf, " (");
		pisf = cat_inline_subfield(p->u.child, buf, pisf);
		strcat(buf, ") ");
	    }
	}
    }
    return pisf; 
}
static void cat_inline_field(mc_field *pf, char *buf, data1_node *subfield)
{    
    if (!pf || !subfield)
	return;

    for (;subfield;)
    {
	int len;
	inline_field *pif=NULL;
	data1_node *psubf;
	
	if (yaz_matchstr(subfield->u.tag.tag, "1"))
	{
	    subfield = subfield->next;
	    continue;
	}
	
	psubf = subfield;
	pif = inline_mk_field();
	do
	{
	    int i;
	    if ((i=inline_parse(pif, psubf->u.tag.tag, get_data(psubf, &len)))<0)
	    {
		logf(LOG_WARN, "inline subfield ($%s): parse error",
		    psubf->u.tag.tag);
		inline_destroy_field(pif);
		return;	
	    }
	    psubf = psubf->next;
	} while (psubf && yaz_matchstr(psubf->u.tag.tag, "1"));
	
	subfield = psubf;
	
	if (pif && !yaz_matchstr(pif->name, pf->name))
	{
	    if (!pf->list && pif->list)
	    {
		strcat(buf, pif->list->data);
	    }
	    else
	    {
		int ind1, ind2;

	        /*
		    check indicators
		*/

		ind1 = (pif->ind1[0] == ' ') ? '_':pif->ind1[0];
		ind2 = (pif->ind2[0] == ' ') ? '_':pif->ind2[0];
    
		if (((pf->ind1[0] == '.') || (ind1 == pf->ind1[0])) &&
		    ((pf->ind2[0] == '.') || (ind2 == pf->ind2[0])))
		{
		    cat_inline_subfield(pf->list, buf, pif->list);
		    
		    /*
		    	add separator for inline fields
		    */
		    if (strlen(buf))
		    {
		    	strcat(buf, "\n");
		    }
		}
		else
		{
		    logf(LOG_WARN, "In-line field %s missed -- indicators does not match", pif->name);
		}
	    }
	}
	inline_destroy_field(pif);
    }
#if MARCOMP_DEBUG    
    logf(LOG_LOG, "cat_inline_field(): got buffer {%s}", buf);
#endif
}
static data1_node *cat_subfield(mc_subfield *psf, char *buf, data1_node *subfield)
{
    mc_subfield *p;
    
    for (p = psf; p && subfield; p = p->next)
    {
	if (p->which == MC_SF)
	{
	    data1_node *found = lookup_subfield(subfield, p->name);
	    
	    if (found)
	    {
		int len;
		
		if (strcmp(p->prefix, "_"))
		{
		    strcat(strcat(buf, " "), p->prefix);
		}
		
		if (p->u.in_line)
		{
		    cat_inline_field(p->u.in_line, buf, found);
		}
		else if (p->interval.start == -1)
		{
		    strcat(buf, get_data(found, &len));
		}
		else
		{
		    strncat(buf, get_data(found, &len)+p->interval.start,
			p->interval.end-p->interval.start+1);
		}
		if (strcmp(p->suffix, "_"))
		{
		    strcat(strcat(buf, p->suffix), " ");
		}
#if MARCOMP_DEBUG		
		logf(LOG_LOG, "cat_subfield(): add subfield $%s", found->u.tag.tag);
#endif		
		subfield = found->next;
	    }
	}
	else if (p->which == MC_SFVARIANT)
	{
	    data1_node *next;
	    do {
		next = cat_subfield(p->u.child, buf, subfield);
		if (next == subfield)
		    break;
		subfield = next;
	    } while (subfield);
	}
	else if (p->which == MC_SFGROUP)
	{
	    mc_subfield *pp;
	    int found;
	    
	    for (pp = p->u.child, found = 0; pp; pp = pp->next)
	    {
		if (!yaz_matchstr(subfield->u.tag.tag, pp->name))
		{
		    found = 1;
		    break;
		}
	    }
	    if (found)
	    {
		strcat(buf, " (");
		subfield = cat_subfield(p->u.child, buf, subfield);
		strcat(buf, ") ");
	    }
	}
    }
    return subfield;
}
static data1_node *cat_field(struct grs_read_info *p, mc_field *pf, char *buf, data1_node *field)
{
    data1_node *subfield;
    int ind1, ind2;
    
    if (!pf || !field)
	return 0;

    
    if (yaz_matchstr(field->u.tag.tag, pf->name))
	return field->next;

    subfield = field->child;
    
    if (!subfield)
	return field->next;

    /*
	check subfield without indicators
    */
    
    if (!pf->list && subfield->which == DATA1N_data)
    {
	int len;
	
	if (pf->interval.start == -1)
	{
	    strcat(buf, get_data(field, &len));
	}
	else
	{
	    strncat(buf, get_data(field, &len)+pf->interval.start,
		pf->interval.end-pf->interval.start+1);
	}
#if MARCOMP_DEBUG
        logf(LOG_LOG, "cat_field(): got buffer {%s}", buf);
#endif
	return field->next;
    }
    
    /*
	check indicators
    */

    ind1 = (subfield->u.tag.tag[0] == ' ') ? '_':subfield->u.tag.tag[0];
    ind2 = (subfield->u.tag.tag[1] == ' ') ? '_':subfield->u.tag.tag[1];
    
    if (!(
	((pf->ind1[0] == '.') || (ind1 == pf->ind1[0])) &&
	((pf->ind2[0] == '.') || (ind2 == pf->ind2[0]))
	))
    {
#if MARCOMP_DEBUG
	logf(LOG_WARN, "Field %s missed -- does not match indicators", field->u.tag.tag);
#endif
	return field->next;
    }
    
    subfield = subfield->child;
    
    if (!subfield)
	return field->next;

    cat_subfield(pf->list, buf, subfield);

#if MARCOMP_DEBUG    
    logf(LOG_LOG, "cat_field(): got buffer {%s}", buf);
#endif
    
    return field->next;    
}
static int is_empty(char *s)
{
    char *p = s;
    
    for (p = s; *p; p++)
    {
	if (!isspace(*p))
	    return 0;
    }
    return 1;
}
static void parse_data1_tree(struct grs_read_info *p, const char *mc_stmnt, data1_node *root)
{
    data1_marctab *marctab = root->u.root.absyn->marc;
    data1_node *top = root->child;
    data1_node *field;
    mc_context *c;
    mc_field *pf;
    char buf[1000000];
    
    c = mc_mk_context(mc_stmnt+3);
    
    if (!c)
	return;
	
    pf = mc_getfield(c);
    
    if (!pf)
    {
	mc_destroy_context(c);
	return;
    }
#if MARCOMP_DEBUG    
    logf(LOG_LOG, "parse_data1_tree(): statement -{%s}", mc_stmnt);
#endif
    if (!yaz_matchstr(pf->name, "ldr"))
    {
	data1_node *new;
#if MARCOMP_DEBUG
	logf(LOG_LOG,"parse_data1_tree(): try LEADER from {%d} to {%d} positions",
	    pf->interval.start, pf->interval.end);
#endif	
	new = data1_mk_tag_n(p->dh, p->mem, mc_stmnt, strlen(mc_stmnt), 0, top);
	data1_mk_text_n(p->dh, p->mem, marctab->leader+pf->interval.start,
	    pf->interval.end-pf->interval.start+1, new);
    }
    else
    {
	field=top->child;
	
	while(field)
	{
	    if (!yaz_matchstr(field->u.tag.tag, pf->name))
	    {
		data1_node *new;
		char *pb = buf;
#if MARCOMP_DEBUG		
		logf(LOG_LOG, "parse_data1_tree(): try field {%s}", field->u.tag.tag);
#endif		
		*buf = '\0';	    
		field = cat_field(p, pf, buf, field);
		
		for (pb = strtok(pb, "\n"); pb; pb = strtok(NULL, "\n"))
		{
			if (!is_empty(pb))
			{
		    		new = data1_mk_tag_n(p->dh, p->mem, mc_stmnt, strlen(mc_stmnt), 0, top);
		    		data1_mk_text_n(p->dh, p->mem, pb, strlen(pb), new);
			}
		}
	    }
	    else
	    {
		field = field->next;
	    }
	}
    }
    mc_destroy_field(pf);
    mc_destroy_context(c);
}

data1_node *grs_read_marcxml(struct grs_read_info *p)
{
    data1_node *root = grs_read_iso2709(p, 1);
    data1_element *e;

    if (!root)
	return 0;
	
    for (e=root->u.root.absyn->main_elements; e; e=e->next)
    {
	data1_tag *tag = e->tag;
	
	if (tag && tag->which == DATA1T_string &&
	    !yaz_matchstr(tag->value.string, "mc?"))
		parse_data1_tree(p, tag->value.string, root);
    }
    return root;
}


data1_node *grs_read_marc(struct grs_read_info *p)
{
    data1_node *root = grs_read_iso2709(p, 0);
    data1_element *e;

    if (!root)
	return 0;
	
    for (e=root->u.root.absyn->main_elements; e; e=e->next)
    {
	data1_tag *tag = e->tag;
	
	if (tag && tag->which == DATA1T_string &&
	    !yaz_matchstr(tag->value.string, "mc?"))
		parse_data1_tree(p, tag->value.string, root);
    }
    return root;
}
static void *grs_init_marc(void)
{
    return 0;
}

static void grs_destroy_marc(void *clientData)
{
}

static struct recTypeGrs marc_type = {
    "marc",
    grs_init_marc,
    grs_destroy_marc,
    grs_read_marc
};

RecTypeGrs recTypeGrs_marc = &marc_type;

static struct recTypeGrs marcxml_type = {
    "marcxml",
    grs_init_marc,
    grs_destroy_marc,
    grs_read_marcxml
};

RecTypeGrs recTypeGrs_marcxml = &marcxml_type;
