/* $Id: apitest.c,v 1.25 2006-05-10 08:13:20 adam Exp $
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

#include <stdio.h>
#include <stdlib.h>

#include <yaz/log.h>
#include <yaz/pquery.h>
#include <idzebra/api.h>

/* Small routine to display GRS-1 record variants ... */
/* Copied verbatim from yaz/client/client.c */
static void display_variant(Z_Variant *v, int level)
{
    int i;

    for (i = 0; i < v->num_triples; i++)
    {
	printf("%*sclass=%d,type=%d", level * 4, "", *v->triples[i]->zclass,
	    *v->triples[i]->type);
	if (v->triples[i]->which == Z_Triple_internationalString)
	    printf(",value=%s\n", v->triples[i]->value.internationalString);
	else
	    printf("\n");
    }
}
 
/* Small routine to display a GRS-1 record ... */
/* Copied verbatim from yaz/client/client.c */
static void display_grs1(Z_GenericRecord *r, int level)
{
    int i;

    if (!r)
        return;
    for (i = 0; i < r->num_elements; i++)
    {
        Z_TaggedElement *t;

        printf("%*s", level * 4, "");
        t = r->elements[i];
        printf("(");
        if (t->tagType)
            printf("%d,", *t->tagType);
        else
            printf("?,");
        if (t->tagValue->which == Z_StringOrNumeric_numeric)
            printf("%d) ", *t->tagValue->u.numeric);
        else
            printf("%s) ", t->tagValue->u.string);
        if (t->content->which == Z_ElementData_subtree)
        {
            printf("\n");
            display_grs1(t->content->u.subtree, level+1);
        }
        else if (t->content->which == Z_ElementData_string)
            printf("%s\n", t->content->u.string);
        else if (t->content->which == Z_ElementData_numeric)
	    printf("%d\n", *t->content->u.numeric);
	else if (t->content->which == Z_ElementData_oid)
	{
	    int *ip = t->content->u.oid;
	    oident *oent;

	    if ((oent = oid_getentbyoid(t->content->u.oid)))
		printf("OID: %s\n", oent->desc);
	    else
	    {
		printf("{");
		while (ip && *ip >= 0)
		    printf(" %d", *(ip++));
		printf(" }\n");
	    }
	}
	else if (t->content->which == Z_ElementData_noDataRequested)
	    printf("[No data requested]\n");
	else if (t->content->which == Z_ElementData_elementEmpty)
	    printf("[Element empty]\n");
	else if (t->content->which == Z_ElementData_elementNotThere)
	    printf("[Element not there]\n");
	else
            printf("??????\n");
	if (t->appliedVariant)
	    display_variant(t->appliedVariant, level+1);
	if (t->metaData && t->metaData->supportedVariants)
	{
	    int c;

	    printf("%*s---- variant list\n", (level+1)*4, "");
	    for (c = 0; c < t->metaData->num_supportedVariants; c++)
	    {
		printf("%*svariant #%d\n", (level+1)*4, "", c);
		display_variant(t->metaData->supportedVariants[c], level + 2);
	    }
	}
    }
}

/* Small test main to illustrate the use of the C api */
int main (int argc, char **argv)
{
    /* odr is a handle to memory assocated with RETURNED data from
       various functions */
    ODR odr_input, odr_output;
    
    /* zs is our Zebra Service - decribes whole server */
    ZebraService zs;

    /* zh is our Zebra Handle - describes database session */
    ZebraHandle zh;
    
    /* the database we specify in our example */
    const char *base = "Default";
    int argno;

    nmem_init ();

    yaz_log_init_file("apitest.log");

    odr_input = odr_createmem (ODR_DECODE);    
    odr_output = odr_createmem (ODR_ENCODE);    
    
    zs = zebra_start ("zebra.cfg");
    if (!zs)
    {
	printf ("zebra_start failed; missing zebra.cfg?\n");
	exit (1);
    }
    /* open Zebra */
    zh = zebra_open (zs, 0);
    if (!zh)
    {
	printf ("zebras_open failed\n");
	exit (1);
    }
    if (zebra_select_databases (zh, 1, &base) != ZEBRA_OK)
    {
	printf ("zebra_select_databases failed\n");
	exit (1);
    }
    /* Each argument to main will be a query */
    for (argno = 1; argno < argc; argno++)
    {
	/* parse the query and generate an RPN structure */
	Z_RPNQuery *query = p_query_rpn (odr_input, PROTO_Z3950, argv[argno]);
	char setname[64];
	int errCode;
	int i;
        zint hits;
	char *errString;
	ZebraRetrievalRecord *records;
	int noOfRecordsToFetch;

	/* bad query? */
	if (!query)
	{
	    yaz_log (YLOG_WARN, "bad query %s\n", argv[argno]);
	    odr_reset (odr_input);
	    continue;
	}
	else
	{
	    char out_str[100];
	    int r;
#if 1
	    r = zebra_string_norm (zh, 'w',
				   argv[argno], strlen(argv[argno]),
				   out_str, sizeof(out_str));
	    if (r >= 0)
	    {
		printf ("norm: '%s'\n", out_str);
	    }
	    else
	    {
		printf ("norm fail: %d\n", r);
	    }
#endif

	}
	/* result set name will be called 1,2, etc */
	sprintf (setname, "%d", argno);

	/* fire up the search */
	zebra_search_RPN (zh, odr_input, query, setname, &hits);
	
	/* status ... */
        zebra_result (zh, &errCode, &errString);
	
	/* error? */
	if (errCode)
	{
	    printf ("Zebra Search Error %d %s\n",
		    errCode, errString);
	    continue;
	}
	/* ok ... */
	printf ("Zebra Search gave " ZINT_FORMAT " hits\n", hits);
	
	/* Deterimine number of records to fetch ... */
	if (hits > 10)
	    noOfRecordsToFetch = 10;
	else
	    noOfRecordsToFetch = hits;

	/* reset our memory - we've finished dealing with search */
	odr_reset (odr_input);
 	odr_reset (odr_output);

	/* prepare to fetch ... */
	records = odr_malloc (odr_input, sizeof(*records) * noOfRecordsToFetch);
	/* specify position of each record to fetch */
	/* first one is numbered 1 and NOT 0 */
	for (i = 0; i<noOfRecordsToFetch; i++)
	    records[i].position = i+1;
	/* fetch them and request for GRS-1 records */
	zebra_records_retrieve (zh, odr_input, setname, NULL, VAL_SUTRS,
				noOfRecordsToFetch, records);

	/* status ... */

        zebra_result (zh, &errCode, &errString);

	/* error ? */
	if (errCode)
	{
	    printf ("Zebra Search Error %d %s\n",
		    errCode, errString);
	}
	else
	{
	    /* inspect each record in result */
	    for (i = 0; i<noOfRecordsToFetch; i++)
	    {
		printf ("Record %d\n", i+1);
		/* error when fetching this record? */
		if (records[i].errCode)
		{
		    printf ("  Error %d\n", records[i].errCode);
		    continue;
		}
		/* GRS-1 record ? */
		if (records[i].format == VAL_GRS1)
		{
		    Z_GenericRecord *grs_record =
			(Z_GenericRecord *) records[i].buf;
		    printf ("  GRS-1\n");
		    display_grs1(grs_record, 0);
		}
                else if (records[i].format == VAL_SUTRS)
                {
                    printf ("  SUTRS\n");
                    printf ("%.*s", records[i].len, records[i].buf);
                }
		/* some other record we don't handle yet... */
		else
		{
		    printf ("  Other record (ignored)\n");
		}
	    }
	}
	/* reset our memory - we've finished dealing with present */
	odr_reset (odr_input); 
	odr_reset (odr_output);
    }
    odr_destroy (odr_input);
    odr_destroy (odr_output);
    zebra_close (zh);
    zebra_stop (zs);
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

