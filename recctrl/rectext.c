/*
 * Copyright (C) 1994-1998, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rectext.c,v $
 * Revision 1.12  1999-05-26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.11  1999/05/21 12:00:17  adam
 * Better diagnostics for extraction process.
 *
 * Revision 1.10  1999/05/20 12:57:18  adam
 * Implemented TCL filter. Updated recctrl system.
 *
 * Revision 1.9  1998/10/16 08:14:38  adam
 * Updated record control system.
 *
 * Revision 1.8  1998/05/20 10:12:27  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.7  1998/03/11 11:19:05  adam
 * Changed the way sequence numbers are generated.
 *
 * Revision 1.6  1998/02/10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.5  1997/10/27 14:33:06  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.4  1996/11/04 14:09:16  adam
 * Minor changes.
 *
 * Revision 1.3  1996/11/01 09:00:33  adam
 * This simple "text" format now supports element specs B and M.
 *
 * Revision 1.2  1996/10/29 14:02:45  adam
 * Uses buffered read to speed up things.
 *
 * Revision 1.1  1996/10/11 10:57:28  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 * Revision 1.7  1996/01/17 14:57:55  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int.
 *
 * Revision 1.6  1995/10/10  13:59:24  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.5  1995/10/02  16:24:39  adam
 * Use attribute actually used in search requests.
 *
 * Revision 1.4  1995/10/02  15:42:55  adam
 * Extract uses file descriptors instead of FILE pointers.
 *
 * Revision 1.3  1995/09/28  09:19:45  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.2  1995/09/15  14:45:21  adam
 * Retrieve control.
 * Work on truncation.
 *
 * Revision 1.1  1995/09/14  07:48:25  adam
 * Record control management.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <zebrautl.h>
#include "rectext.h"

static void *text_init (RecType recType)
{
    return 0;
}

static void text_destroy (void *clientData)
{
}

struct buf_info {
    struct recExtractCtrl *p;
    char *buf;
    int offset;
    int max;
};

struct buf_info *buf_open (struct recExtractCtrl *p)
{
    struct buf_info *fi = (struct buf_info *) xmalloc (sizeof(*fi));

    fi->p = p;
    fi->buf = (char *) xmalloc (4096);
    fi->offset = 1;
    fi->max = 1;
    return fi;
}

int buf_read (struct buf_info *fi, char *dst)
{
    if (fi->offset >= fi->max)
    {
        if (fi->max <= 0)
            return 0;
        fi->max = (*fi->p->readf)(fi->p->fh, fi->buf, 4096);
        fi->offset = 0;
        if (fi->max <= 0)
            return 0;
    }
    *dst = fi->buf[(fi->offset)++];
    return 1;
}

void buf_close (struct buf_info *fi)
{
    xfree (fi->buf);
    xfree (fi);
}

static int text_extract (void *clientData, struct recExtractCtrl *p)
{
    char w[512];
    RecWord recWord;
    int r;
    struct buf_info *fi = buf_open (p);

    (*p->init)(p, &recWord);
    recWord.reg_type = 'w';
    do
    {
        int i = 0;
            
        r = buf_read (fi, w);
        while (r > 0 && i < 511 && w[i] != '\n' && w[i] != '\r')
        {
            i++;
            r = buf_read (fi, w + i);
        }
        if (i)
        {
            recWord.string = w;
	    recWord.length = i;
            (*p->addWord)(&recWord);
        }
    } while (r > 0);
    buf_close (fi);
    return RECCTRL_EXTRACT_OK;
}

static int text_retrieve (void *clientData, struct recRetrieveCtrl *p)
{
    int r, text_ptr = 0;
    static char *text_buf = NULL;
    static int text_size = 0;
    int start_flag = 1;
    const char *elementSetName = NULL;
    int no_lines = 0;

    if (p->comp && p->comp->which == Z_RecordComp_simple &&
        p->comp->u.simple->which == Z_ElementSetNames_generic)
        elementSetName = p->comp->u.simple->u.generic;

    while (1)
    {
        if (text_ptr + 4096 >= text_size)
        {
            char *nb;

            text_size = 2*text_size + 8192;
            nb = (char *) xmalloc (text_size);
            if (text_buf)
            {
                memcpy (nb, text_buf, text_ptr);
                xfree (text_buf);
            }
            text_buf = nb;
        }
        if (start_flag)
        {
            start_flag = 0;
            if (p->score >= 0)
            {
                sprintf (text_buf, "Rank: %d\n", p->score);
                text_ptr = strlen(text_buf);
            }
            sprintf (text_buf + text_ptr, "Local Number: %d\n", p->localno);
            text_ptr = strlen(text_buf);
        }
        r = (*p->readf)(p->fh, text_buf + text_ptr, 4096);
        if (r <= 0)
            break;
        text_ptr += r;
    }
    text_buf[text_ptr] = '\0';
    if (elementSetName)
    {
        if (!strcmp (elementSetName, "B"))
            no_lines = 4;
        if (!strcmp (elementSetName, "M"))
            no_lines = 20;
    }
    if (no_lines)
    {
        char *p = text_buf;
        int i = 0;

        while (++i <= no_lines && (p = strchr (p, '\n')))
            p++;
        if (p)
        {
            p[1] = '\0';
            text_ptr = p-text_buf;
        }
    }
    p->output_format = VAL_SUTRS;
    p->rec_buf = text_buf;
    p->rec_len = text_ptr; 
    return 0;
}

static struct recType text_type = {
    "text",
    text_init,
    text_destroy,
    text_extract,
    text_retrieve
};

RecType recTypeText = &text_type;
