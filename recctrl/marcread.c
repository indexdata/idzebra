/*
 * Copyright (C) 1997-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: marcread.c,v $
 * Revision 1.11.2.1  2002-04-09 14:55:40  adam
 * Fix attributes for MARC reader
 *
 * Revision 1.11  2000/05/15 15:32:51  adam
 * Added support for 64 bit input file support.
 *
 * Revision 1.10  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.9  1999/06/25 13:47:25  adam
 * Minor change that prevents MSVC warning.
 *
 * Revision 1.8  1999/05/26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.7  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.6  1999/02/02 14:51:27  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.5  1997/11/18 10:03:24  adam
 * Member num_children removed from data1_node.
 *
 * Revision 1.4  1997/10/27 14:34:26  adam
 * Fixed bug - data1 root node wasn't tagged at all!
 *
 * Revision 1.3  1997/09/24 13:36:51  adam
 * *** empty log message ***
 *
 * Revision 1.2  1997/09/17 12:19:21  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.1  1997/09/04 13:54:40  adam
 * Added MARC filter - type grs.marc.<syntax> where syntax refers
 * to abstract syntax. New method tellf in retrieve/extract method.
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include <yaz/log.h>
#include <yaz/yaz-util.h>
#include <yaz/marcdisp.h>
#include "grsread.h"

data1_node *data1_mk_node_wp (data1_handle dh, NMEM mem, data1_node *parent)
{
    data1_node *res = data1_mk_node (dh, mem);
    
    if (!parent)
        res->root = res;
    else
    {
        res->root = parent->root;
        res->parent = parent;
        if (!parent->child)
            parent->child = parent->last_child = res;
        else
            parent->last_child->next = res;
        parent->last_child = res;
    }
    return res;
}

static void destroy_data (struct data1_node *n)
{
    assert (n->which == DATA1N_data);
    xfree (n->u.data.data);
}

data1_node *data1_mk_node_text (data1_handle dh, NMEM mem, data1_node *parent,
                                const char *buf, size_t len)
{
    data1_node *res = data1_mk_node_wp (dh, mem, parent);
    res->which = DATA1N_data;
    res->u.data.formatted_text = 0;
    res->u.data.what = DATA1I_text;
    res->u.data.len = len;
    if (res->u.data.len > DATA1_LOCALDATA) {
        res->u.data.data = (char *) xmalloc (res->u.data.len);
        res->destroy = destroy_data;
    }
    else
        res->u.data.data = res->lbuf;
    memcpy (res->u.data.data, buf, res->u.data.len);
    return res;
}

data1_node *data1_mk_node_tag (data1_handle dh, NMEM mem, data1_node *parent,
                               const char *tag, size_t len)
{
    data1_element *elem = NULL;
    data1_node *partag = get_parent_tag(dh, parent);
    data1_node *res;
    data1_element *e = NULL;
    int localtag = 0;
    
    res = data1_mk_node_wp (dh, mem, parent);

    res->which = DATA1N_tag;
    res->u.tag.tag = res->lbuf;
    res->u.tag.get_bytes = -1;

    if (len >= DATA1_LOCALDATA)
        len = DATA1_LOCALDATA-1;
#if DATA1_USING_XATTR
    res->u.tag.attributes = 0;
#endif

    memcpy (res->u.tag.tag, tag, len);
    res->u.tag.tag[len] = '\0';
   
    if (parent->which == DATA1N_variant)
        return res;
    if (partag)
        if (!(e = partag->u.tag.element))
            localtag = 1;
    
    elem = data1_getelementbytagname (dh, res->root->u.root.absyn, e,
                                      res->u.tag.tag);
    res->u.tag.element = elem;
    res->u.tag.node_selected = 0;
    res->u.tag.make_variantlist = 0;
    res->u.tag.no_data_requested = 0;
    return res;
}

#define MARC_DEBUG 0

data1_node *grs_read_marc (struct grs_read_info *p)
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

    data1_node *res_root;
    data1_absyn *absyn;
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
    logf (LOG_DEBUG, "absynName = %s", absynName);
    if (!(absyn = data1_get_absyn (p->dh, absynName)))
    {
        logf (LOG_WARN, "Unknown abstract syntax: %s", absynName);
        return NULL;
    }
    res_root = data1_mk_node_wp (p->dh, p->mem, NULL);
    res_root->which = DATA1N_root;
    res_root->u.root.type = (char *) nmem_malloc (p->mem, strlen(absynName)+1);
    strcpy (res_root->u.root.type, absynName);
    res_root->u.root.absyn = absyn;

    marctab = absyn->marc;

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
        data1_node *parent = res_root;

        memcpy (tag, buf+entry_p, 3);
        entry_p += 3;
        tag[3] = '\0';

        /* generate field node */
        res = data1_mk_node_tag (p->dh, p->mem, res_root, tag, 3);

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
#if MARC_DEBUG
            int j;
#endif
            res = data1_mk_node_tag (p->dh, p->mem, res, buf+i,
				     indicator_length);
#if MARC_DEBUG
            for (j = 0; j<indicator_length; j++)
                fprintf (outf, "%c", buf[j+i]);
#endif
            i += indicator_length;
        }
        parent = res;
        /* traverse sub fields */
        i0 = i;
        while (buf[i] != ISO2709_RS && buf[i] != ISO2709_FS && i < end_offset)
        {
            if (memcmp (tag, "00", 2) && identifier_length)
            {
	        data1_node *res =
		    data1_mk_node_tag (p->dh, p->mem, parent,
				       buf+i+1, identifier_length-1);
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
                data1_mk_node_text (p->dh, p->mem, res, buf + i0, i - i0);
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
	    data1_node *res = data1_mk_node_tag (p->dh, p->mem,
						 parent, "@", 1);
            data1_mk_node_text (p->dh, p->mem, res, buf + i0, i - i0);
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
