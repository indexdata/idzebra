
#include <stdio.h>

#include <yaz/log.h>
#include <yaz/pquery.h>
#include "zebraapi.h"

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
    
    /* zh is our Zebra Handle - describes the server as a whole */
    ZebraHandle zh;
    
    /* the database we specify in our example */
    char *base = "Default";
    int argno;

    nmem_init ();

    log_init_file("apitest.log");

    odr_input = odr_createmem (ODR_DECODE);    
    odr_output = odr_createmem (ODR_ENCODE);    
    
   /* open Zebra */
    zh = zebra_open ("zebra.cfg");
    if (!zh)
    {
	printf ("Couldn't init zebra\n");
	exit (1);
    }

    /* This call controls the logging facility in YAZ/Zebra */
#if 0
    log_init(LOG_ALL, "", "out.log");
#endif

    /* Each argument to main will be a query */
    for (argno = 1; argno < argc; argno++)
    {
	/* parse the query and generate an RPN structure */
	Z_RPNQuery *query = p_query_rpn (odr_input, PROTO_Z3950, argv[argno]);
	char setname[64];
	int errCode;
	int i;
	const char *errString;
	char *errAdd;
	ZebraRetrievalRecord *records;
	int noOfRecordsToFetch;

	/* bad query? */
	if (!query)
	{
	    logf (LOG_WARN, "bad query %s\n", argv[argno]);
	    odr_reset (odr_input);
	    continue;
	}

	/* result set name will be called 1,2, etc */
	sprintf (setname, "%d", argno);

	/* fire up the search */
	zebra_search_rpn (zh, odr_input, odr_output, query, 1, &base, setname);
	
	/* status ... */
	errCode = zebra_errCode (zh);
	errString = zebra_errString (zh);
	errAdd = zebra_errAdd (zh);
	
	/* error? */
	if (errCode)
	{
	    printf ("Zebra Search Error %d %s %s\n",
		    errCode, errString, errAdd ? errAdd : "");
	    continue;
	}
	/* ok ... */
	printf ("Zebra Search gave %d hits\n", zebra_hits (zh));
	
	/* Deterimine number of records to fetch ... */
	if (zebra_hits(zh) > 10)
	    noOfRecordsToFetch = 10;
	else
	    noOfRecordsToFetch = zebra_hits(zh);

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
	errCode = zebra_errCode (zh);
	errString = zebra_errString (zh);
	errAdd = zebra_errAdd (zh);
	
	/* error ? */
	if (errCode)
	{
	    printf ("Zebra Search Error %d %s %s\n",
		    errCode, errString, errAdd ? errAdd : "");
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
    return 0;
}
