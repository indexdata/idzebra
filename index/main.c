/*
 * Copyright (C) 1994-2001, Index Data
 * All rights reserved.
 *
 * $Id: main.c,v 1.80 2001-11-19 23:05:22 adam Exp $
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <yaz/data1.h>
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
    char *configName = FNAME_CONFIG;
    int nsections = 0;
    int disableCommit = 0;
    size_t mem_max = 0;
    char nbuf[100];
    struct recordGroup rGroupDef;

    nmem_init ();

#ifdef WIN32
#else
    sprintf(nbuf, "%.40s(%d)", *argv, getpid());
    yaz_log_init_prefix (nbuf);
#endif

#if ZEBRASDR
    zebraSdr_std ();
    rGroupDef.useSDR = 0;
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
        fprintf (stderr, "%s [options] command <dir> ...\n"
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
        " -l <file>     Write log to <file>.\n"
        " -f <n>        Display information for the first <n> records.\n"
#if ZEBRASDR
	" -S            Use SDRKit\n"
#endif
        " -V            Show version.\n", *argv
                 );
        exit (1);
    }
    while ((ret = options ("sVt:c:g:d:m:v:nf:l:"
#if ZEBRASDR
			   "S"
#endif
			   , argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            const char *rval;
            if(cmd == 0) /* command */
            {
                if (!common_resource)
                {
#if ZMBOL
                    logf (LOG_LOG, "zmbol version %s %s",
                          ZEBRAVER, ZEBRADATE);
#else
                    logf (LOG_LOG, "zebra version %s %s",
                          ZEBRAVER, ZEBRADATE);
#endif
                    common_resource = res_open (configName ?
                                                configName : FNAME_CONFIG);
                    if (!common_resource)
                    {
                        logf (LOG_FATAL, "cannot read file `%s'", configName);
                        exit (1);
                    }
                    data1_set_tabpath (rGroupDef.dh, res_get (common_resource,
							      "profilePath"));

		    rGroupDef.bfs =
			bfs_create (res_get (common_resource, "register"));
                    if (!rGroupDef.bfs)
                    {
                        logf (LOG_FATAL, "Cannot access register");
                        exit(1);
                    }

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
		else if (!strcmp (arg, "init"))
		{
		    zebraIndexUnlock();	
		    rval = res_get (common_resource, "shadow");
		    zebraIndexLock (rGroupDef.bfs, 0, rval);
		    if (rval && *rval)
			bf_cache (rGroupDef.bfs, rval);
		    zebraIndexLockMsg ("w");
		    bf_reset (rGroupDef.bfs);
		}
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
                        logf (LOG_LOG, "nothing to commit");
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
		    records = rec_open (rGroupDef.bfs, 0, 0);
                    rec_prstat (records);
		    rec_close (&records);
                    inv_prstat (rGroupDef.bfs);
                }
                else if (!strcmp (arg, "compact"))
                {
                    rval = res_get (common_resource, "shadow");
                    zebraIndexLock (rGroupDef.bfs, 0, rval);
                    if (rval && *rval)
                    {
                        bf_cache (rGroupDef.bfs, rval);
                        zebraIndexLockMsg ("r");
                    }
                    inv_compact(rGroupDef.bfs);
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
#if ZMBOL
#else
		/* For zebra, delete lock file and reset register */
		if (rGroupDef.flagRw)
		{
		    zebraIndexUnlock();
		    bf_reset (rGroupDef.bfs);
		}
#endif
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
                    key_input (rGroup.bfs, nsections, 60, common_resource);
#ifndef WIN32
                    sync ();
#endif
                }
                log_event_end (NULL, NULL);
            }
        }
        else if (ret == 'V')
        {
#if ZMBOL
            fprintf (stderr, "Z'mbol %s %s\n", ZEBRAVER, ZEBRADATE);
#else
            fprintf (stderr, "Zebra %s %s\n", ZEBRAVER, ZEBRADATE);
#endif
	    fprintf (stderr, " (C) 1994-2001, Index Data ApS\n");
#ifdef WIN32
#ifdef _DEBUG
            fprintf (stderr, " WIN32 Debug\n");
#else
            fprintf (stderr, " WIN32 Release\n");
#endif
#endif
#if HAVE_BZLIB_H
            fprintf (stderr, "libbzip2\n"
		     " (C) 1996-1999 Julian R Seward.  All rights reserved.\n");
#endif
        }
        else if (ret == 'v')
            yaz_log_init_level (yaz_log_mask_str(arg));
	else if (ret == 'l')
	    yaz_log_init_file (arg);
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
#if ZEBRASDR
	else if (ret == 'S')
	    rGroupDef.useSDR = 1;
#endif
        else
            logf (LOG_WARN, "unknown option '-%s'", arg);
    }
    recTypes_destroy (rGroupDef.recTypes);
    if (common_resource)
    {
        zebraIndexUnlock ();
	bfs_destroy (rGroupDef.bfs);
    }
    data1_destroy (rGroupDef.dh);
    exit (0);
    return 0;
}

