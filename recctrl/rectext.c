/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rectext.c,v $
 * Revision 1.4  1996-11-04 14:09:16  adam
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

static void text_init (void)
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
    struct buf_info *fi = xmalloc (sizeof(*fi));

    fi->p = p;
    fi->buf = xmalloc (4096);
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

static int text_extract (struct recExtractCtrl *p)
{
    char w[256];
    RecWord recWord;
    int r, seqno = 1;
    struct buf_info *fi = buf_open (p);

    (*p->init)(&recWord);
    recWord.which = Word_String;
    do
    {
        int i = 0;
            
        r = buf_read (fi, w);
        while (r > 0 && i < 255 && isalnum(w[i]))
        {
            i++;
            r = buf_read (fi, w + i);
        }
        if (i)
        {
            int j;
            for (j = 0; j<i; j++)
                w[j] = tolower(w[j]);
            w[i] = 0;
            recWord.seqno = seqno++;
            recWord.u.string = w;
            (*p->add)(&recWord);
        }
    } while (r > 0);
    buf_close (fi);
    return 0;
}

static int text_retrieve (struct recRetrieveCtrl *p)
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
            nb = xmalloc (text_size);
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
    text_extract,
    text_retrieve
};

RecType recTypeText = &text_type;
