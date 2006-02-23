/* $Id: main.c,v 1.112.2.5 2006-02-23 14:30:03 adam Exp $
   Copyright (C) 1995-2005
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
#include <time.h>
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#if HAVE_TCL_H
#include <tcl.h>
#endif

#if HAVE_EXPAT_H
#include <expat.h>
#endif

#include <data1.h>
#include "zebraapi.h"

char *prog;

int main (int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *configName = 0;
    int nsections = 0;
    int enable_commit = 1;
    char *database = 0;
    Res res = res_open(0, 0, 0);
    
    int trans_started=0;
#if HAVE_SYS_TIMES_H
    struct tms tms1, tms2;
    struct timeval start_time, end_time;
    double usec;
#endif
#ifndef WIN32
    char nbuf[100];
#endif
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
    gettimeofday(&start_time, 0);
#endif
    prog = *argv;
    if (argc < 2)
    {
        fprintf (stderr, "%s [options] command <dir> ...\n"
        "Commands:\n"
        " update <dir>  Update index with files below <dir>.\n"
	"               If <dir> is empty filenames are read from stdin.\n"
        " delete <dir>  Delete index with files below <dir>.\n"
        " create <db>   Create database <db>\n"
        " drop <db>     Drop database <db>\n"
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
        " -L            Don't follow symbolic links.\n"
        " -f <n>        Display information for the first <n> records.\n"
        " -V            Show version.\n", *argv
                 );
        exit (1);
    }
    while ((ret = options ("sVt:c:g:d:m:v:nf:l:L"
			   , argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if(cmd == 0) /* command */
            {
                if (!zs)
                {
		    const char *config = configName ? configName : "zebra.cfg";
                    zs = zebra_start_res (config, 0, res);
                    if (!zs)
                    {
			yaz_log (LOG_FATAL, "Cannot read config %s", config);
                        exit (1);
	            }	
                    zh = zebra_open (zs);
		    zebra_shadow_enable (zh, enable_commit);
                }

		if (database && zebra_select_database (zh, database))
		{
		    logf(LOG_FATAL, "Could not select database %s errCode=%d",
			 database, zebra_errCode(zh) );
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
		else if (!strcmp(arg, "drop"))
		{
		    cmd = 'D';
		}
		else if (!strcmp(arg, "create"))
		{
		    cmd = 'C';
		}
                else if (!strcmp (arg, "commit"))
                {
                    zebra_commit (zh);
                }
                else if (!strcmp (arg, "clean"))
                {
                    zebra_clean (zh);
                }
                else if (!strcmp (arg, "stat") || !strcmp (arg, "status"))
                {
                    zebra_register_statistics (zh,0);
                }
                else if (!strcmp (arg, "dumpdict"))
                {
                    zebra_register_statistics (zh,1);
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
		if (!trans_started)
		{
		    trans_started=1;
                    if (zebra_begin_trans (zh, 1))
                        exit(1);
		}
                switch (cmd)
                {
                case 'u':
                    zebra_repository_update (zh, arg);
                    break;
                case 'd':
                    zebra_repository_delete (zh, arg);
                    break;
                case 's':
                    zebra_repository_show (zh, arg);
                    nsections = 0;
                    break;
		case 'C':
		    zebra_create_database(zh, arg);
		    break;
		case 'D':
		    zebra_drop_database(zh, arg);
		    break;
                default:
                    nsections = 0;
                }
                log_event_end (NULL, NULL);
            }
        }
        else if (ret == 'V')
        {
            printf("Zebra %s %s\n", ZEBRAVER, ZEBRADATE);
	    printf(" (C) 1994-2005, Index Data ApS\n");
#ifdef WIN32
#ifdef _DEBUG
            printf(" WIN32 Debug\n");
#else
            printf(" WIN32 Release\n");
#endif
#endif
#if HAVE_BZLIB_H
            printf("Using: libbzip2, (C) 1996-1999 Julian R Seward.  All rights reserved.\n");
#endif
#if HAVE_TCL_H
	    printf("Using: Tcl %s\n", TCL_VERSION);
#endif
#if HAVE_EXPAT_H
	    if (1)
	    {
		XML_Expat_Version v = XML_ExpatVersionInfo();
		printf ("Using: Expat %d.%d.%d\n",
			v.major, v.minor, v.micro);
	    }
#endif
        }
        else if (ret == 'v')
            yaz_log_init_level (yaz_log_mask_str(arg));
	else if (ret == 'l')
	    yaz_log_init_file (arg);
        else if (ret == 'm')
	    res_set(res, "memMax", arg);
        else if (ret == 'd')
            database = arg;
	else if (ret == 's')
	    res_set(res, "openRW", "0");
        else if (ret == 'g')
	    res_set(res, "group", arg);
        else if (ret == 'f')
	    res_set(res, "fileVerboseLimit", arg);
        else if (ret == 'c')
            configName = arg;
        else if (ret == 't')
	    res_set(res, "recordType", arg);
        else if (ret == 'n')
	    enable_commit = 0;
        else if (ret == 'L')
	    res_set(res, "followLinks", "0");
        else
            logf (LOG_WARN, "unknown option '-%s'", arg);
    } /* while arg */

    if (trans_started)
        zebra_end_trans (zh);

    zebra_close (zh);
    zebra_stop (zs);
#if HAVE_SYS_TIMES_H
    if (trans_started)
    {
        gettimeofday(&end_time, 0);
        usec = (end_time.tv_sec - start_time.tv_sec) * 1000000.0 +
	    end_time.tv_usec - start_time.tv_usec;
        times(&tms2);
        yaz_log (LOG_LOG, "zebraidx times: %5.2f %5.2f %5.2f",
		usec / 1000000,
		(double) (tms2.tms_utime - tms1.tms_utime)/100,
		(double) (tms2.tms_stime - tms1.tms_stime)/100);
    }
#endif
    nmem_exit();
    exit (0);
    return 0;
}

