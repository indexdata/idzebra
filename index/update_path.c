/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

static void repositoryExtractR(ZebraHandle zh, char *rep, int level,
                               enum zebra_recctrl_action_t action)
{
    struct dir_entry *e;
    int i;
    size_t rep_len = strlen(rep);

    e = dir_open(rep, zh->path_reg, zh->m_follow_links);
    if (!e)
        return;
    yaz_log(YLOG_LOG, "dir %s", rep);
    if (rep[rep_len-1] != '/')
        rep[rep_len] = '/';
    else
        --rep_len;
    
    for (i=0; e[i].name; i++)
    {
	char *ecp;
        strcpy(rep +rep_len+1, e[i].name);
	if ((ecp = strrchr(e[i].name, '/')))
	    *ecp = '\0';

        switch (e[i].kind)
        {
        case dirs_file:
            zebra_extract_file(zh, NULL, rep, action);
            break;
        case dirs_dir:
            repositoryExtractR(zh, rep, level+1, action);
            break;
        }
    }
    dir_free(&e);

}

void repositoryShow(ZebraHandle zh, const char *path)
{
    char src[1024];
    int src_len;
    struct dirs_entry *dst;
    Dict dict;
    struct dirs_info *di;

    if (!(dict = dict_open_res(zh->reg->bfs, FMATCH_DICT, 50, 0, 0, zh->res)))
    {
        yaz_log(YLOG_FATAL, "dict_open fail of %s", FMATCH_DICT);
	return;
    }
    
    strncpy(src, path, sizeof(src)-1);
    src[sizeof(src)-1]='\0';
    src_len = strlen(src);
    
    if (src_len && src[src_len-1] != '/')
    {
        src[src_len] = '/';
        src[++src_len] = '\0';
    }
    
    di = dirs_open(dict, src, zh->m_flag_rw);
    
    while ((dst = dirs_read(di)))
        yaz_log(YLOG_LOG, "%s", dst->path);
    dirs_free(&di);
    dict_close(dict);
}

static void repositoryExtract(ZebraHandle zh,
                              const char *path,
                              enum zebra_recctrl_action_t action)

{
    struct stat sbuf;
    char src[1024];
    int ret;

    assert (path);

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

    if (ret == -1)
        yaz_log(YLOG_WARN|YLOG_ERRNO, "Cannot access path %s", src);
    else if (S_ISREG(sbuf.st_mode))
        zebra_extract_file(zh, NULL, src, action);
    else if (S_ISDIR(sbuf.st_mode))
	repositoryExtractR(zh, src, 0, action);
    else
        yaz_log(YLOG_WARN, "Skipping path %s", src);
}


ZEBRA_RES zebra_update_from_path(ZebraHandle zh, const char *path, 
                                 enum zebra_recctrl_action_t action)
{
    if (!strcmp(path, "") || !strcmp(path, "-"))
    {
        char src[1024];
	
        while (scanf("%1020s", src) == 1)
            repositoryExtract(zh, src, action);
    }
    else
        repositoryExtract(zh, path, action);
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

