/* $Id: testlib.c,v 1.1 2004-10-28 10:37:15 heikki Exp $
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

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>


/* read zebra.cfg from env var srcdir if it exists; otherwise current dir */
ZebraService start_service(char *cfgname)
{
    char cfg[256];
    char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir=".";
    if (!cfgname || ! *cfgname )
        cfgname="zebra.cfg";
    /*sprintf(cfg, "%.200s%szebra.cfg", srcdir ? srcdir : "", srcdir ? "/" : "");     */

    sprintf(cfg, "%.200s/%s",srcdir, cfgname);
    return zebra_start(cfg);
}

/** 
 * makes a query, checks number of hits, and for the first hit, that 
 * it contains the given string, and that it gets the right score
 */
void RankingQuery(int lineno, ZebraHandle zh, char *query, 
          int exphits, char *firstrec, int firstscore )
{
    ZebraRetrievalRecord retrievalRecord[10];
    ODR odr_output = odr_createmem (ODR_DECODE);    
    ODR odr_input = odr_createmem (ODR_DECODE);    
    YAZ_PQF_Parser parser = yaz_pqf_create();
    Z_RPNQuery *rpn = yaz_pqf_parse(parser, odr_input, query);
    const char *setname="rsetname";
    int hits;
    int rc;
    int i;
        
    logf(LOG_LOG,"======================================");
    logf(LOG_LOG,"qry[%d]: %s", lineno, query);

    if (!rpn) {
        printf("Error: Parse failed \n%s\n",query);
        exit(1);
    }
    rc=zebra_search_RPN (zh, odr_input, rpn, setname, &hits);
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

    for (i = 0; i<10; i++)
    {
        retrievalRecord[i].position = i+1;
        retrievalRecord[i].score = i+20000;
    }

    rc=zebra_records_retrieve (zh, odr_output, setname, 0,
                     VAL_TEXT_XML, hits, retrievalRecord);

    if (rc) {
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
    odr_destroy (odr_input);
}

