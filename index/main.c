/*
 * Copyright (C) 1994-2002, Index Data
 * All rights reserved.
 *
 * $Id: main.c,v 1.89 2002-04-26 08:44:47 adam Exp $
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#include <yaz/data1.h>
#include "zebraapi.h"

char *prog;

int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *configName = 0;
    int nsections = 0;
    int disableCommit = 0;
    size_t mem_max = 0;
#if HAVE_SYS_TIMES_H
    struct tms tms1, tms2;
#endif
#ifndef WIN32
    char nbuf[100];
#endif
    struct recordGroup rGroupDef;
    ZebraService zs = 0;
    ZebraHandle zh = 0;

    nmem_init ();

#ifdef WIN32
#else
    sprintf(nbuf, "%.40s(%d)", *argv, getpid());
    yaz_log_init_prefix (nbuf);
#endif
#if HAVE_SYS_TIMES_H
    times(&tms1);
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
        " -V            Show version.\n", *argv
                 );
        exit (1);
    }
    while ((ret = options ("sVt:c:g:d:m:v:nf:l:"
			   , argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if(cmd == 0) /* command */
            {
                if (!zs)
                {
#if ZMBOL
                    logf (LOG_LOG, "Z'mbol version %s %s",
                          ZEBRAVER, ZEBRADATE);
#else
                    logf (LOG_LOG, "Zebra version %s %s",
                          ZEBRAVER, ZEBRADATE);
#endif
                    zs = zebra_start (configName ? configName : "zebra.cfg");
                    if (!zs)
                        exit (1);
                    zh = zebra_open (zs);
                    if (disableCommit)
                        zebra_shadow_enable (zh, 0);
                }
                if (rGroupDef.databaseName)
                {
                    if (zebra_select_database (zh, rGroupDef.databaseName))
                        exit (1);
                }
                else
                {
                    if (zebra_select_database (zh, "Default"))
                        exit (1);
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
                    zebra_init (zh);
		}
                else if (!strcmp (arg, "commit"))
                {
                    zebra_commit (zh);
                }
                else if (!strcmp (arg, "clean"))
                {
                    assert (!"todo");
                }
                else if (!strcmp (arg, "stat") || !strcmp (arg, "status"))
                {
                    zebra_register_statistics (zh);
                }
                else if (!strcmp (arg, "compact"))
                {
                    zebra_compact (zh);
                }
                else
                {
                    logf (LOG_FATAL, "unknown command: %s", arg);
                    exit (1);
                }
            }
	    else
            {
                rGroupDef.path = arg;
                zebra_set_group (zh, &rGroupDef);
                zebra_begin_trans (zh);

                switch (cmd)
                {
                case 'u':
                    zebra_repository_update (zh);
                    break;
                case 'd':
                    zebra_repository_delete (zh);
                    break;
                case 's':
                    logf (LOG_LOG, "dumping %s", rGroupDef.path);
                    zebra_repository_show (zh);
                    nsections = 0;
                    break;
                default:
                    nsections = 0;
                }
                cmd = 0;
                zebra_end_trans (zh);
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
	    fprintf (stderr, " (C) 1994-2002, Index Data ApS\n");
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
        else
            logf (LOG_WARN, "unknown option '-%s'", arg);
    }
    zebra_close (zh);
    zebra_stop (zs);
#if HAVE_SYS_TIMES_H
    times(&tms2);
    yaz_log (LOG_LOG, "zebraidx user/system: %ld/%ld",
		(long) tms2.tms_utime - tms1.tms_utime,
		(long) tms2.tms_stime - tms1.tms_stime);
#endif
    exit (0);
    return 0;
}

