/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: main.c,v $
 * Revision 1.33  1996-02-06 17:11:18  adam
 * Minor changes.
 *
 * Revision 1.32  1996/02/01  20:50:04  adam
 * Bug fix: zebraIndexUnlock was always called even though zebraIndexLock
 * was never called - happens when no commands are specified.
 *
 * Revision 1.31  1996/01/08  19:15:46  adam
 * New input filter that works!
 *
 * Revision 1.30  1995/12/12  16:00:59  adam
 * System call sync(2) used after update/commit.
 * Locking (based on fcntl) uses F_EXLCK and F_SHLCK instead of F_WRLCK
 * and F_RDLCK.
 *
 * Revision 1.29  1995/12/11  11:43:30  adam
 * Locking based on fcntl instead of flock.
 * Setting commitEnable removed. Command line option -n can be used to
 * prevent commit if commit setting is defined in the configuration file.
 *
 * Revision 1.28  1995/12/08  16:22:56  adam
 * Work on update while servers are running. Three lock files introduced.
 * The servers reload their registers when necessary, but they don't
 * reestablish result sets yet.
 *
 * Revision 1.27  1995/12/07  17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 * Revision 1.26  1995/12/06  12:41:23  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.25  1995/12/01  16:24:39  adam
 * Commit files use separate meta file area.
 *
 * Revision 1.24  1995/11/30  17:01:38  adam
 * New setting commitCache: points to commit directories/files.
 * New command commit: commits at the end of a zebraidx run.
 *
 * Revision 1.23  1995/11/30  08:34:31  adam
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

static void abort_func (int level, const char *msg, void *info)
{
    if (level & LOG_FATAL)
        abort ();
}

int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *configName = NULL;
    int nsections;
    int disableCommit = 0;

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
        " commit        Commit changes\n"
        "Options:\n"
	" -t <type>     Index files as <type> (grs or text).\n"
	" -c <config>   Read configuration file <config>.\n"
	" -g <group>    Index files according to group settings.\n"
	" -d <database> Records belong to Z39.50 database <database>.\n"
	" -m <mbytes>   Use <mbytes> before flushing keys to disk.\n"
        " -n            Don't use shadow system\n"
	" -v <level>    Set logging to <level>.\n");
        exit (1);
    }
    log_event_end (abort_func, NULL);
    while ((ret = options ("t:c:g:d:m:v:n", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            const char *rval;
            if(cmd == 0) /* command */
            {
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
                }
                if (!strcmp (arg, "update"))
                    cmd = 'u';
                else if (!strcmp (arg, "del") || !strcmp(arg, "delete"))
                    cmd = 'd';
                else if (!strcmp (arg, "commit"))
                {
                    zebraIndexLock (1);
                    rval = res_get (common_resource, "shadow");
                    if (rval && *rval)
                        bf_cache (1);
                    else
                    {
                        logf (LOG_FATAL, "Cannot perform commit");
                        logf (LOG_FATAL, "No shadow area defined");
                        exit (1);
                    }
                    if (bf_commitExists ())
                    {
                        logf (LOG_LOG, "Commit start");
                        zebraIndexLockMsg ("c");
                        zebraIndexWait (1);
                        logf (LOG_LOG, "Commit execute");
                        bf_commitExec ();
                        sync ();
                        zebraIndexLockMsg ("d");
                        zebraIndexWait (0);
                        logf (LOG_LOG, "Commit clean");
                        bf_commitClean ();
                    }
                    else
                        logf (LOG_LOG, "Nothing to commit");
                }
                else if (!strcmp (arg, "stat") || !strcmp (arg, "status"))
                {
                    zebraIndexLock (0);
                    rval = res_get (common_resource, "shadow");
                    if (rval && *rval)
                    {
                        bf_cache (1);
                        zebraIndexLockMsg ("r");
                    }
                    rec_prstat ();
                }
                else
                {
                    logf (LOG_FATAL, "Unknown command: %s", arg);
                    exit (1);
                }
            }
            else
            {
                struct recordGroup rGroup;

                zebraIndexLock (0);
                rval = res_get (common_resource, "shadow");
                if (rval && *rval && !disableCommit)
                {
                    bf_cache (1);
                    zebraIndexLockMsg ("r");
                }
                else
                {
                    bf_cache (0);
                    zebraIndexLockMsg ("w");
                }
                zebraIndexWait (0);

                memcpy (&rGroup, &rGroupDef, sizeof(rGroup));
                key_open (mem_max);
                rGroup.path = arg;
                if (cmd == 'u')
                {
                    logf (LOG_LOG, "Updating %s", rGroup.path);
                    repositoryUpdate (&rGroup);
                }
                else if (cmd == 'd')
                {
                    logf (LOG_LOG, "Deleting %s", rGroup.path);
                    repositoryDelete (&rGroup);
                }
                cmd = 0;
                nsections = key_close ();
                if (nsections)
                {
                    logf (LOG_LOG, "Merging with index");
                    key_input (FNAME_WORD_DICT, FNAME_WORD_ISAM, nsections,
                               60);
                    sync ();
                }
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
        else if (ret == 'n')
            disableCommit = 1;
        else
        {
            logf (LOG_FATAL, "Unknown option '-%s'", arg);
            exit (1);
        }
    }
    if (common_resource)
        zebraIndexUnlock ();
    exit (0);
}

