/* $Id: t3.c,v 1.9 2004-08-25 09:23:39 adam Exp $
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

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>

/* read zebra.cfg from env var srcdir if it exists; otherwise current dir */
static ZebraService start_service()
{
    char cfg[256];
    char *srcdir = getenv("srcdir");
    sprintf(cfg, "%.200s%szebra.cfg", srcdir ? srcdir : "", srcdir ? "/" : "");
    return zebra_start(cfg);
}
	
int main(int argc, char **argv)
{
    int i;
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec =
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n";

    yaz_log_init_file("t3.log");

    nmem_init ();
    
    zs = start_service();
    zh = zebra_open (zs);
    zebra_select_database(zh, "Default");
    zebra_init(zh);

    zebra_begin_trans (zh, 1);
    zebra_add_record (zh, myrec, strlen(myrec));
    zebra_end_trans (zh);

    for (i = 0; i<4; i++)
    {
#if 0
        ZebraRetrievalRecord retrievalRecord;
#endif
        char setname[20];
        char *setnamep = setname;
        int status;
        ODR odr_input = odr_createmem (ODR_DECODE);    
        ODR odr_output = odr_createmem (ODR_ENCODE);    
        YAZ_PQF_Parser parser = yaz_pqf_create();
        Z_RPNQuery *query = yaz_pqf_parse(parser, odr_input, 
                                          "@attr 1=4 my");
        int hits;
        zebra_begin_trans (zh, 1);
        zebra_begin_trans (zh, 0);
        
        sprintf(setname, "s%d", i+1);
        zebra_search_RPN (zh, odr_input, query, setname, &hits);

        zebra_end_trans (zh);
        zebra_end_trans (zh);
        yaz_pqf_destroy(parser);
#if 0
        zebra_records_retrieve (zh, odr_output, setname, 0,
                                VAL_TEXT_XML, 1, &retrievalRecord);
#endif
#if 1
        zebra_deleleResultSet(zh, Z_DeleteRequest_list,
                              1, &setnamep, &status);
#endif
        odr_destroy (odr_input);
        odr_destroy (odr_output);
    }
    zebra_commit (zh);
    zebra_close (zh);
    zebra_stop (zs);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (0);
}
