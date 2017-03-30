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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if YAZ_HAVE_ICU
#include <yaz/icu.h>
#endif
#include <yaz/log.h>
#include <yaz/options.h>
#include <idzebra/version.h>
#include <idzebra/api.h>

char *prog;

static void filter_cb(void *cd, const char *name)
{
    puts(name);
}

static void show_filters(ZebraService zs)
{
    zebra_filter_info(zs, 0, filter_cb);
}

int main(int argc, char **argv)
{
    int ret;
    int cmd = 0;
    char *arg;
    char *configName = 0;
    int enable_commit = 1;
    char *database = 0;
    Res res = res_open(0, 0);
    Res default_res = res_open(0, 0);

    int trans_started = 0;
#ifndef WIN32
    char nbuf[100];
#endif
    ZebraService zs = 0;
    ZebraHandle zh = 0;

#ifdef WIN32
#else
    sprintf(nbuf, "%.40s(%ld)", *argv, (long) getpid());
    yaz_log_init_prefix(nbuf);
#endif
    prog = *argv;
    if (argc < 2)
    {
        fprintf(stderr, "%s [options] command <dir> ...\n"
        "Commands:\n"
        " update <dir>  Update index with files below <dir>.\n"
	"               If <dir> is empty filenames are read from stdin.\n"
        " delete <dir>  Delete index with files below <dir>.\n"
        " create <db>   Create database <db>\n"
        " drop <db>     Drop database <db>\n"
        " commit        Commit changes\n"
        " clean         Clean shadow files\n"
        " check mode    Check register; mode is one of: default, full, quick\n"
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
        exit(1);
    }
    res_set(default_res, "profilePath", DEFAULT_PROFILE_PATH);
    res_set(default_res, "modulePath", DEFAULT_MODULE_PATH);
    while ((ret = options("sVt:c:g:d:m:v:nf:l:L", argv, argc, &arg)) != -2)
    {
        if (ret == 0)
        {
            if (cmd == 0) /* command */
            {
                if (!zs)
                {
		    const char *config = configName ? configName : "zebra.cfg";
                    zs = zebra_start_res(config, default_res, res);
                    if (!zs)
                    {
			yaz_log(YLOG_FATAL, "Cannot read config %s", config);
                        exit(1);
	            }
                    zh = zebra_open(zs, 0);
		    zebra_shadow_enable(zh, enable_commit);
                }

		if (database &&
		    zebra_select_database(zh, database) == ZEBRA_FAIL)
		{
		    yaz_log(YLOG_FATAL, "Could not select database %s "
			    "errCode=%d",
			    database, zebra_errCode(zh) );
		    exit(1);
		}
                if (!strcmp(arg, "update"))
                    cmd = 'u';
                else if (!strcmp(arg, "update1"))
                    cmd = 'U';
                else if (!strcmp(arg, "update2"))
                    cmd = 'm';
                else if (!strcmp(arg, "dump"))
                    cmd = 's';
                else if (!strcmp(arg, "del") || !strcmp(arg, "delete"))
                    cmd = 'd';
                else if (!strcmp(arg, "adelete"))
                    cmd = 'a';
		else if (!strcmp(arg, "init"))
		{
                    zebra_init(zh);
		}
		else if (!strcmp(arg, "drop"))
		{
		    cmd = 'D';
		}
		else if (!strcmp(arg, "create"))
		{
		    cmd = 'C';
		}
                else if (!strcmp(arg, "commit"))
                {
                    zebra_commit(zh);
                }
                else if (!strcmp(arg, "clean"))
                {
                    zebra_clean(zh);
                }
                else if (!strcmp(arg, "stat") || !strcmp(arg, "status"))
                {
                    zebra_register_statistics(zh, 0);
                }
                else if (!strcmp(arg, "dumpdict"))
                {
                    zebra_register_statistics(zh, 1);
                }
                else if (!strcmp(arg, "compact"))
                {
                    zebra_compact(zh);
                }
                else if (!strcmp(arg, "filters"))
                {
                    show_filters(zs);
                }
		else if (!strcmp(arg, "check"))
		{
                    cmd = 'K';
 		}
                else
                {
                    yaz_log(YLOG_FATAL, "unknown command: %s", arg);
                    exit(1);
                }
            }
	    else
            {
		ZEBRA_RES res = ZEBRA_OK;
		if (!trans_started)
		{
		    trans_started = 1;
                    if (zebra_begin_trans(zh, 1) != ZEBRA_OK)
                        exit(1);
		}
                switch (cmd)
                {
                case 'K':
                    res = zebra_register_check(zh, arg);
                    break;
                case 'u':
                    res = zebra_repository_index(zh, arg, action_update);
                    break;
                case 'd':
                    res = zebra_repository_index(zh, arg, action_delete);
                    break;
                case 'a':
                    res = zebra_repository_index(zh, arg, action_a_delete);
                    break;
                case 's':
                    res = zebra_repository_show(zh, arg);
                    break;
		case 'C':
		    res = zebra_create_database(zh, arg);
		    break;
		case 'D':
		    res = zebra_drop_database(zh, arg);
		    break;
                }
		if (res != ZEBRA_OK)
		{
                    const char *add = zebra_errAdd(zh);
		    yaz_log(YLOG_FATAL, "Operation failed: %s %s",
                            zebra_errString(zh), add ? add : "");

                    if (trans_started)
                        if (zebra_end_trans(zh) != ZEBRA_OK)
                            yaz_log(YLOG_WARN, "zebra_end_trans failed");


                    zebra_close(zh);
                    zebra_stop(zs);
		    exit(1);
		}
                log_event_end(NULL, NULL);
            }
        }
        else if (ret == 'V')
        {
            char version_str[20];
            char sys_str[80];
            zebra_get_version(version_str, sys_str);

            printf("Zebra %s\n", version_str);
	    printf("(C) 1994-2017, Index Data\n");
            printf("Zebra is free software, covered by the GNU General Public License, and you are\n");
            printf("welcome to change it and/or distribute copies of it under certain conditions.\n");
            printf("SHA1 ID: %s\n", sys_str);
            if (strcmp(version_str, ZEBRAVER))
                printf("zebraidx compiled version %s\n", ZEBRAVER);
#if YAZ_HAVE_ICU
            printf("Using ICU\n");
#endif
        }
        else if (ret == 'v')
            yaz_log_init_level(yaz_log_mask_str(arg));
	else if (ret == 'l')
	    yaz_log_init_file(arg);
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
            yaz_log(YLOG_WARN, "unknown option '-%s'", arg);
    } /* while arg */

    ret = 0;
    if (trans_started)
        if (zebra_end_trans(zh) != ZEBRA_OK)
        {
            yaz_log(YLOG_WARN, "zebra_end_trans failed");
            ret = 1;
        }

    zebra_close(zh);
    zebra_stop(zs);

    res_close(res);
    res_close(default_res);
    exit(ret);
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

