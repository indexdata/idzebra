/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: extract.c,v $
 * Revision 1.5  1995-09-06 16:11:16  adam
 * Option: only one word key per file.
 *
 * Revision 1.4  1995/09/05  15:28:39  adam
 * More work on search engine.
 *
 * Revision 1.3  1995/09/04  12:33:41  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.2  1995/09/04  09:10:34  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 * Revision 1.1  1995/09/01  14:06:35  adam
 * Split of work into more files.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include <alexutil.h>
#include "index.h"

#define KEY_BUF_SIZE 100000

static Dict file_idx;
static SYSNO sysno_next;
static int key_fd = -1;
static int sys_idx_fd = -1;
static char *key_buf;
static int key_offset;

void key_open (const char *fname)
{
    void *file_key;
    if (key_fd != -1)
        return;
    if ((key_fd = open (fname, O_RDWR|O_CREAT, 0666)) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", fname);
        exit (1);
    }
    logf (LOG_DEBUG, "key_open of %s", fname);
    if (!(key_buf = malloc (KEY_BUF_SIZE)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    key_offset = 0;
    if (!(file_idx = dict_open (FNAME_FILE_DICT, 40, 1)))
    {
        logf (LOG_FATAL, "dict_open fail of %s", "fileidx");
        exit (1);
    }
    file_key = dict_lookup (file_idx, ".");
    if (file_key)
        memcpy (&sysno_next, (char*)file_key+1, sizeof(sysno_next));
    else
        sysno_next = 1;
    if ((sys_idx_fd = open (FNAME_SYS_IDX, O_RDWR|O_CREAT, 0666)) == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "open %s", FNAME_SYS_IDX);
        exit (1);
    }
}

int key_close (void)
{
    if (key_fd == -1)
    {
        logf (LOG_DEBUG, "key_close - but no file");
        return 0;
    }
    close (key_fd);
    close (sys_idx_fd);
    dict_insert (file_idx, ".", sizeof(sysno_next), &sysno_next);
    dict_close (file_idx);
    key_fd = -1;
    logf (LOG_DEBUG, "key close - key file exist");
    return 1;
}

void key_flush (void)
{
    size_t i = 0;
    int w;

    if (key_fd == -1)
	return; 
    while (i < key_offset)
    {
        w = write (key_fd, key_buf + i, key_offset - i);
        if (w == -1)
        {
            logf (LOG_FATAL|LOG_ERRNO, "Write key fail");
            exit (1);
        }
        i += w;
    }
    key_offset = 0;
}

void key_write (int cmd, struct it_key *k, const char *str)
{
    char x;
    size_t slen = strlen(str);

    if (key_offset + sizeof(*k) + slen >= KEY_BUF_SIZE - 2)
        key_flush ();
    x = (cmd == 'a') ? 1 : 0;
    memcpy (key_buf + key_offset, str, slen+1);
    key_offset += slen+1;
    memcpy (key_buf + key_offset, &x, 1);
    key_offset++;
    memcpy (key_buf + key_offset, k, sizeof(*k));
    key_offset += sizeof(*k);
}

void key_write_x (struct strtab *t, int cmd, struct it_key *k, const char *str)
{
    void **oldinfo;

    if (strtab_src (t, str, &oldinfo))
        ((struct it_key *) *oldinfo)->seqno++;
    else
    {
        *oldinfo = xmalloc (sizeof(*k));
        memcpy (*oldinfo, k, sizeof(*k));
        ((struct it_key *) *oldinfo)->seqno = 1;
    }
}

void key_rec_flush (const char *str, void *info, void *data)
{
    key_write (*((int*) data), (struct it_key *)info, str);
    xfree (info);
}

void text_extract (struct strtab *t, SYSNO sysno, int cmd, const char *fname)
{
    FILE *inf;
    struct it_key k;
    int seqno = 1;
    int c;
    char w[256];

    logf (LOG_DEBUG, "Text extract of %d", sysno);
    k.sysno = sysno;
    inf = fopen (fname, "r");
    if (!inf)
    {
        logf (LOG_WARN|LOG_ERRNO, "open %s", fname);
        return;
    }
    while ((c=getc (inf)) != EOF)
    {
        int i = 0;
        while (i < 254 && c != EOF && isalnum(c))
        {
            w[i++] = c;
            c = getc (inf);
        }
        if (i)
        {
            w[i] = 0;
            
            k.seqno = seqno++;
#if IT_KEY_HAVE_FIELD
            k.field = 0;
#endif
            key_write_x (t, cmd, &k, w);
        }
        if (c == EOF)
            break;
    }
    fclose (inf);
}

void file_extract (int cmd, const char *fname, const char *kname)
{
    int i;
    char ext[128];
    SYSNO sysno;
    char ext_res[128];
    const char *file_type;
    void *file_info;
    struct strtab *t;

    logf (LOG_DEBUG, "%c %s k=%s", cmd, fname, kname);
    for (i = strlen(fname); --i >= 0; )
        if (fname[i] == '/')
        {
            strcpy (ext, "");
            break;
        }
        else if (fname[i] == '.')
        {
            strcpy (ext, fname+i+1);
            break;
        }
    sprintf (ext_res, "fileExtension.%s", ext);
    if (!(file_type = res_get (common_resource, ext_res)))
        return;
    
    file_info = dict_lookup (file_idx, kname);
    if (!file_info)
    {
        sysno = sysno_next++;
        dict_insert (file_idx, kname, sizeof(sysno), &sysno);
        lseek (sys_idx_fd, sysno * SYS_IDX_ENTRY_LEN, SEEK_SET);
        write (sys_idx_fd, kname, strlen(kname)+1);
    }
    else
        memcpy (&sysno, (char*) file_info+1, sizeof(sysno));
    t = strtab_mk ();
    if (!strcmp (file_type, "text"))
        text_extract (t, sysno, cmd, fname);
    strtab_del (t, key_rec_flush, &cmd);
}


