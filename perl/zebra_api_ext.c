#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <yaz/diagbib1.h>
#include "index.h"
#include <charmap.h>
#include <data1.h>
#include "zebra_perl.h"
#include "zebra_api_ext.h"
#include "yaz/log.h"
#include <yaz/pquery.h>
#include <yaz/cql.h>
#include <yaz/sortspec.h>

void data1_print_tree(data1_handle dh, data1_node *n, FILE *out) {
  data1_pr_tree(dh, n, stdout);
}

int zebra_get_shadow_enable (ZebraHandle zh) {
  return (zh->shadow_enable);
}

void zebra_set_shadow_enable (ZebraHandle zh, int value) {
  zh->shadow_enable = value;
}

void init_recordGroup (recordGroup *rg) {
    rg->groupName = NULL;
    rg->databaseName = NULL;
    rg->path = NULL;
    rg->recordId = NULL;
    rg->recordType = NULL;
    rg->flagStoreData = -1;
    rg->flagStoreKeys = -1; 
    rg->flagRw = 1;
    rg->databaseNamePath = 0;
    rg->explainDatabase = 0; 
    rg->fileVerboseLimit = 100000; 
    rg->followLinks = -1;
} 


/* This is from extract.c... it seems useful, when extract_rec_in mem is 
   called... and in general... Should be moved to somewhere else */
void res_get_recordGroup (ZebraHandle zh,
			  recordGroup *rGroup,
			  const char *ext) {
  char gprefix[128];
  char ext_res[128]; 
    
  if (!rGroup->groupName || !*rGroup->groupName)
    *gprefix = '\0';
  else 
    sprintf (gprefix, "%s.", rGroup->groupName);
  
  /* determine file type - depending on extension */
  if (!rGroup->recordType) {
    sprintf (ext_res, "%srecordType.%s", gprefix, ext);
    if (!(rGroup->recordType = res_get (zh->res, ext_res))) {
      sprintf (ext_res, "%srecordType", gprefix);
      rGroup->recordType = res_get (zh->res, ext_res);
    }
  }
  /* determine match criteria */
  if (!rGroup->recordId) { 
    sprintf (ext_res, "%srecordId.%s", gprefix, ext);
    if (!(rGroup->recordId = res_get (zh->res, ext_res))) {
      sprintf (ext_res, "%srecordId", gprefix);
      rGroup->recordId = res_get (zh->res, ext_res);
    }
  } 
  
  /* determine database name */
  if (!rGroup->databaseName) {
    sprintf (ext_res, "%sdatabase.%s", gprefix, ext);
    if (!(rGroup->databaseName = res_get (zh->res, ext_res))) { 
      sprintf (ext_res, "%sdatabase", gprefix);
      rGroup->databaseName = res_get (zh->res, ext_res);
    }
  }
  if (!rGroup->databaseName)
    rGroup->databaseName = "Default";

  /* determine if explain database */
  sprintf (ext_res, "%sexplainDatabase", gprefix);
  rGroup->explainDatabase =
    atoi (res_get_def (zh->res, ext_res, "0"));

  /* storeData */
  if (rGroup->flagStoreData == -1) {
    const char *sval;
    sprintf (ext_res, "%sstoreData.%s", gprefix, ext);
    if (!(sval = res_get (zh->res, ext_res))) {
      sprintf (ext_res, "%sstoreData", gprefix);
      sval = res_get (zh->res, ext_res);
    }
    if (sval)
      rGroup->flagStoreData = atoi (sval);
  }
  if (rGroup->flagStoreData == -1)  rGroup->flagStoreData = 0;

  /* storeKeys */
  if (rGroup->flagStoreKeys == -1)  {
    const char *sval;
    
    sprintf (ext_res, "%sstoreKeys.%s", gprefix, ext);
    sval = res_get (zh->res, ext_res);
    if (!sval) {
      sprintf (ext_res, "%sstoreKeys", gprefix);
      sval = res_get (zh->res, ext_res);
    }
    if (!sval)  sval = res_get (zh->res, "storeKeys");
    if (sval) rGroup->flagStoreKeys = atoi (sval);
  }
  if (rGroup->flagStoreKeys == -1) rGroup->flagStoreKeys = 0;
  
} 

int zebra_trans_processed(ZebraTransactionStatus s) {
  return (s.processed);
}

/* ---------------------------------------------------------------------------
  Record insert(=update), delete 

  If sysno is provided, then it's used to identify the reocord.
  If not, and match_criteria is provided, then sysno is guessed
  If not, and a record is provided, then sysno is got from there
*/

int zebra_update_record (ZebraHandle zh, 
			 struct recordGroup *rGroup,
			 const char *recordType,
			 int sysno, const char *match, const char *fname,
			 const char *buf, int buf_size)

{
    int res;

    if (buf_size < 1) buf_size = strlen(buf);

    zebra_begin_trans(zh);
    res=bufferExtractRecord (zh, buf, buf_size, rGroup, 
			     0, // delete_flag 
			     0, // test_mode,
			     recordType,
			     &sysno,   
			     match, fname);     
    zebra_end_trans(zh); 
    return sysno; 
}

int zebra_delete_record (ZebraHandle zh, 
			 struct recordGroup *rGroup, 
			 const char *recordType,
			 int sysno, const char *match, const char *fname,
			 const char *buf, int buf_size)
{
    int res;

    if (buf_size < 1) buf_size = strlen(buf);

    zebra_begin_trans(zh);
    res=bufferExtractRecord (zh, buf, buf_size, rGroup, 
			     1, // delete_flag
			     0, // test_mode, 
			     recordType,
			     &sysno,
			     match,fname);    
    zebra_end_trans(zh);
    return sysno;   
}

/* ---------------------------------------------------------------------------
  Searching 

  zebra_search_RPN is the same as zebra_search_rpn, except that read locking
  is not mandatory. (it's repeatable now, also in zebraapi.c)
*/

void zebra_search_RPN (ZebraHandle zh, ODR decode, ODR stream,
		       Z_RPNQuery *query, const char *setname, int *hits)
{
    zh->hits = 0;
    *hits = 0;

    if (zebra_begin_read (zh))
    	return;
    resultSetAddRPN (zh, decode, stream, query, 
                     zh->num_basenames, zh->basenames, setname);

    zebra_end_read (zh);

    *hits = zh->hits;
}

int zebra_search_PQF (ZebraHandle zh, 
		      ODR odr_input, ODR odr_output, 
		      const char *pqf_query,
		      const char *setname)

{
  int hits;
  Z_RPNQuery *query;
  query = p_query_rpn (odr_input, PROTO_Z3950, pqf_query);

  if (!query) {
    logf (LOG_WARN, "bad query %s\n", pqf_query);
    odr_reset (odr_input);
    return(0);
  }
  zebra_search_RPN (zh, odr_input, odr_output, query, setname, &hits);

  odr_reset (odr_input);
  odr_reset (odr_output);
  
  return(hits);
}

int zebra_cql2pqf (cql_transform_t ct, 
		   const char *query, char *res, int len) {
  
  int status;
  const char *addinfo;
  CQL_parser cp = cql_parser_create();

  if (status = cql_transform_error(ct, &addinfo)) {
    logf (LOG_WARN,"Transform error %d %s\n", status, addinfo ? addinfo : "");
    return (status);
  }

  if (status = cql_parser_string(cp, query))
    return (status);

  if (status = cql_transform_buf(ct, cql_parser_result(cp), res, len)) {
    logf (LOG_WARN,"Transform error %d %s\n", status, addinfo ? addinfo : "");
    return (status);
  }

  return (0);
}

void zebra_scan_PQF (ZebraHandle zh,
		     ScanObj *so,
		     ODR stream,
		     const char *pqf_query)
{
  Z_AttributesPlusTerm *zapt;
  Odr_oid *attrsetid;
  const char* oidname;
  oid_value attributeset;
  ZebraScanEntry *entries;
  int i, class;

  logf(LOG_DEBUG,  
       "scan req: pos:%d, num:%d, partial:%d", 
       so->position, so->num_entries, so->is_partial);

  zapt = p_query_scan (stream, PROTO_Z3950, &attrsetid, pqf_query);

  oidname = yaz_z3950oid_to_str (attrsetid, &class); 
  logf (LOG_DEBUG, "Attributreset: %s", oidname);
  attributeset = oid_getvalbyname(oidname);

  if (!zapt) {
    logf (LOG_WARN, "bad query %s\n", pqf_query);
    odr_reset (stream);
    return;
  }

  so->entries = (ScanEntry *)
    odr_malloc (stream, sizeof(so->entries) * (so->num_entries));


  zebra_scan (zh, stream, zapt, attributeset, 
	      &so->position, &so->num_entries, 
	      (ZebraScanEntry **) &so->entries, &so->is_partial);

  logf(LOG_DEBUG, 
       "scan res: pos:%d, num:%d, partial:%d", 
       so->position, so->num_entries, so->is_partial);
}

ScanEntry *getScanEntry(ScanObj *so, int pos) {
  return (&so->entries[pos-1]);
}

/* ---------------------------------------------------------------------------
  Record retrieval 
  2 phase retrieval - I didn't manage to return array of blessed references
  to wrapped structures... it's feasible, but I'll need some time 
  / pop - 2002-11-17
*/

void record_retrieve(RetrievalObj *ro,
		     ODR stream,
		     RetrievalRecord *res,
		     int pos) 
{
  int i = pos - 1;


  RetrievalRecordBuf *buf = 
    (RetrievalRecordBuf *) odr_malloc(stream, sizeof(*buf));  

  res->errCode    = ro->records[i].errCode;
  res->errString  = ro->records[i].errString;
  res->position   = ro->records[i].position;
  res->base       = ro->records[i].base;
  res->format     = ro->records[i].format;
  res->buf        = buf;
  res->buf->len   = ro->records[i].len;
  res->buf->buf   = ro->records[i].buf;
  res->score      = ro->records[i].score;
  res->sysno      = ro->records[i].sysno;

}

/* most of the code here was copied from yaz-client */
void records_retrieve(ZebraHandle zh,
		      ODR stream,
		      const char *setname,
		      const char *a_eset, 
		      const char *a_schema,
		      const char *a_format,
		      int from,
		      int to,
		      RetrievalObj *res) 
{
  static enum oid_value recordsyntax = VAL_SUTRS;
  static enum oid_value schema = VAL_NONE;
  static Z_ElementSetNames *elementSetNames = 0; 
  static Z_RecordComposition compo;
  static Z_ElementSetNames esn;
  static char what[100];
  int i;
  int oid[OID_SIZE];

  compo.which = -1;

  if (from < 1) from = 1;
  if (from > to) to = from;
  res->noOfRecords = to - from + 1;

  res->records = odr_malloc (stream, 
			     sizeof(*res->records) * (res->noOfRecords));  

  for (i = 0; i<res->noOfRecords; i++) res->records[i].position = from+i;

  if (!a_eset || !*a_eset) {
    elementSetNames = 0;
  } else {
    strcpy(what, a_eset);
    esn.which = Z_ElementSetNames_generic;
    esn.u.generic = what;
    elementSetNames = &esn;
  }

  if (!a_schema || !*a_schema) {
    schema = VAL_NONE;
  } else {
    schema = oid_getvalbyname (a_schema);
    if (schema == VAL_NONE) {
      logf(LOG_WARN,"unknown schema '%s'",a_schema);
    }
  }

  
  if (!a_format || !*a_format) {
    recordsyntax = VAL_SUTRS;
  } else {
    recordsyntax = oid_getvalbyname (a_format);
    if (recordsyntax == VAL_NONE) {
      logf(LOG_WARN,"unknown record syntax '%s', using SUTRS",a_schema);
      recordsyntax = VAL_SUTRS;
    }
  }

  if (schema != VAL_NONE) {
    oident prefschema;

    prefschema.proto = PROTO_Z3950;
    prefschema.oclass = CLASS_SCHEMA;
    prefschema.value = schema;
    
    compo.which = Z_RecordComp_complex;
    compo.u.complex = (Z_CompSpec *)
      odr_malloc(stream, sizeof(*compo.u.complex));
    compo.u.complex->selectAlternativeSyntax = (bool_t *) 
      odr_malloc(stream, sizeof(bool_t));
    *compo.u.complex->selectAlternativeSyntax = 0;
    
    compo.u.complex->generic = (Z_Specification *)
      odr_malloc(stream, sizeof(*compo.u.complex->generic));
    compo.u.complex->generic->which = Z_Schema_oid;
    compo.u.complex->generic->schema.oid = (Odr_oid *)
      odr_oiddup(stream, oid_ent_to_oid(&prefschema, oid));
    if (!compo.u.complex->generic->schema.oid)
      {
	/* OID wasn't a schema! Try record syntax instead. */
	prefschema.oclass = CLASS_RECSYN;
	compo.u.complex->generic->schema.oid = (Odr_oid *)
	  odr_oiddup(stream, oid_ent_to_oid(&prefschema, oid));
      }
    if (!elementSetNames)
      compo.u.complex->generic->elementSpec = 0;
    else
      {
	compo.u.complex->generic->elementSpec = (Z_ElementSpec *)
	  odr_malloc(stream, sizeof(Z_ElementSpec));
	compo.u.complex->generic->elementSpec->which =
	  Z_ElementSpec_elementSetName;
	compo.u.complex->generic->elementSpec->u.elementSetName =
	  elementSetNames->u.generic;
      }
    compo.u.complex->num_dbSpecific = 0;
    compo.u.complex->dbSpecific = 0;
    compo.u.complex->num_recordSyntax = 0;
    compo.u.complex->recordSyntax = 0;
  } 
  else if (elementSetNames) {
    compo.which = Z_RecordComp_simple;
    compo.u.simple = elementSetNames;
  }

  if (compo.which == -1) {
    api_records_retrieve (zh, stream, setname, 
			    NULL, 
			    recordsyntax,
			    res->noOfRecords, res->records);
  } else {
    api_records_retrieve (zh, stream, setname, 
			    &compo,
			    recordsyntax,
			    res->noOfRecords, res->records);
  }

}
 
int zebra_trans_no (ZebraHandle zh) {
  return (zh->trans_no);
}

/* almost the same as zebra_records_retrieve ... but how did it work? 
   I mean for multiple records ??? CHECK ??? */
void api_records_retrieve (ZebraHandle zh, ODR stream,
			   const char *setname, Z_RecordComposition *comp,
			   oid_value input_format, int num_recs,
			   ZebraRetrievalRecord *recs)
{
    ZebraPosSet poset;
    int i, *pos_array;

    if (!zh->res)
    {
        zh->errCode = 30;
        zh->errString = odr_strdup (stream, setname);
        return;
    }
    
    zh->errCode = 0; 

    if (zebra_begin_read (zh))
	return;

    pos_array = (int *) xmalloc (num_recs * sizeof(*pos_array));
    for (i = 0; i<num_recs; i++)
	pos_array[i] = recs[i].position;
    poset = zebraPosSetCreate (zh, setname, num_recs, pos_array);
    if (!poset)
    {
        logf (LOG_DEBUG, "zebraPosSetCreate error");
        zh->errCode = 30;
        zh->errString = nmem_strdup (stream->mem, setname);
    }
    else
    {
	for (i = 0; i<num_recs; i++)
	{
	    if (poset[i].term)
	    {
		recs[i].errCode = 0;
		recs[i].format = VAL_SUTRS;
		recs[i].len = strlen(poset[i].term);
		recs[i].buf = poset[i].term;
		recs[i].base = poset[i].db;
		recs[i].sysno = 0;
	    
	    }
	    else if (poset[i].sysno)
	    {
	      /* changed here ??? CHECK ??? */
	      char *b;
		recs[i].errCode =
		    zebra_record_fetch (zh, poset[i].sysno, poset[i].score,
					stream, input_format, comp,
					&recs[i].format, 
					&b,
					&recs[i].len,
					&recs[i].base);
		recs[i].buf = (char *) odr_malloc(stream,recs[i].len);
		memcpy(recs[i].buf, b, recs[i].len);
		recs[i].errString = NULL;
		recs[i].sysno = poset[i].sysno;
		recs[i].score = poset[i].score;
	    }
	    else
	    {
	        char num_str[20];

		sprintf (num_str, "%d", pos_array[i]);	
		zh->errCode = 13;
                zh->errString = odr_strdup (stream, num_str);
                break;
	    }

	}
	zebraPosSetDestroy (zh, poset, num_recs);
    }
    zebra_end_read (zh);
    xfree (pos_array);
}


/* ---------------------------------------------------------------------------
  Sort - a simplified interface, with optional read locks.
*/
int sort (ZebraHandle zh, 
	  ODR stream,
	  const char *sort_spec,
	  const char *output_setname,
	  const char **input_setnames
	  ) 
{
  int num_input_setnames = 0;
  int sort_status = 0;
  Z_SortKeySpecList *sort_sequence = yaz_sort_spec (stream, sort_spec);

  /* we can do this, since the typemap code for char** will 
     put a NULL at the end of list */
    while (input_setnames[num_input_setnames]) num_input_setnames++;

    if (zebra_begin_read (zh))
	return;

    resultSetSort (zh, stream->mem, num_input_setnames, input_setnames,
		   output_setname, sort_sequence, &sort_status);

    zebra_end_read(zh);
    return (sort_status);
}
