/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: main.c,v $
 * Revision 1.1  1995-08-31 14:50:24  adam
 * New simple file index tool.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

#include <util.h>
#include "index.h"

char *prog;

static int key_fd = -1;
#define KEY_BUF_SIZE 100000
static char *key_buf;
int key_offset;
SYSNO sysno_next;
Dict file_idx;
static char *base_path = NULL;

void key_open (const char *fname)
{
    void *file_key;
    if (key_fd != -1)
        return;
    if ((key_fd = open (fname, O_RDWR|O_CREAT, 0666)) == -1)
    {
        log (LOG_FATAL|LOG_ERRNO, "Creat %s", fname);
        exit (1);
    }
    if (!(key_buf = malloc (KEY_BUF_SIZE)))
    {
        log (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    key_offset = 0;
    if (!(file_idx = dict_open ("fileidx", 10, 1)))
    {
        log (LOG_FATAL, "dict_open fail of %s", "fileidx");
        exit (1);
    }
    file_key = dict_lookup (file_idx, ".");
    if (file_key)
        memcpy (&sysno_next, (char*)file_key+1, sizeof(sysno_next));
    else
        sysno_next = 1;
}

void key_close (void)
{
    if (key_fd == -1)
        return;
    close (key_fd);
    dict_insert (file_idx, ".", sizeof(sysno_next), &sysno_next);
    dict_close (file_idx);
    key_fd = -1;
}

void key_flush (void)
{
    size_t i = 0;
    int w;
    
    while (i < key_offset)
    {
        w = write (key_fd, key_buf + i, key_offset - i);
        if (w == -1)
        {
            log (LOG_FATAL|LOG_ERRNO, "Write key fail");
            exit (1);
        }
        i += w;
    }
    key_offset = 0;
}

void key_write (int cmd, struct it_key *k, const char *str)
{
    char x = cmd;
    size_t slen = strlen(str);

    if (key_offset + sizeof(*k) + slen >= KEY_BUF_SIZE - 2)
        key_flush ();
    memcpy (key_buf + key_offset, &x, 1);
    key_offset++;
    memcpy (key_buf + key_offset, k, sizeof(*k));
    key_offset += sizeof(*k);
    memcpy (key_buf + key_offset, str, slen+1);
    key_offset += slen+1;
}

void text_extract (SYSNO sysno, int cmd, const char *fname)
{
    FILE *inf;
    struct it_key k;
    int seqno = 1;
    int c;
    char w[256];

    log (LOG_DEBUG, "Text extract of %d", sysno);
    k.sysno = sysno;
    inf = fopen (fname, "r");
    if (!inf)
    {
        log (LOG_WARN|LOG_ERRNO, "open %s", fname);
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
            k.field = 0;
            key_write (cmd, &k, w);
        }
        if (c == EOF)
            break;
    }
    fclose (inf);
}

void file_extract (int cmd, struct stat *fs, const char *fname)
{
    int i;
    char ext[128];
    SYSNO sysno;
    char ext_res[128];
    const char *file_type;
    void *file_info;

    log (LOG_DEBUG, "%c %s", cmd, fname);
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
    
    file_info = dict_lookup (file_idx, fname);
    if (!file_info)
    {
        sysno = sysno_next++;
        dict_insert (file_idx, fname, sizeof(sysno), &sysno);
    }
    else
        memcpy (&sysno, (char*) file_info+1, sizeof(sysno));
    if (!strcmp (file_type, "text"))
        text_extract (sysno, cmd, fname);
}

static void repository_extract_r (int cmd, char *rep)
{
    DIR *dir;
    size_t rep_len;
    struct dirent *dent;
    struct stat fs;

    rep_len = strlen(rep);
    dir = opendir(rep);
    if (!dir)
    {
        if (errno == ENOENT)
        {
            log (LOG_WARN|LOG_ERRNO, "opendir %s", rep);
            return;
        }
        log (LOG_FATAL|LOG_ERRNO, "opendir %s", rep);
        exit (1);
    }
    if (rep[rep_len-1] != '/')
        rep[rep_len] = '/';
    else
        --rep_len;
    while ((dent = readdir (dir)))
    {
        strcpy (rep +rep_len+1, dent->d_name);
        stat (rep, &fs);
        switch (fs.st_mode & S_IFMT)
        {
        case S_IFREG:
            file_extract (cmd, &fs, rep);
            break;
        case S_IFDIR:
            if (strcmp (dent->d_name, ".") && strcmp(dent->d_name, ".."))
                repository_extract_r (cmd, rep);
            break;
        }
    }
    closedir (dir);
}

void repository_update_r (int cmd, const char *rep, const char *with_rep)
{

}

void repository_traverse (int cmd, const char *rep)
{
    char rep_tmp1[2048];
    char rep_tmp2[2048];

    strcpy (rep_tmp1, rep);
    if (base_path)
    {
        strcpy (rep_tmp2, base_path);
        repository_update_r (cmd, rep_tmp1, rep_tmp2);
    }
    else
        repository_extract_r (cmd, rep_tmp1);
}


int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *base_name;

    prog = *argv;
    while ((ret = options ("b:v:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (!base_name)
            {
                base_name = arg;

                common_resource = res_open (base_name);
                if (!common_resource)
                {
                    log (LOG_FATAL, "Cannot open resource `%s'", base_name);
                    exit (1);
                }
            }
            else if(cmd == 0) /* command */
            {
                if (!strcmp (arg, "add"))
                {
                    cmd = 'a';
                }
                else if (!strcmp (arg, "del"))
                {
                    cmd = 'd';
                }
                else
                {
                    log (LOG_FATAL, "Unknown command: %s", arg);
                    exit (1);
                }
            }
            else
            {
                key_open ("keys.tmp");
                repository_traverse (cmd, arg);
                cmd = 0;
            }
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else if (ret == 'b')
        {
            base_path = arg;
        }
        else
        {
            log (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    key_flush ();
    key_close ();
    exit (0);
}
