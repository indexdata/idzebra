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
#include "rg.h"
#include "yaz/log.h"
#include <yaz/pquery.h>

void data1_print_tree(data1_handle dh, data1_node *n, FILE *out) {
  data1_pr_tree(dh, n, stdout);
}

int zebra_get_shadow_enable (ZebraHandle zh) {
  return (zh->shadow_enable);
}

void zebra_set_shadow_enable (ZebraHandle zh, int value) {
  zh->shadow_enable = value;
}
/* recordGroup stuff */
void describe_recordGroup (recordGroup *rg) {
  printf ("name:%s\ndatabaseName%s\npath:%s\n",
	  rg->groupName,
	  rg->databaseName,
	  rg->path);
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

/* ---------------------------------------------------------------------------
  Record insert(=update), delete 

  If sysno is provided, then it's used to identify the reocord.
  If not, and match_criteria is provided, then sysno is guessed
  If not, and a record is provided, then sysno is got from there

*/

int zebra_update_record (ZebraHandle zh, 
			 struct recordGroup *rGroup, 
			 int sysno, const char *match, const char *fname,
			 const char *buf, int buf_size)

{
    int res;

    if (buf_size < 1) buf_size = strlen(buf);

    res=bufferExtractRecord (zh, buf, buf_size, rGroup, 
			     0, // delete_flag
			     0, // test_mode, 
			     &sysno,
			     match, fname);    
  
    return sysno;
}

int zebra_delete_record (ZebraHandle zh, 
			 struct recordGroup *rGroup, 
			 int sysno, const char *match, const char *fname,
			 const char *buf, int buf_size)
{
    int res;

    if (buf_size < 1) buf_size = strlen(buf);

    res=bufferExtractRecord (zh, buf, buf_size, rGroup, 
			     1, // delete_flag
			     0, // test_mode, 
			     &sysno,
			     match,fname);    
    return sysno;
}
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

void zebra_retrieve_sysnos (ZebraHandle zh,
			    const char *setname,
			    int num_recs)
{			    
  if (!zh->res) {
    zh->errCode = 30;
    /* zh->errString = odr_strdup (stream, setname); */
    logf(LOG_FATAL,"No resources open");
    return;
  }

  zh->errCode = 0;
    if (zebra_begin_read (zh))
    	return;

    zebra_end_read (zh);

}

int zebra_search_PQF (ZebraHandle zh, 
		      ODR odr_input, ODR odr_output, 
		      const char *pqf_query,
		      const char *setname)

{
  int hits;
  Z_RPNQuery *query;
  logf (LOG_LOG, "11");
  query = p_query_rpn (odr_input, PROTO_Z3950, pqf_query);
  logf (LOG_LOG, "22");

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
