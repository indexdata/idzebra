/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: main.c,v $
 * Revision 1.23  1995-11-30 08:34:31  adam
 * Started work on commit facility.
 * Changed a few malloc/free to xmalloc/xfree.
 *
 * Revision 1.22  1995/11/28  09:09:42  adam
 * Zebra config renamed.
 * Use setting 'recordId' to identify record now.
 * Bug fix in recindex.c: rec_release_blocks was invokeded even
 * though the blocks were already released.
 * File traversal properly deletes records when needed.
 *
 * Revision 1.21  1995/11/27  14:27:39  adam
 * Renamed 'update' command to 'dir'.
 *
 * Revision 1.20  1995/11/27  13:58:53  adam
 * New option -t. storeStore data implemented in server.
 *
 * Revision 1.19  1995/11/25  10:24:06  adam
 * More record fields - they are enumerated now.
 * New options: flagStoreData flagStoreKey.
 *
 * Revision 1.18  1995/11/22  17:19:17  adam
 * Record management uses the bfile system.
 *
 * Revision 1.17  1995/11/21  15:01:16  adam
 * New general match criteria implemented.
 * New feature: document groups.
 *
 * Revision 1.16  1995/11/20  11:56:27  adam
 * Work on new traversal.
 *
 * Revision 1.15  1995/11/01  16:25:51  quinn
 * *** empty log message ***
 *
 * Revision 1.14  1995/10/17  18:02:09  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.13  1995/10/10  12:24:39  adam
 * Temporary sort files are compressed.
 *
 * Revision 1.12  1995/10/04  16:57:20  adam
 * Key input and merge sort in one pass.
 *
 * Revision 1.11  1995/09/29  14:01:45  adam
 * Bug fixes.
 *
 * Revision 1.10  1995/09/28  14:22:57  adam
 * Sort uses smaller temporary files.
 *
 * Revision 1.9  1995/09/14  07:48:24  adam
 * Record control management.
 *
 * Revision 1.8  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.7  1995/09/05  15:28:39  adam
 * More work on search engine.
 *
 * Revision 1.6  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.5  1995/09/04  09:10:39  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 * Revision 1.4  1995/09/01  14:06:36  adam
 * Split of work into more files.
 *
 * Revision 1.3  1995/09/01  10:57:07  adam
 * Minor changes.
 *
 * Revision 1.2  1995/09/01  10:30:24  adam
 * More work on indexing. Not working yet.
 *
 * Revision 1.1  1995/08/31  14:50:24  adam
 * New simple file index tool.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <alexutil.h>
#include <data1.h>
#include "index.h"

char *prog;
size_t mem_max = 4*1024*1024;
extern char *data1_tabpath;

int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *configName = NULL;
    int nsections;
    int key_open_flag = 0;

    struct recordGroup rGroupDef;
    
    rGroupDef.groupName = NULL;
    rGroupDef.databaseName = NULL;
    rGroupDef.path = NULL;
    rGroupDef.recordId = NULL;
    rGroupDef.recordType = NULL;
    rGroupDef.flagStoreData = -1;
    rGroupDef.flagStoreKeys = -1;

    prog = *argv;
    if (argc < 2)
    {
        fprintf (stderr, "zebraidx [options] command <dir> ...\n"
        "Commands:\n"
        " update <dir>  Update index with files below <dir>.\n"
	"               If <dir> is empty filenames are read from stdin.\n"
        " delete <dir>  Delete index with files below <dir>.\n"
        "Options:\n"
	" -t <type>     Index files as <type> (grs or text).\n"
	" -c <config>   Read configuration file <config>.\n"
	" -g <group>    Index files according to group settings.\n"
	" -d <database> Records belong to Z39.50 database <database>.\n"
	" -m <mbytes>   Use <mbytes> before flushing keys to disk.\n"
	" -v <level>    Set logging to <level>.\n");
        exit (1);
    }
    while ((ret = options ("t:c:g:d:m:v:l:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if(cmd == 0) /* command */
            {
                if (!strcmp (arg, "update"))
                    cmd = 'u';
                else if (!strcmp (arg, "del") || !strcmp(arg, "delete"))
                    cmd = 'd';
                else
                {
                    logf (LOG_FATAL, "Unknown command: %s", arg);
                    exit (1);
                }
            }
            else
            {
                struct recordGroup rGroup;

                memcpy (&rGroup, &rGroupDef, sizeof(rGroup));
                if (!common_resource)
                {
                    common_resource = res_open (configName ?
                                                configName : FNAME_CONFIG);
                    if (!common_resource)
                    {
                        logf (LOG_FATAL, "Cannot open resource `%s'",
                              configName);
                        exit (1);
                    }
                    data1_tabpath = res_get (common_resource, "profilePath");
                    assert (data1_tabpath);
                }
                if (!key_open_flag)
                {
                    key_open (mem_max);
                    key_open_flag = 1;
                }
                rGroup.path = arg;
                if (cmd == 'u')
                    repositoryUpdate (&rGroup);
                else if (cmd == 'd')
                    repositoryDelete (&rGroup);
                cmd = 0;
            }
        }
        else if (ret == 'v')
        {
            log_init (log_mask_str(arg), prog, NULL);
        }
        else if (ret == 'm')
        {
            mem_max = 1024*1024*atoi(arg);
        }
        else if (ret == 'd')
        {
            rGroupDef.databaseName = arg;
        }
        else if (ret == 'g')
        {
            rGroupDef.groupName = arg;
        }
        else if (ret == 'c')
            configName = arg;
        else if (ret == 't')
            rGroupDef.recordType = arg;
        else if (ret == 'l')
            bf_cache (arg);
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (!key_open_flag)
        exit (0);
    nsections = key_close ();
    if (!nsections)
        exit (0);
    logf (LOG_LOG, "Merging with index");
    key_input (FNAME_WORD_DICT, FNAME_WORD_ISAM, nsections, 60);
    exit (0);
}

