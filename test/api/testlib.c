/* $Id: testlib.c,v 1.10 2005-01-04 20:00:20 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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

/** testlib - utilities for the api tests */

#include <assert.h>
#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>
#include "testlib.h"

/** start_log: open a log file */
/*    FIXME - parse command line arguments to set log levels etc */
int log_level=0; /* not static, t*.c may use it */

void start_log(int argc, char **argv)
{
    char logname[2048];
    if (!argv) 
        return;
    if (!argv[0])
        return;
    sprintf(logname, "%s.log", argv[0]);
    yaz_log_init_file(logname);
    log_level = yaz_log_mask_str_x(argv[0],0);
    yaz_log_init_level(YLOG_DEFAULT_LEVEL | log_level);
    yaz_log(log_level,"starting %s",argv[0]);
}

/** 
 * start_up : do common start things, and a zebra_start
 *    - nmem_init
 *    - build the name of logfile from argv[0], and open it
 *      if no argv passed, do not open a log
 *    - read zebra.cfg from env var srcdir if it exists; otherwise current dir 
 *      default to zebra.cfg, if no name is given
 */
ZebraService start_up(char *cfgname, int argc, char **argv)
{
    nmem_init();
    start_log(argc, argv);
    return start_service(cfgname);
}

/**
 * get_srcdir: return env srcdir or . (if does does not exist)
 */
const char *get_srcdir()
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir=".";
    return srcdir;

}
/** start_service - do a zebra_start with a decent config name */
ZebraService start_service(char *cfgname)
{
    char cfg[256];
    const char *srcdir = get_srcdir();
    ZebraService zs;
    if (!cfgname || ! *cfgname )
        cfgname="zebra.cfg";

    sprintf(cfg, "%.200s/%.50s", srcdir, cfgname);
    zs=zebra_start(cfg);
    if (!zs)
    {
        printf("zebra_start failed, probably because missing config file \n"
               "check %s\n", cfg);
        exit(9);
    }
    return zs;
}


/** close_down closes down the zebra, logfile, nmem, xmalloc etc. logs an OK */
int close_down(ZebraHandle zh, ZebraService zs, int retcode)
{
    if (zh)
        zebra_close(zh);
    if (zs)
        zebra_stop(zs);

    if (retcode)
        yaz_log(log_level,"========= Exiting with return code %d", retcode);
    else
        yaz_log(log_level,"========= All tests OK");
    nmem_exit();
    xmalloc_trav("x");
    return retcode;
}

/** inits the database and inserts test data */

void init_data(ZebraHandle zh, const char **recs)
{
    int i;
    char *addinfo;
    assert(zh);
    zebra_select_database(zh, "Default");
    yaz_log(log_level, "going to call init");
    i = zebra_init(zh);
    yaz_log(log_level, "init returned %d",i);
    if (i) 
    {
        printf("init failed with %d\n",i);
        zebra_result(zh, &i, &addinfo);
        printf("  Error %d   %s\n",i,addinfo);
        exit(1);
    }
    if (recs)
    {
        zebra_begin_trans (zh, 1);
        for (i = 0; recs[i]; i++)
            zebra_add_record (zh, recs[i], strlen(recs[i]));
        zebra_end_trans (zh);
        zebra_commit (zh);
    }

}

int do_query(int lineno, ZebraHandle zh, char *query, int exphits)
{
    ODR odr;
    YAZ_PQF_Parser parser;
    Z_RPNQuery *rpn;
    const char *setname="rsetname";
    int hits;
    int rc;
        

    yaz_log(log_level,"======================================");
    yaz_log(log_level,"qry[%d]: %s", lineno, query);
    odr=odr_createmem (ODR_DECODE);    

    parser = yaz_pqf_create();
    rpn = yaz_pqf_parse(parser, odr, query);
    if (!rpn) {
        printf("Error: Parse failed \n%s\n",query);
        exit(1);
    }
    rc = zebra_search_RPN (zh, odr, rpn, setname, &hits);
    if (rc) {
        printf("Error: search returned %d \n%s\n",rc,query);
        exit (1);
    }

    if (hits != exphits) {
        printf("Error: search returned %d hits instead of %d\n",
                hits, exphits);
        exit (1);
    }
    yaz_pqf_destroy(parser);
    odr_destroy (odr);
    return hits;
}


/** 
 * makes a query, checks number of hits, and for the first hit, that 
 * it contains the given string, and that it gets the right score
 */
void ranking_query(int lineno, ZebraHandle zh, char *query, 
          int exphits, char *firstrec, int firstscore )
{
    ZebraRetrievalRecord retrievalRecord[10];
    ODR odr_output = odr_createmem (ODR_ENCODE);    
    const char *setname="rsetname";
    int hits;
    int rc;
    int i;
        
    hits = do_query(lineno, zh, query, exphits);

    for (i = 0; i<10; i++)
        retrievalRecord[i].position = i+1;

    rc = zebra_records_retrieve (zh, odr_output, setname, 0,
				 VAL_TEXT_XML, hits, retrievalRecord);
    
    if (rc)
    {
        printf("Error: retrieve returned %d \n%s\n",rc,query);
        exit (1);
    }

    if (!strstr(retrievalRecord[0].buf, firstrec))
    {
        printf("Error: Got the wrong record first\n");
        printf("Expected '%s' but got \n",firstrec);
        printf("%.*s\n",retrievalRecord[0].len,retrievalRecord[0].buf);
        exit(1);
    }
    
    if (retrievalRecord[0].score != firstscore)
    {
        printf("Error: first rec got score %d instead of %d\n",
	       retrievalRecord[0].score, firstscore);
        exit(1);
    }
    odr_destroy (odr_output);
}

void meta_query(int lineno, ZebraHandle zh, char *query, int exphits,
		zint *ids)
{
    ZebraMetaRecord *meta;
    ODR odr_output = odr_createmem (ODR_ENCODE);    
    const char *setname="rsetname";
    zint *positions = (zint *) malloc(1 + (exphits * sizeof(zint)));
    int hits;
    int i;
        
    hits = do_query(lineno, zh, query, exphits);
    
    for (i = 0; i<exphits; i++)
        positions[i] = i+1;

    meta = zebra_meta_records_create (zh, setname,  exphits, positions);
    
    if (!meta)
    {
        printf("Error: retrieve returned error\n%s\n", query);
        exit (1);
    }

    for (i = 0; i<exphits; i++)
    {
	if (meta[i].sysno != ids[i])
	{
	    printf("Expected id=" ZINT_FORMAT " but got id=" ZINT_FORMAT,
		   ids[i], meta[i].sysno);
	    exit(1);
	}
    }
    zebra_meta_records_destroy(zh, meta, exphits);
    odr_destroy (odr_output);
    free(positions);
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

void check_filter(ZebraService zs, const char *name)
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




