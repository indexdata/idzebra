/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: trav.c,v $
 * Revision 1.3  1995-09-06 16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/01  14:06:36  adam
 * Split of work into more files.
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

#include <alexutil.h>
#include "index.h"

static void repository_extract_r (int cmd, char *rep)
{
    struct dir_entry *e;
    int i;
    struct stat fs;
    size_t rep_len = strlen (rep);

    e = dir_open (rep);
    if (!e)
        return;
    if (rep[rep_len-1] != '/')
        rep[rep_len] = '/';
    else
        --rep_len;
    for (i=0; e[i].name; i++)
    {
        strcpy (rep +rep_len+1, e[i].name);
        stat (rep, &fs);
        switch (fs.st_mode & S_IFMT)
        {
        case S_IFREG:
            file_extract (cmd, rep, rep);
            break;
        case S_IFDIR:
            repository_extract_r (cmd, rep);
            break;
        }
    }
    dir_free (&e);
}

void copy_file (const char *dst, const char *src)
{
    int d_fd = open (dst, O_WRONLY|O_CREAT, 0666);
    int s_fd = open (src, O_RDONLY);
    char *buf;
    size_t i, r, w;

    if (d_fd == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Cannot create %s", dst);
        exit (1);
    }
    if (s_fd == -1)
    {
        logf (LOG_FATAL|LOG_ERRNO, "Cannot open %s", src);
        exit (1);
    }
    if (!(buf = malloc (4096)))
    {
        logf (LOG_FATAL|LOG_ERRNO, "malloc");
        exit (1);
    }
    while ((r=read (s_fd, buf, 4096))>0)
        for (w = 0; w < r; w += i)
        {
            i = write (d_fd, buf + w, r - w);
            if (i == -1)
            {
                logf (LOG_FATAL|LOG_ERRNO, "write");
                exit (1);
            }
        }
    if (r)
    {
        logf (LOG_FATAL|LOG_ERRNO, "read");
        exit (1);
    }
    free (buf);
    close (d_fd);
    close (s_fd);
}

void del_file (const char *dst)
{
    unlink (dst);
}

void del_dir (const char *dst)
{
    logf (LOG_DEBUG, "rmdir of %s", dst);
    if (rmdir (dst) == -1)
        logf (LOG_ERRNO|LOG_WARN, "rmdir");
}

void repository_update_r (int cmd, char *dst, char *src);

void repository_add_tree (int cmd, char *dst, char *src)
{
    mkdir (dst, 0755);
    repository_update_r (cmd, dst, src);
}

void repository_del_tree (int cmd, char *dst, char *src)
{
    size_t dst_len = strlen (dst);
    size_t src_len = strlen (src);
    struct dir_entry *e_dst;
    int i_dst = 0;
    struct stat fs_dst;

    e_dst = dir_open (dst);

    dir_sort (e_dst);

    if (src[src_len-1] != '/')
        src[src_len] = '/';
    else
        --src_len;
    if (dst[dst_len-1] != '/')
        dst[dst_len] = '/';
    else
        --dst_len;
    while (e_dst[i_dst].name)
    {
        strcpy (dst +dst_len+1, e_dst[i_dst].name);
        strcpy (src +src_len+1, e_dst[i_dst].name);
        
        stat (dst, &fs_dst);
        switch (fs_dst.st_mode & S_IFMT)
        {
        case S_IFREG:
            file_extract ('d', dst, dst);
            del_file (dst);
            break;
        case S_IFDIR:
            repository_del_tree (cmd, dst, src);
            break;
        }
        i_dst++;
    }
    dir_free (&e_dst);
    if (dst_len > 0)
    {
        dst[dst_len] = '\0';
        del_dir (dst);
    }
}

void repository_update_r (int cmd, char *dst, char *src)
{
    struct dir_entry *e_dst, *e_src;
    int i_dst = 0, i_src = 0;
    struct stat fs_dst, fs_src;
    size_t dst_len = strlen (dst);
    size_t src_len = strlen (src);

    e_dst = dir_open (dst);
    e_src = dir_open (src);

    if (!e_dst && !e_src)
        return;
    if (!e_dst)
    {
        dir_free (&e_src);
        repository_add_tree (cmd, dst, src);
        return;
    }
    else if (!e_src)
    {
        dir_free (&e_dst);
        repository_del_tree (cmd, dst, src);
        return;
    }

    dir_sort (e_src);
    dir_sort (e_dst);

    if (src[src_len-1] != '/')
        src[src_len] = '/';
    else
        --src_len;
    if (dst[dst_len-1] != '/')
        dst[dst_len] = '/';
    else
        --dst_len;
    while (e_dst[i_dst].name || e_src[i_src].name)
    {
        int sd;

        if (e_dst[i_dst].name && e_src[i_src].name)
            sd = strcmp (e_dst[i_dst].name, e_src[i_src].name);
        else if (e_src[i_src].name)
            sd = 1;
        else
            sd = -1;
                
        if (sd == 0)
        {
            strcpy (dst +dst_len+1, e_dst[i_dst].name);
            strcpy (src +src_len+1, e_src[i_src].name);
            
            /* check type, date, length */

            stat (dst, &fs_dst);
            stat (src, &fs_src);
                
            switch (fs_dst.st_mode & S_IFMT)
            {
            case S_IFREG:
                if (fs_src.st_ctime > fs_dst.st_ctime)
                {
                    file_extract ('d', dst, dst);
                    file_extract ('a', src, dst);
                    copy_file (dst, src);
                }
                break;
            case S_IFDIR:
                repository_update_r (cmd, dst, src);
                break;
            }
            i_src++;
            i_dst++;
        }
        else if (sd > 0)
        {
            strcpy (dst +dst_len+1, e_src[i_src].name);
            strcpy (src +src_len+1, e_src[i_src].name);
            
            stat (src, &fs_src);
            switch (fs_src.st_mode & S_IFMT)
            {
            case S_IFREG:
                file_extract ('a', src, dst);
                copy_file (dst, src);
                break;
            case S_IFDIR:
                repository_add_tree (cmd, dst, src);
                break;
            }
            i_src++;
        }
        else 
        {
            strcpy (dst +dst_len+1, e_dst[i_dst].name);
            strcpy (src +src_len+1, e_dst[i_dst].name);
            
            stat (dst, &fs_dst);
            switch (fs_dst.st_mode & S_IFMT)
            {
            case S_IFREG:
                file_extract ('d', dst, dst);
                del_file (dst);
                break;
            case S_IFDIR:
                repository_del_tree (cmd, dst, src);
                break;
            }
            i_dst++;
        }
    }
    dir_free (&e_dst);
    dir_free (&e_src);
}

void repository (int cmd, const char *rep, const char *base_path)
{
    char rep_tmp1[2048];
    char rep_tmp2[2048];

    strcpy (rep_tmp1, rep);
    if (base_path)
    {
        strcpy (rep_tmp2, base_path);
        repository_update_r (cmd, rep_tmp2, rep_tmp1);
    }
    else
        repository_extract_r (cmd, rep_tmp1);
}

