/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: main.c,v $
 * Revision 1.61  1999-02-02 14:51:00  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.60  1998/10/28 10:54:39  adam
 * SDRKit integration.
 *
 * Revision 1.59  1998/10/16 08:14:32  adam
 * Updated record control system.
 *
 * Revision 1.58  1998/06/08 14:43:13  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.57  1998/05/20 10:12:18  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.56  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.55  1998/01/26 10:37:34  adam
 * Minor changes.
 *
 * Revision 1.54  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.53  1997/11/18 10:05:08  adam
 * Changed character map facility so that admin can specify character
 * mapping files for each register type, w, p, etc.
 *
 * Revision 1.52  1997/10/31 12:34:04  adam
 * Added missing nmem_init.
 *
 * Revision 1.51  1997/10/27 14:33:05  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.50  1997/09/25 14:55:52  adam
 * Minor changes.
 *
 * Revision 1.49  1997/09/17 12:19:15  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.48  1997/09/09 13:38:08  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.47  1997/02/12 20:39:46  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.46  1997/02/10 10:20:13  adam
 * Flag fileVerboseFlag set to 0 (default).
 *
 * Revision 1.45  1996/11/08 11:10:26  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.44  1996/10/29 14:09:48  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.43  1996/06/06 12:08:42  quinn
 * Added showRecord function
 *
 * Revision 1.42  1996/05/31  09:07:01  quinn
 * Work on character-set handling
 *
 * Revision 1.41  1996/05/14  14:04:34  adam
 * In zebraidx, the 'stat' command is improved. Statistics about ISAM/DICT
 * is collected.
 *
 * Revision 1.40  1996/04/26  10:00:23  adam
 * Added option -V to zebraidx to display version information.
 * Removed stupid warnings from file update.
 *
 * Revision 1.39  1996/04/09  10:05:21  adam
 * Bug fix: prev_name buffer possibly too small; allocated in key_file_init.
 *
 * Revision 1.38  1996/03/26  16:01:14  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.37  1996/03/19  12:43:26  adam
 * Bug fix: File update traversal didn't handle trailing slashes correctly.
 * Bug fix: Update of sub directory groups wasn't handled correctly.
 *
 * Revision 1.36  1996/02/12  18:45:37  adam
 * New fileVerboseFlag in record group control.
 *
 * Revision 1.35  1996/02/12  15:56:11  adam
 * New code command: unread.
 *
 * Revision 1.34  1996/02/07  14:06:39  adam
 * Better progress report during register merge.
 * New command: clean - removes temporary shadow files.
 *
 * Revision 1.33  1996/02/06  17:11:18  adam
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
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <data1.h>
#include "index.h"
#include "recindex.h"

#ifndef ZEBRASDR
#define ZEBRASDR 0
#endif

#if ZEBRASDR
#include "zebrasdr.h"
#endif

char *prog;

Res common_resource = 0;

int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *configName = NULL;
    int nsections;
    int disableCommit = 0;
    size_t mem_max = 0;

    struct recordGroup rGroupDef;

    nmem_init ();

#if ZEBRASDR
    zebraSdr_std ();
#endif
    rGroupDef.groupName = NULL;
    rGroupDef.databaseName = NULL;
    rGroupDef.path = NULL;
    rGroupDef.recordId = NULL;
    rGroupDef.recordType = NULL;
    rGroupDef.flagStoreData = -1;
    rGroupDef.flagStoreKeys = -1;
    rGroupDef.flagRw = 1;
    rGroupDef.databaseNamePath = 0;
    rGroupDef.explainDatabase = 0;
    rGroupDef.fileVerboseLimit = 100000;
    rGroupDef.zebra_maps = NULL;
    rGroupDef.dh = data1_create ();
    rGroupDef.recTypes = recTypes_init (rGroupDef.dh);
    recTypes_default_handlers (rGroupDef.recTypes);

    prog = *argv;
    if (argc < 2)
    {
        fprintf (stderr, "zebraidx [options] command <dir> ...\n"
        "Commands:\n"
        " update <dir>  Update index with files below <dir>.\n"
	"               If <dir> is empty filenames are read from stdin.\n"
        " delete <dir>  Delete index with files below <dir>.\n"
        " commit        Commit changes\n"
        " clean         Clean shadow files\n"
        "Options:\n"
	" -t <type>     Index files as <type> (grs or text).\n"
	" -c <config>   Read configuration file <config>.\n"
	" -g <group>    Index files according to group settings.\n"
	" -d <database> Records belong to Z39.50 database <database>.\n"
	" -m <mbytes>   Use <mbytes> before flushing keys to disk.\n"
        " -n            Don't use shadow system.\n"
	" -s            Show analysis on stdout, but do no work.\n"
	" -v <level>    Set logging to <level>.\n"
        " -f <n>        Display information for the first <n> records.\n"
        " -V            Show version.\n"
                 );
        exit (1);
    }
    while ((ret = options ("sVt:c:g:d:m:v:nf:l:", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            const char *rval;
            if(cmd == 0) /* command */
            {
                if (!common_resource)
                {
                    logf (LOG_LOG, "zebra version %s %s",
                          ZEBRAVER, ZEBRADATE);
                    common_resource = res_open (configName ?
                                                configName : FNAME_CONFIG);
                    if (!common_resource)
                    {
                        logf (LOG_FATAL, "cannot configuration file `%s'",
                              configName);
                        exit (1);
                    }
                    data1_set_tabpath (rGroupDef.dh, res_get (common_resource,
							      "profilePath"));

		    rGroupDef.bfs =
			bfs_create (res_get (common_resource, "register"));

                    bf_lockDir (rGroupDef.bfs,
				res_get (common_resource, "lockDir"));
		    rGroupDef.zebra_maps = zebra_maps_open (common_resource);
                }
                if (!strcmp (arg, "update"))
                    cmd = 'u';
                else if (!strcmp (arg, "update1"))
                    cmd = 'U';
                else if (!strcmp (arg, "update2"))
                    cmd = 'm';
                else if (!strcmp (arg, "dump"))
                    cmd = 's';
                else if (!strcmp (arg, "del") || !strcmp(arg, "delete"))
                    cmd = 'd';
                else if (!strcmp (arg, "commit"))
                {
                    rval = res_get (common_resource, "shadow");
                    zebraIndexLock (rGroupDef.bfs, 1, rval);
                    if (rval && *rval)
                        bf_cache (rGroupDef.bfs, rval);
                    else
                    {
                        logf (LOG_FATAL, "Cannot perform commit");
                        logf (LOG_FATAL, "No shadow area defined");
                        exit (1);
                    }
                    if (bf_commitExists (rGroupDef.bfs))
                    {
                        logf (LOG_LOG, "commit start");
                        zebraIndexLockMsg ("c");
                        zebraIndexWait (1);
                        logf (LOG_LOG, "commit execute");
                        bf_commitExec (rGroupDef.bfs);
#ifndef WIN32
                        sync ();
#endif
                        zebraIndexLockMsg ("d");
                        zebraIndexWait (0);
                        logf (LOG_LOG, "commit clean");
                        bf_commitClean (rGroupDef.bfs, rval);
                    }
                    else
                        logf (LOG_LOG, "cothing to commit");
                }
                else if (!strcmp (arg, "clean"))
                {
                    rval = res_get (common_resource, "shadow");
                    zebraIndexLock (rGroupDef.bfs, 1, rval);
                    if (bf_commitExists (rGroupDef.bfs))
                    {
                        zebraIndexLockMsg ("d");
                        zebraIndexWait (0);
                        logf (LOG_LOG, "commit clean");
                        bf_commitClean (rGroupDef.bfs, rval);
                    }
                    else
                        logf (LOG_LOG, "nothing to clean");
                }
                else if (!strcmp (arg, "stat") || !strcmp (arg, "status"))
                {
		    Records records;
                    rval = res_get (common_resource, "shadow");
                    zebraIndexLock (rGroupDef.bfs, 0, rval);
                    if (rval && *rval)
                    {
                        bf_cache (rGroupDef.bfs, rval);
                        zebraIndexLockMsg ("r");
                    }
		    records = rec_open (rGroupDef.bfs, 0);
                    rec_prstat (records);
		    rec_close (&records);
                    inv_prstat (rGroupDef.bfs);
                }
                else
                {
                    logf (LOG_FATAL, "unknown command: %s", arg);
                    exit (1);
                }
            }
	    else
            {
                struct recordGroup rGroup;

                rval = res_get (common_resource, "shadow");
                zebraIndexLock (rGroupDef.bfs, 0, rval);
		if (rGroupDef.flagRw)
		{
		    if (rval && *rval && !disableCommit)
		    {
			bf_cache (rGroupDef.bfs, rval);
			zebraIndexLockMsg ("r");
		    }
		    else
		    {
			bf_cache (rGroupDef.bfs, 0);
			zebraIndexLockMsg ("w");
		    }
		    zebraIndexWait (0);
		}
                memcpy (&rGroup, &rGroupDef, sizeof(rGroup));
                rGroup.path = arg;
                switch (cmd)
                {
                case 'u':
                    if (!key_open (&rGroup, mem_max))
		    {
			logf (LOG_LOG, "updating %s", rGroup.path);
			repositoryUpdate (&rGroup);
			nsections = key_close (&rGroup);
		    }
                    break;
                case 'U':
                    if (!key_open (&rGroup, mem_max))
		    {
			logf (LOG_LOG, "updating (pass 1) %s", rGroup.path);
			repositoryUpdate (&rGroup);
			key_close (&rGroup);
		    }
                    nsections = 0;
                    break;
                case 'd':
                    if (!key_open (&rGroup,mem_max))
		    {
			logf (LOG_LOG, "deleting %s", rGroup.path);
			repositoryDelete (&rGroup);
			nsections = key_close (&rGroup);
		    }
                    break;
                case 's':
                    logf (LOG_LOG, "dumping %s", rGroup.path);
                    repositoryShow (&rGroup);
                    nsections = 0;
                    break;
                case 'm':
                    nsections = -1;
                    break;
                default:
                    nsections = 0;
                }
                cmd = 0;
                if (nsections)
                {
                    logf (LOG_LOG, "merging with index");
                    key_input (rGroup.bfs, nsections, 60);
#ifndef WIN32
                    sync ();
#endif
                }
                log_event_end (NULL, NULL);
            }
        }
        else if (ret == 'V')
        {
            fprintf (stderr, "Zebra %s %s\n", ZEBRAVER, ZEBRADATE);
        }
        else if (ret == 'v')
            log_init_level (log_mask_str(arg));
	else if (ret == 'l')
	    log_init_file (arg);
        else if (ret == 'm')
            mem_max = 1024*1024*atoi(arg);
        else if (ret == 'd')
            rGroupDef.databaseName = arg;
	else if (ret == 's')
	    rGroupDef.flagRw = 0;
        else if (ret == 'g')
            rGroupDef.groupName = arg;
        else if (ret == 'f')
            rGroupDef.fileVerboseLimit = atoi(arg);
        else if (ret == 'c')
            configName = arg;
        else if (ret == 't')
            rGroupDef.recordType = arg;
        else if (ret == 'n')
            disableCommit = 1;
        else
            logf (LOG_WARN, "unknown option '-%s'", arg);
    }
    if (common_resource)
    {
        zebraIndexUnlock ();
	bfs_destroy (rGroupDef.bfs);
    }
    data1_destroy (rGroupDef.dh);
    exit (0);
    return 0;
}

