/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: kinput.c,v $
 * Revision 1.5  1995-09-29 14:01:43  adam
 * Bug fixes.
 *
 * Revision 1.4  1995/09/28  14:22:57  adam
 * Sort uses smaller temporary files.
 *
 * Revision 1.3  1995/09/06  16:11:17  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/04  12:33:42  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:37  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "index.h"

#define KEY_SIZE (1+sizeof(struct it_key))
#define INP_NAME_MAX 8192
#define INP_BUF_START 60000
#define INP_BUF_ADD  400000

static int no_diffs   = 0;
static int no_updates = 0;
static int no_insertions = 0;
static int no_iterations = 0;

static int read_one (FILE *inf, char *name, char *key)
{
    int c;
    int i = 0;
    do
    {
        if ((c=getc(inf)) == EOF)
            return 0;
        name[i++] = c;
    } while (c);
    for (i = 0; i<KEY_SIZE; i++)
        ((char *)key)[i] = getc (inf);
    ++no_iterations;
    return 1;
}

static int inp (Dict dict, ISAM isam, const char *name)
{
    FILE *inf;
    char *info;
    char next_name[INP_NAME_MAX+1];
    char cur_name[INP_NAME_MAX+1];
    int key_buf_size = INP_BUF_START;
    int key_buf_ptr;
    char *next_key;
    char *key_buf;
    int more;
    
    next_key = xmalloc (KEY_SIZE);
    key_buf = xmalloc (key_buf_size * (KEY_SIZE));
    if (!(inf = fopen (name, "r")))
    {
        logf (LOG_FATAL|LOG_ERRNO, "cannot open `%s'", name);
        exit (1);
    }
    more = read_one (inf, cur_name, key_buf);
    while (more)                   /* EOF ? */
    {
        int nmemb;
        key_buf_ptr = KEY_SIZE;
        while (1)
        {
            if (!(more = read_one (inf, next_name, next_key)))
                break;
            if (*next_name && strcmp (next_name, cur_name))
                break;
            memcpy (key_buf + key_buf_ptr, next_key, KEY_SIZE);
            key_buf_ptr += KEY_SIZE;
            if (key_buf_ptr+KEY_SIZE >= key_buf_size)
            {
                char *new_key_buf;
                new_key_buf = xmalloc (key_buf_size + INP_BUF_ADD);
                memcpy (new_key_buf, key_buf, key_buf_size);
                key_buf_size += INP_BUF_ADD;
                xfree (key_buf);
                key_buf = new_key_buf;
            }
        }
        no_diffs++;
        nmemb = key_buf_ptr / KEY_SIZE;
        assert (nmemb*KEY_SIZE == key_buf_ptr);
        if ((info = dict_lookup (dict, cur_name)))
        {
            ISAM_P isam_p, isam_p2;
            logf (LOG_DEBUG, "updating %s", cur_name);
            no_updates++;
            memcpy (&isam_p, info+1, sizeof(ISAM_P));
            isam_p2 = is_merge (isam, isam_p, nmemb, key_buf);
            if (isam_p2 != isam_p)
                dict_insert (dict, cur_name, sizeof(ISAM_P), &isam_p2);
        }
        else
        {
            ISAM_P isam_p;
            logf (LOG_DEBUG, "inserting %s", cur_name);
            no_insertions++;
            isam_p = is_merge (isam, 0, nmemb, key_buf);
            dict_insert (dict, cur_name, sizeof(ISAM_P), &isam_p);
        }
        memcpy (key_buf, next_key, KEY_SIZE);
        strcpy (cur_name, next_name);
    }
    fclose (inf);
    return 0;
}

void key_input (const char *dict_fname, const char *isam_fname,
                const char *key_fname, int cache)
{
    Dict dict;
    ISAM isam;

    dict = dict_open (dict_fname, cache, 1);
    if (!dict)
    {
        logf (LOG_FATAL, "dict_open fail of `%s'", dict_fname);
        exit (1);
    }
    isam = is_open (isam_fname, key_compare, 1, sizeof(struct it_key));
    if (!isam)
    {
        logf (LOG_FATAL, "is_open fail of `%s'", isam_fname);
        exit (1);
    }
    inp (dict, isam, key_fname);
    dict_close (dict);
    is_close (isam);
    logf (LOG_LOG, "Iterations . . .%7d", no_iterations);
    logf (LOG_LOG, "Distinct words .%7d", no_diffs);
    logf (LOG_LOG, "Updates. . . . .%7d", no_updates);
    logf (LOG_LOG, "Insertions . . .%7d", no_insertions);
}
