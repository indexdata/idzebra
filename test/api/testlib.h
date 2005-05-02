/* $Id: testlib.h,v 1.13 2005-05-02 09:05:22 adam Exp $
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

#include <stdlib.h>
#include <yaz/yconfig.h>
#include <yaz/pquery.h>
#include <yaz/log.h>
#include <idzebra/api.h>

/** 
 * start_up : Does all the usual start functions
 *    - nmem_init
 *    - build the name of logfile from argv[0], and open it
 *      if no argv passed, do not open a log
 *    - read zebra.cfg from env var srcdir if it exists; otherwise current dir 
 *      default to zebra.cfg, if no name is given
 */
ZebraService start_up(char *cfgname, int argc, char **argv);

/**
 * get_srcdir : returns the source dir. Most often ".", but when
 * making distcheck, some other dir 
 */
const char *get_srcdir();

/** 
 * start_log: open a log file 
 */
/*    FIXME - parse command line arguments to set log levels etc */
void start_log(int argc, char **argv);

/** 
 * start_service - do a zebra_start with a decent config name 
 * Takes care of checking the environment for srcdir (as needed by distcheck)
 * and uses that if need be. 
 * The name defaults to zebra.cfg, if null or emtpy
 */
ZebraService start_service(char *cfgname);


/** 
 * close_down closes it all down
 * Does a zebra_close on zh, if not null.
 * Does a zebra_stop on zs, if not null 
 * Writes a log message, OK if retcode is zero, error if not
 * closes down nmem and xmalloc
 * returns the retcode, for use in return or exit in main()
 */
int close_down(ZebraHandle zh, ZebraService zs, int retcode);

/** inits the database and inserts test data */
void init_data(ZebraHandle zh, const char **recs);


/**
 * do_query does a simple query, and checks that the number of hits matches
 */
int do_query(int lineno, ZebraHandle zh, char *query, int exphits);


/**
 * do_query does a simple query, and checks that error is what is expected
 */
int do_query_x(int lineno, ZebraHandle zh, char *query, int exphits,
	       int experror);

/**
 * do_scan is a utility for scan testing 
 */
void do_scan(int lineno, ZebraHandle zh, const char *query,
	     int pos, int num,  /* input params */
	     int exp_pos, int exp_num,  int exp_partial, /* expected result */
	     const char **exp_entries  /* expected entries (or NULL) */
    );

/** 
 * ranking_query makes a query, checks number of hits, and for 
 * the first hit, that it contains the given string, and that it 
 * gets the right score
 */
void ranking_query(int lineno, ZebraHandle zh, char *query, 
          int exphits, char *firstrec, int firstscore );

/** 
 * meta_query makes a query, checks number of hits, and for 
 * checks that the all records in result set has the proper identifiers (ids)
 */
void meta_query(int lineno, ZebraHandle zh, char *query, int exphits,
		zint *ids);

/**
 * if filter given by name does not exist, exit nicely but warn in log 
 */
void check_filter(ZebraService zs, const char *name);
