/*
 * Copyright (C) 1997-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: marcread.c,v 1.16 2002-07-05 12:43:30 adam Exp $
 */
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include <yaz/log.h>
#include <yaz/yaz-util.h>
#include <yaz/marcdisp.h>
#include "grsread.h"

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
    res_top = data1_mk_tag (p->dh, p->mem, absynName, 0, res_root);

    marctab = res_root->u.root.absyn->marc;

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


        /* generate field node */
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
#if MARC_DEBUG
            int j;
#endif
            res = data1_mk_tag_n (p->dh, p->mem, 
                                  buf+i, indicator_length, 0 /* attr */, res);
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
		    data1_mk_tag_n (p->dh, p->mem,
                                    buf+i+1, identifier_length-1, 
                                    0 /* attr */, parent);
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
