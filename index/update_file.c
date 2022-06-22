/* This file is part of the Zebra server.
   Copyright (C) Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#define S_ISREG(x) (x & _S_IFREG)
#define S_ISDIR(x) (x & _S_IFDIR)
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <direntz.h>
#include <fcntl.h>
#include <time.h>

#include "index.h"

#if 0
static int dump_file_dict_func(char *name, const char *info, int pos,
                                void *client)
{
    yaz_log(YLOG_LOG, "%s", name);
    return 0;
}
static void dump_file_dict(Dict dict)
{
    int before = 10;
    int after = 1000;
    char term[1000];

    strcpy(term, "0");
    dict_scan(dict, term, &before, &after, 0, dump_file_dict_func);
}
#endif

static int repComp(const char *a, const char *b, size_t len)
{
    if (!len)
        return 0;
    return memcmp(a, b, len);
}

static void fileDelete_r(ZebraHandle zh,
                         struct dirs_info *di, struct dirs_entry *dst,
                         const char *base, char *src)
{
    char tmppath[1024];
    size_t src_len = strlen(src);

    while (dst && !repComp(dst->path, src, src_len+1))
    {
        switch (dst->kind)
        {
        case dirs_file:
            sprintf(tmppath, "%s%s", base, dst->path);
            zebra_extract_file(zh, &dst->sysno, tmppath, action_delete);
            strcpy(tmppath, dst->path);
            dst = dirs_read(di);
            dirs_del(di, tmppath);
            break;
        case dirs_dir:
            strcpy(tmppath, dst->path);
            dst = dirs_read(di);
            dirs_rmdir(di, tmppath);
            break;
        default:
            dst = dirs_read(di);
        }
    }
}

static void file_update_r(ZebraHandle zh,
                          struct dirs_info *di, struct dirs_entry *dst,
                          const char *base, char *src,
                          int level)
{
    struct dir_entry *e_src;
    int i_src = 0;
    static char tmppath[1024];
    size_t src_len = strlen(src);

    sprintf(tmppath, "%s%s", base, src);
    e_src = dir_open(tmppath, zh->path_reg, zh->m_follow_links);
    yaz_log(YLOG_LOG, "dir %s", tmppath);

#if 0
    if (!dst || repComp(dst->path, src, src_len))
#else
    if (!dst || strcmp(dst->path, src))
#endif
    {
        if (!e_src)
            return;

        if (src_len && src[src_len-1] != '/')
        {
            src[src_len] = '/';
            src[++src_len] = '\0';
        }
        dirs_mkdir(di, src, 0);
        if (dst && repComp(dst->path, src, src_len))
            dst = NULL;
    }
    else if (!e_src)
    {
        strcpy(src, dst->path);
        fileDelete_r(zh, di, dst, base, src);
        return;
    }
    else
    {
        if (src_len && src[src_len-1] != '/')
        {
            src[src_len] = '/';
            src[++src_len] = '\0';
        }
        dst = dirs_read(di);
    }
    dir_sort(e_src);

    while (1)
    {
        int sd;

        if (dst && !repComp(dst->path, src, src_len))
        {
            if (e_src[i_src].name)
            {
                yaz_log(YLOG_DEBUG, "dst=%s src=%s", dst->path + src_len,
                      e_src[i_src].name);
                sd = strcmp(dst->path + src_len, e_src[i_src].name);
            }
            else
                sd = -1;
        }
        else if (e_src[i_src].name)
            sd = 1;
        else
            break;
        yaz_log(YLOG_DEBUG, "trav sd=%d", sd);

        if (sd == 0)
        {
            strcpy(src + src_len, e_src[i_src].name);
            sprintf(tmppath, "%s%s", base, src);

            switch(e_src[i_src].kind)
            {
            case dirs_file:
                if (e_src[i_src].mtime > dst->mtime)
                {
                    if (zebra_extract_file(zh, &dst->sysno, tmppath, action_update) == ZEBRA_OK)
                    {
                        dirs_add(di, src, dst->sysno, e_src[i_src].mtime);
                    }
                    yaz_log(YLOG_DEBUG, "old: %s", ctime(&dst->mtime));
                    yaz_log(YLOG_DEBUG, "new: %s", ctime(&e_src[i_src].mtime));
                }
                dst = dirs_read(di);
                break;
            case dirs_dir:
                file_update_r(zh, di, dst, base, src, level+1);
                dst = dirs_last(di);
                yaz_log(YLOG_DEBUG, "last is %s", dst ? dst->path : "null");
                break;
            default:
                dst = dirs_read(di);
            }
            i_src++;
        }
        else if (sd > 0)
        {
            zint sysno = 0;
            strcpy(src + src_len, e_src[i_src].name);
            sprintf(tmppath, "%s%s", base, src);

            switch (e_src[i_src].kind)
            {
            case dirs_file:
                if (zebra_extract_file(zh, &sysno, tmppath, action_update) == ZEBRA_OK)
                    dirs_add(di, src, sysno, e_src[i_src].mtime);
                break;
            case dirs_dir:
                file_update_r(zh, di, dst, base, src, level+1);
                if (dst)
                    dst = dirs_last(di);
                break;
            }
            i_src++;
        }
        else  /* sd < 0 */
        {
            strcpy(src, dst->path);
            sprintf(tmppath, "%s%s", base, dst->path);

            switch (dst->kind)
            {
            case dirs_file:
                zebra_extract_file(zh, &dst->sysno, tmppath, action_delete);
                dirs_del(di, dst->path);
                dst = dirs_read(di);
                break;
            case dirs_dir:
                fileDelete_r(zh, di, dst, base, src);
                dst = dirs_last(di);
            }
        }
    }
    dir_free(&e_src);
}

static void file_update_top(ZebraHandle zh, Dict dict, const char *path)
{
    struct dirs_info *di;
    struct stat sbuf;
    char src[1024];
    char dst[1024];
    int src_len, ret;

    assert(path);

    if (zh->path_reg && !yaz_is_abspath(path))
    {
        strcpy(src, zh->path_reg);
        strcat(src, "/");
    }
    else
        *src = '\0';
    strcat(src, path);
    ret = zebra_file_stat(src, &sbuf, zh->m_follow_links);

    strcpy(src, path);
    src_len = strlen(src);

    if (ret == -1)
    {
        yaz_log(YLOG_WARN|YLOG_ERRNO, "Cannot access path %s", src);
    }
    else if (S_ISREG(sbuf.st_mode))
    {
        struct dirs_entry *e_dst;
        di = dirs_fopen(dict, src, zh->m_flag_rw);

        e_dst = dirs_read(di);
        if (e_dst)
        {
            if (sbuf.st_mtime > e_dst->mtime)
                if (zebra_extract_file(zh, &e_dst->sysno, src, action_update) == ZEBRA_OK)
                    dirs_add(di, src, e_dst->sysno, sbuf.st_mtime);
        }
        else
        {
            zint sysno = 0;
            if (zebra_extract_file(zh, &sysno, src, action_update) == ZEBRA_OK)
                 dirs_add(di, src, sysno, sbuf.st_mtime);
        }
        dirs_free(&di);
    }
    else if (S_ISDIR(sbuf.st_mode))
    {
        if (src_len && src[src_len-1] != '/')
        {
            src[src_len] = '/';
            src[++src_len] = '\0';
        }
        di = dirs_open(dict, src, zh->m_flag_rw);
        *dst = '\0';
        file_update_r(zh, di, dirs_read(di), src, dst, 0);
        dirs_free (&di);
    }
    else
    {
        yaz_log(YLOG_WARN, "Skipping path %s", src);
    }
}

static ZEBRA_RES zebra_open_fmatch(ZebraHandle zh, Dict *dictp)
{
    char fmatch_fname[1024];
    int ord;

    ord = zebraExplain_get_database_ord(zh->reg->zei);
    sprintf(fmatch_fname, FMATCH_DICT, ord);
    if (!(*dictp = dict_open_res(zh->reg->bfs, fmatch_fname, 50,
                                zh->m_flag_rw, 0, zh->res)))
    {
        yaz_log(YLOG_FATAL, "dict_open fail of %s", fmatch_fname);
        return ZEBRA_FAIL;
    }
    return ZEBRA_OK;
}

ZEBRA_RES zebra_remove_file_match(ZebraHandle zh)
{
    Dict dict;

    if (zebra_open_fmatch(zh, &dict) != ZEBRA_OK)
        return ZEBRA_FAIL;

    dict_clean(dict);
    dict_close(dict);

    return ZEBRA_OK;
}

ZEBRA_RES zebra_update_file_match(ZebraHandle zh, const char *path)
{
    Dict dict;

    if (zebraExplain_curDatabase(zh->reg->zei, zh->basenames[0]))
    {
        if (zebraExplain_newDatabase(zh->reg->zei, zh->basenames[0], 0))
            return ZEBRA_FAIL;
    }
    if (zebra_open_fmatch(zh, &dict) != ZEBRA_OK)
        return ZEBRA_FAIL;

    if (!strcmp(path, "") || !strcmp(path, "-"))
    {
        char src[1024];
        while (scanf("%s", src) == 1)
            file_update_top(zh, dict, src);
    }
    else
        file_update_top(zh, dict, path);
#if 0
    dump_file_dict(dict);
#endif
    dict_close(dict);
    return ZEBRA_OK;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

