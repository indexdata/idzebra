/* $Id: testlib.c,v 1.30 2006-03-31 15:58:05 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

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

/** testlib - utilities for the api tests */

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <assert.h>
#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>
#include "testlib.h"

/** start_log: open a log file */
/*    FIXME - parse command line arguments to set log levels etc */
int log_level=0; /* not static, t*.c may use it */

void tl_start_log(int argc, char **argv)
{
    int cmd_level = 0;
    char logname[2048];
    if (!argv) 
        return;
    if (!argv[0])
        return;
    sprintf(logname, "%s.log", argv[0]);
    yaz_log_init_file(logname);
    log_level = yaz_log_mask_str_x(argv[0], 0);
    if (argc >= 2)
	cmd_level |= yaz_log_mask_str_x(argv[1], 0);
    yaz_log_init_level(YLOG_DEFAULT_LEVEL | log_level | cmd_level);
    yaz_log(log_level, "starting %s", argv[0]);
}

/** 
 * tl_start_up : do common start things, and a zebra_start
 *    - nmem_init
 *    - build the name of logfile from argv[0], and open it
 *      if no argv passed, do not open a log
 *    - read zebra.cfg from env var srcdir if it exists; otherwise current dir 
 *      default to zebra.cfg, if no name is given
 */
ZebraService tl_start_up(char *cfgname, int argc, char **argv)
{
#if HAVE_SYS_RESOURCE_H
#if HAVE_SYS_TIME_H
    struct rlimit rlim;
    rlim.rlim_cur = 20;
    rlim.rlim_max = 20;
    setrlimit(RLIMIT_CPU, &rlim);
#endif
#endif
    nmem_init();
    tl_start_log(argc, argv);
    return tl_zebra_start(cfgname);
}

/**
 * get_srcdir: return env srcdir or . (if does does not exist)
 */
const char *tl_get_srcdir()
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir = ".";
    return srcdir;

}
/** tl_zebra_start - do a zebra_start with a decent config name */
ZebraService tl_zebra_start(const char *cfgname)
{
    char cfg[256];
    const char *srcdir = tl_get_srcdir();
    if (!cfgname || ! *cfgname )
        cfgname="zebra.cfg";

    sprintf(cfg, "%.200s/%.50s", srcdir, cfgname);
    return  zebra_start(cfg);
}

/** tl_close_down closes down the zebra, logfile, nmem, xmalloc etc. logs an OK */
int tl_close_down(ZebraHandle zh, ZebraService zs)
{
    if (zh)
        zebra_close(zh);
    if (zs)
        zebra_stop(zs);

    nmem_exit();
    xmalloc_trav("x");
    return 1;
}

/** inits the database and inserts test data */

int tl_init_data(ZebraHandle zh, const char **recs)
{
    ZEBRA_RES res;

    if (!zh)
	return 0;

    if (zebra_select_database(zh, "Default") != ZEBRA_OK)
	return 0;

    yaz_log(log_level, "going to call init");
    res = zebra_init(zh);
    if (res == ZEBRA_FAIL) 
    {
	yaz_log(log_level, "init_data: zebra_init failed with %d", res);
        printf("init_data failed with %d\n", res);
	return 0;
    }
    if (recs)
    {
	int i;
	if (zebra_begin_trans (zh, 1) != ZEBRA_OK)
	    return 0;
        for (i = 0; recs[i]; i++)
            zebra_add_record(zh, recs[i], strlen(recs[i]));
        if (zebra_end_trans(zh) != ZEBRA_OK)
	    return 0;
        zebra_commit(zh);
    }
    return 1;
}

int tl_query_x(ZebraHandle zh, const char *query, zint exphits, int experror)
{
    ODR odr;
    YAZ_PQF_Parser parser;
    Z_RPNQuery *rpn;
    const char *setname="rsetname";
    zint hits;
    ZEBRA_RES rc;

    yaz_log(log_level, "======================================");
    yaz_log(log_level, "query: %s", query);
    odr = odr_createmem (ODR_DECODE);
    if (!odr)
	return 0;

    parser = yaz_pqf_create();
    rpn = yaz_pqf_parse(parser, odr, query);
    yaz_pqf_destroy(parser);
    if (!rpn)
    {
	odr_destroy(odr);
	return 0;
    }

    rc = zebra_search_RPN(zh, odr, rpn, setname, &hits);
    if (experror)
    {
	int code;
	if (rc != ZEBRA_FAIL)
	{
	    yaz_log(log_level, "search returned %d (OK), but error was "
		    "expected", rc);
	    printf("Error: search returned %d (OK), but error was expected\n"
		   "%s\n",  rc, query);
	    return 0;
	}
	code = zebra_errCode(zh);
	if (code != experror)
	{
	    yaz_log(log_level, "search returned error code %d, but error %d "
		    "was expected", code, experror);
	    printf("Error: search returned error code %d, but error %d was "
		   "expected\n%s\n",
		   code, experror, query);
	    return 0;
	}
    }
    else
    {
	if (rc == ZEBRA_FAIL) {
	    int code = zebra_errCode(zh);
	    yaz_log(log_level, "search returned %d. Code %d", rc, code);
	    
	    printf("Error: search returned %d. Code %d\n%s\n", rc, 
		   code, query);
	    return 0;
	}
	if (exphits != -1 && hits != exphits)
	{
	    yaz_log(log_level, "search returned " ZINT_FORMAT 
		   " hits instead of " ZINT_FORMAT, hits, exphits);
	    printf("Error: search returned " ZINT_FORMAT 
		   " hits instead of " ZINT_FORMAT "\n%s\n",
		   hits, exphits, query);
	    return 0;
	}
    }
    odr_destroy(odr);
    return 1;
}


int tl_query(ZebraHandle zh, const char *query, zint exphits)
{
    return tl_query_x(zh, query, exphits, 0);
}

int tl_scan(ZebraHandle zh, const char *query,
	    int pos, int num,
	    int exp_pos, int exp_num, int exp_partial,
	    const char **exp_entries)
{
    ODR odr = odr_createmem(ODR_ENCODE);
    ZebraScanEntry *entries = 0;
    int partial = -123;
    ZEBRA_RES res;

    yaz_log(log_level, "======================================");
    yaz_log(log_level, "scan: pos=%d num=%d %s", pos, num, query);

    res = zebra_scan_PQF(zh, odr, query, &pos, &num, &entries, &partial, 
			 0 /* setname */);
    if (res != ZEBRA_OK)
    {
	printf("Error: scan returned %d (FAIL), but no error was expected\n"
	       "%s\n",  res, query);
	return 0;
    }
    else
    {
	int fails = 0;
	if (partial == -123)
	{
	    printf("Error: scan returned OK, but partial was not set\n"
		   "%s\n", query);
	    fails++;
	}
	if (partial != exp_partial)
	{
	    printf("Error: scan returned OK, with partial/expected %d/%d\n"
		   "%s\n", partial, exp_partial, query);
	    fails++;
	}
	if (num != exp_num)
	{
	    printf("Error: scan returned OK, with num/expected %d/%d\n"
		   "%s\n", num, exp_num, query);
	    fails++;
	}
	if (pos != exp_pos)
	{
	    printf("Error: scan returned OK, with pos/expected %d/%d\n"
		   "%s\n", pos, exp_pos, query);
	    fails++;
	}
	if (fails)
	    return 0;
	fails = 0;
	if (exp_entries)
	{
	    int i;
	    for (i = 0; i<num; i++)
	    {
		if (strcmp(exp_entries[i], entries[i].term))
		{
		    printf("Error: scan OK, but entry %d term/exp %s/%s\n"
			   "%s\n",
			   i, entries[i].term, exp_entries[i], query);
		    fails++;
		}
	    }
	}
	if (fails)
	    return 0;
    }
    odr_destroy(odr);
    return 1;
}

/** 
 * makes a query, checks number of hits, and for the first hit, that 
 * it contains the given string, and that it gets the right score
 */
int tl_ranking_query(ZebraHandle zh, char *query, 
		     int exphits, char *firstrec, int firstscore)
{
    ZebraRetrievalRecord retrievalRecord[10];
    ODR odr_output = odr_createmem (ODR_ENCODE);    
    const char *setname="rsetname";
    int rc;
    int i;
        
    if (!tl_query(zh, query, exphits))
	return 0;

    for (i = 0; i<10; i++)
        retrievalRecord[i].position = i+1;

    rc = zebra_records_retrieve (zh, odr_output, setname, 0,
				 VAL_TEXT_XML, exphits, retrievalRecord);
    if (rc != ZEBRA_OK)
	return 0;

    if (!strstr(retrievalRecord[0].buf, firstrec))
    {
        printf("Error: Got the wrong record first\n");
        printf("Expected '%s' but got\n", firstrec);
        printf("%.*s\n", retrievalRecord[0].len, retrievalRecord[0].buf);
	return 0;
    }
    
    if (retrievalRecord[0].score != firstscore)
    {
        printf("Error: first rec got score %d instead of %d\n",
	       retrievalRecord[0].score, firstscore);
	return 0;
    }
    odr_destroy (odr_output);
    return 1;
}

int tl_meta_query(ZebraHandle zh, char *query, int exphits,
		  zint *ids)
{
    ZebraMetaRecord *meta;
    ODR odr_output = odr_createmem (ODR_ENCODE);    
    const char *setname="rsetname";
    zint *positions = (zint *) malloc(1 + (exphits * sizeof(zint)));
    int i;
        
    if (!tl_query(zh, query, exphits))
	return 0;
    
    for (i = 0; i<exphits; i++)
        positions[i] = i+1;

    meta = zebra_meta_records_create (zh, setname,  exphits, positions);
    
    if (!meta)
    {
        printf("Error: retrieve returned error\n%s\n", query);
	return 0;
    }

    for (i = 0; i<exphits; i++)
    {
	if (meta[i].sysno != ids[i])
	{
	    printf("Expected id=" ZINT_FORMAT " but got id=" ZINT_FORMAT "\n",
		   ids[i], meta[i].sysno);
	    return 0;
	}
    }
    zebra_meta_records_destroy(zh, meta, exphits);
    odr_destroy (odr_output);
    free(positions);
    return 1;
}

int tl_sort(ZebraHandle zh, const char *query, zint hits, zint *exp)
{
    ZebraMetaRecord *recs;
    zint i;
    int errs = 0;
    zint min_val_recs = 0;
    zint min_val_exp = 0;

    assert(query);
    if (!tl_query(zh, query, hits))
	return 0;

    recs = zebra_meta_records_create_range (zh, "rsetname", 1, 4);
    if (!recs)
	return 0;

    /* find min for each sequence to get proper base offset */
    for (i = 0; i<hits; i++)
    {
	if (min_val_recs == 0 || recs[i].sysno < min_val_recs)
	    min_val_recs = recs[i].sysno;
	if (min_val_exp == 0 || exp[i] < min_val_exp)
	    min_val_exp = exp[i];
    }
	    
    /* compare sequences using base offset */
    for (i = 0; i<hits; i++)
	if ((recs[i].sysno-min_val_recs) != (exp[i]-min_val_exp))
	    errs++;
    if (errs)
    {
	printf("Sequence not in right order for query\n%s\ngot exp\n",
	       query);
	for (i = 0; i<hits; i++)
	    printf(" " ZINT_FORMAT "   " ZINT_FORMAT "\n",
		   recs[i].sysno, exp[i]);
    }
    zebra_meta_records_destroy (zh, recs, 4);

    if (errs)
	return 0;
    return 1;
}


struct finfo {
    const char *name;
    int occurred;
};

static void filter_cb(void *cd, const char *name)
{
    struct finfo *f = (struct finfo*) cd;
    if (!strcmp(f->name, name))
	f->occurred = 1;
}

void tl_check_filter(ZebraService zs, const char *name)
{
    struct finfo f;

    f.name = name;
    f.occurred = 0;
    zebra_filter_info(zs, &f, filter_cb);
    if (!f.occurred)
    {
	yaz_log(YLOG_WARN, "Filter %s does not exist.", name);
	exit(0);
    }
}




