/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: extract.c,v $
 * Revision 1.9  1995-09-27 12:22:28  adam
 * More work on extract in record control.
 * Field name is not in isam keys but in prefix in dictionary words.
 *
 * Revision 1.8  1995/09/14  07:48:22  adam
 * Record control management.
 *
 * Revision 1.7  1995/09/11  13:09:32  adam
 * More work on relevance feedback.
 *
 * Revision 1.6  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.5  1995/09/06  16:11:16  adam
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
#include <recctrl.h>
#include "index.h"

static Dict file_idx;
static SYSNO sysno_next;
static int key_fd = -1;
static int sys_idx_fd = -1;
static char *key_buf;
static int key_offset, key_buf_size;
static int key_cmd;
static int key_sysno;

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
    key_buf_size = 49100;
    key_buf = xmalloc (key_buf_size);
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
    xfree (key_buf);
    return 1;
}

void wordFlush (int sysno)
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

static void wordInit (RecWord *p)
{
    p->attrSet = 1;
    p->attrUse = 1;
    p->which = Word_String;
}

static void wordAdd (const RecWord *p)
{
    struct it_key key;
    char x;
    size_t i;
    char wordPrefix[8];

    if (key_offset + 1000 > key_buf_size)
    {
        char *new_key_buf;

        key_buf_size *= 2;
        new_key_buf = xmalloc (2*key_buf_size);
        memcpy (new_key_buf, key_buf, key_offset);
        xfree (key_buf);
        key_buf = new_key_buf;
    }
    sprintf (wordPrefix, "%c%04d", p->attrSet + '0', p->attrUse);
    strcpy (key_buf + key_offset, wordPrefix);
    key_offset += strlen (wordPrefix);
    switch (p->which)
    {
    case Word_String:
        for (i = 0; p->u.string[i]; i++)
            key_buf[key_offset++] = index_char_cvt (p->u.string[i]);
        key_buf[key_offset++] = '\0';
        break;
    default:
        return ;
    }
    x = (key_cmd == 'a') ? 1 : 0;
    memcpy (key_buf + key_offset, &x, 1);
    key_offset++;

    key.sysno = key_sysno;
    key.seqno   = p->seqno;
    memcpy (key_buf + key_offset, &key, sizeof(key));
    key_offset += sizeof(key);
}

void file_extract (int cmd, const char *fname, const char *kname)
{
    int i;
    char ext[128];
    SYSNO sysno;
    char ext_res[128];
    const char *file_type;
    void *file_info;
    FILE *inf;
    struct recExtractCtrl extractCtrl;
    RecType rt;

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

    if (!(inf = fopen (fname, "r")))
    {
        logf (LOG_WARN|LOG_ERRNO, "open %s", fname);
        return;
    }
    if (!(rt = recType_byName (file_type)))
        return;
    extractCtrl.inf = inf;
    extractCtrl.subType = "";
    extractCtrl.init = wordInit;
    extractCtrl.add = wordAdd;
    key_sysno = sysno;
    key_cmd = cmd;
    (*rt->extract)(&extractCtrl);
    fclose (inf);
    wordFlush (sysno);
}


