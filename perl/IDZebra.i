%module "IDZebra"

%{
#include "zebraapi.h"
#include "zebra_api_ext.h"
#include "zebra_perl.h"
#include "data1.h"
#include "yaz/odr.h"
%}

/* == Typemaps ============================================================= */

/* RetrievalRecordBuff is a special construct, to allow to map a char * buf
   to non-null terminated perl string scalar value (SVpv). */
%typemap(out) RetrievalRecordBuf * {
  $result = newSVpv($1->buf,$1->len);
  sv_2mortal($result);
  argvi++;
}

/* All char ** values are mapped in-out to array of strings. */
%typemap(in) char ** {
	AV *tempav;
	I32 len;
	int i;
	SV  **tv;
	STRLEN na;
	if (!SvROK($input))
	    croak("Argument $argnum is not a reference.");
        if (SvTYPE(SvRV($input)) != SVt_PVAV)
	    croak("Argument $argnum is not an array.");
        tempav = (AV*)SvRV($input);
	len = av_len(tempav);
	$1 = (char **) malloc((len+2)*sizeof(char *));
	for (i = 0; i <= len; i++) {
	    tv = av_fetch(tempav, i, 0);	
	    $1[i] = (char *) SvPV(*tv,na);
        }
	$1[i] = NULL;
};

/* This cleans up the char ** array after the function call */
%typemap(freearg) char ** {
	free($1);
}

/* Creates a new Perl array and places a NULL-terminated char ** into it */
%typemap(out) char ** {
	AV *myav;
	SV **svs;
	int i = 0,len = 0;
	/* Figure out how many elements we have */
	while ($1[len])
	   len++;
	svs = (SV **) malloc(len*sizeof(SV *));
	for (i = 0; i < len ; i++) {
	    svs[i] = sv_newmortal();
	    sv_setpv((SV*)svs[i],$1[i]);
	};
	myav =	av_make(len,svs);
	free(svs);
        $result = newRV((SV*)myav);
        sv_2mortal($result);
        argvi++;
}


/* == Structures for shadow classes  ======================================= */

%include "zebra_perl.h"


/* == Module initialization and cleanup (zebra_perl.c) ===================== */

void init (void);
void DESTROY (void);


/* == Logging facilities (yaz/log.h) ======================================= */

void logLevel (int level);
void logFile (const char *fname);
void logMsg  (int level, const char *message);

#define LOG_FATAL  0x0001
#define LOG_DEBUG  0x0002
#define LOG_WARN   0x0004
#define LOG_LOG    0x0008
#define LOG_ERRNO  0x0010     /* append strerror to message */
#define LOG_FILE   0x0020
#define LOG_APP    0x0040     /* For application level events */
#define LOG_MALLOC 0x0080     /* debugging mallocs */
#define LOG_ALL    0xff7f
#define LOG_DEFAULT_LEVEL (LOG_FATAL | LOG_ERRNO | LOG_LOG | LOG_WARN)

/* == ODR stuff (yaz/odr.h) ================================================ */

#define ODR_DECODE      0
#define ODR_ENCODE      1
#define ODR_PRINT       2
ODR odr_createmem(int direction);
void odr_reset(ODR o);
void odr_destroy(ODR o);
void *odr_malloc(ODR o, int size);


/* == Zebra session and service (index/zebraapi.c) ========================= */

%name(start)     
ZebraService zebra_start (const char *configName);

%name(open)      
ZebraHandle zebra_open (ZebraService zs);

%name(close)     
void zebra_close (ZebraHandle zh);

%name(stop)      
void zebra_stop (ZebraService zs);


/* == Error handling and reporting (index/zebraapi.c) ====================== */

/* last error code */
%name(errCode)   
int zebra_errCode (ZebraHandle zh); 

/* string representatio of above */
%name(errString) 
const char * zebra_errString (ZebraHandle zh); 

/* extra information associated with error */
%name(errAdd)    
char *  zebra_errAdd (ZebraHandle zh); 


/* == Record groups and database selection ================================= */

/* initialize a recordGroup (zebra_api_ext.c); */
void init_recordGroup (recordGroup *rg);

/* set up a recordGroup for a specific file extension from zebra.cfg 
   (zebra_api_ext.c); */
void res_get_recordGroup (ZebraHandle zh, recordGroup *rg, 
			  const char *ext); 
/* set current record group for update purposes (zebraapi.c) */
%name(set_group)           
void zebra_set_group (ZebraHandle zh, struct recordGroup *rg);

/* select database for update purposes (zebraapi.c) */
%name(select_database)     
int zebra_select_database (ZebraHandle zh, const char *basename);

/* select databases for record retrieval (zebraapi.c) */
%name(select_databases)    
int zebra_select_databases (ZebraHandle zh, int num_bases, 
			     const char **basenames);


/* == Transactions, locking, shadow register =============================== */

/* begin transaction (add write lock) (zebraapi.c) */
%name(begin_trans)         
void zebra_begin_trans (ZebraHandle zh);

/* end transaction (remove write lock) (zebraapi.c) */
%name(end_trans)           
void zebra_end_trans (ZebraHandle zh); 

/* begin retrieval (add read lock) (zebraapi.c) */
%name(begin_read)          
int zebra_begin_read (ZebraHandle zh);

/* end retrieval (remove read lock) (zebraapi.c) */
%name(end_read)            
void zebra_end_read (ZebraHandle zh);

/* commit changes from shadow (zebraapi.c) */
%name(commit)              
int  zebra_commit (ZebraHandle zh);

/* get shadow status (zebra_api_ext.c) */
%name(get_shadow_enable)   
int  zebra_get_shadow_enable (ZebraHandle zh);

/* set shadow status (zebra_api_ext.c) */
%name(set_shadow_enable)   
void zebra_set_shadow_enable (ZebraHandle zh, int value);


/* == Repository actions (zebraapi.c) ====================================== */

%name(init)                
int  zebra_init (ZebraHandle zh);

%name(compact)             
int  zebra_compact (ZebraHandle zh);

%name(repository_update)   
void zebra_repository_update (ZebraHandle zh);

%name(repository_delete)   
void zebra_repository_delete (ZebraHandle zh);

%name(repository_show)     
void zebra_repository_show (ZebraHandle zh); 


/* == Record update/delete (zebra_api_ext.c) =============================== */

/* If sysno is provided, then it's used to identify the reocord.
   If not, and match_criteria is provided, then sysno is guessed
   If not, and a record is provided, then sysno is got from there */

%name(update_record)       
int zebra_update_record (ZebraHandle zh, 
			 recordGroup *rGroup, 
			 int sysno, 
			 const char *match, 
			 const char *fname,
			 const char *buf, 
			 int buf_size);
     
%name(delete_record)       
int zebra_delete_record (ZebraHandle zh, 
			 recordGroup *rGroup, 
			 int sysno, 
			 const char *match, 
			 const char *fname,
			 const char *buf, 
			 int buf_size);

/* == Search (zebra_api_ext.c) ============================================= */

%name(search_PQF) 
int zebra_search_PQF (ZebraHandle zh, 
		      ODR odr_input, ODR odr_output, 
		      const char *pqf_query,
		      const char *setname);


/* TODO: search_CCL */


/* == Retrieval (zebra_api_ext.c) ========================================== */

/* will get a 'retrieval obj' (simple enough to pass to perl), which can be 
   used to get the individual records. Elementset, schema and format strings
   are threated the same way yaz-client does. */
void records_retrieve(ZebraHandle zh,
		      ODR stream,
		      const char *setname,      // resultset name
		      const char *a_eset,       // optional elementset
		      const char *a_schema,     // optional schema
		      const char *a_format,     // optional record syntax
		      int from,                 // range, 1 based
		      int to,
		      RetrievalObj *res
		      );

/* fetch a record from the retrieval object. pos is 1 based */
void record_retrieve(RetrievalObj *ro,
		     ODR stream,
		     RetrievalRecord *res,
		     int pos);

/* == Sort ================================================================= */
int sort (ZebraHandle zh, 
	  ODR stream,
	  const char *sort_spec,
	  const char *output_setname,
	  const char **input_setnames
	  ); 
/*

void zebra_sort (ZebraHandle zh, ODR stream,
		 int num_input_setnames,
		 const char **input_setnames,
		 const char *output_setname,
		 Z_SortKeySpecList *sort_sequence,
		 int *sort_status);
*/

/* Admin functionality */
/*
%name(admin_start)         void zebra_admin_start (ZebraHandle zh);
%name(admin_shutdown)      void zebra_admin_shutdown (ZebraHandle zh);
*/

/* Browse 
void zebra_scan (ZebraHandle zh, ODR stream,
		 Z_AttributesPlusTerm *zapt,
		 oid_value attributeset,
		 int *position, int *num_entries,
		 ZebraScanEntry **list,
		 int *is_partial);
*/

/* Delete Result Set(s) */
/*
int zebra_deleleResultSet(ZebraHandle zh, int function,
			  int num_setnames, char **setnames,
			  int *statuses);
*/

/* do authentication */
/*
int zebra_auth (ZebraHandle zh, const char *user, const char *pass);

*/


/*

void zebra_result (ZebraHandle zh, int *code, char **addinfo);
int zebra_resultSetTerms (ZebraHandle zh, const char *setname, 
			  int no, int *count, 
			  int *type, char *out, size_t *len);
*/

/*
YAZ_EXPORT void zebra_admin_create (ZebraHandle zh, const char *db);

YAZ_EXPORT void zebra_admin_import_begin (ZebraHandle zh, const char *database);

YAZ_EXPORT void zebra_admin_import_segment (ZebraHandle zh,
					    Z_Segment *segment);

void zebra_admin_shutdown (ZebraHandle zh);
void zebra_admin_start (ZebraHandle zh);
void zebra_admin_import_end (ZebraHandle zh);


*/


/* =========================================================================
 * NMEM stuff
 * ========================================================================= 
*/

NMEM         nmem_create (void);
void         nmem_destroy (NMEM handle);

/* =========================================================================
 * Data1 stuff
 * ========================================================================= 
*/

typedef enum data1_datatype
{
    DATA1K_unknown,
    DATA1K_structured,
    DATA1K_string,
    DATA1K_numeric,
    DATA1K_bool,
    DATA1K_oid,
    DATA1K_generalizedtime,
    DATA1K_intunit,
    DATA1K_int,
    DATA1K_octetstring,
    DATA1K_null
} data1_datatype;

#define DATA1T_numeric 1
#define DATA1T_string 2
#define DATA1N_root 1 
#define DATA1N_tag  2       
#define DATA1N_data 3
#define DATA1N_variant 4
#define DATA1N_comment 5
#define DATA1N_preprocess 6
#define DATA1I_inctxt 1
#define DATA1I_incbin 2
#define DATA1I_text 3 
#define DATA1I_num 4
#define DATA1I_oid 5         
#define DATA1_LOCALDATA 12
#define DATA1_FLAG_XML  1

data1_handle data1_create (void);
data1_handle data1_createx (int flags);
void data1_destroy(data1_handle dh);

/* Data1 node */
data1_node *get_parent_tag(data1_handle dh, data1_node *n);
data1_node *data1_read_node(data1_handle dh, const char **buf,NMEM m);
data1_node *data1_read_nodex (data1_handle dh, NMEM m, int (*get_byte)(void *fh), void *fh, WRBUF wrbuf);
data1_node *data1_read_record(data1_handle dh, int (*rf)(void *, char *, size_t), void *fh, NMEM m);
data1_absyn *data1_read_absyn(data1_handle dh, const char *file, int file_must_exist);
data1_tag *data1_gettagbynum(data1_handle dh, data1_tagset *s, int type, int value);

data1_tagset *data1_empty_tagset (data1_handle dh);
data1_tagset *data1_read_tagset(data1_handle dh, const char *file, int type);
data1_element *data1_getelementbytagname(data1_handle dh,
					 data1_absyn *abs,
					 data1_element *parent,
					 const char *tagname);

Z_GenericRecord *data1_nodetogr(data1_handle dh, data1_node *n,
				int select, ODR o,
				int *len);

data1_tag *data1_gettagbyname(data1_handle dh, data1_tagset *s,
			      const char *name);

void data1_free_tree(data1_handle dh, data1_node *t);

char *data1_nodetobuf(data1_handle dh, data1_node *n,
		      int select, int *len);

data1_node *data1_mk_tag_data_wd(data1_handle dh,
				 data1_node *at,
				 const char *tagname, NMEM m);
data1_node *data1_mk_tag_data(data1_handle dh, data1_node *at,
			      const char *tagname, NMEM m);
data1_datatype data1_maptype(data1_handle dh, char *t);
data1_varset *data1_read_varset(data1_handle dh, const char *file);
data1_vartype *data1_getvartypebyct(data1_handle dh,
				    data1_varset *set,
				    char *zclass, char *type);
Z_Espec1 *data1_read_espec1(data1_handle dh, const char *file);
int data1_doespec1(data1_handle dh, data1_node *n, Z_Espec1 *e);

data1_esetname *data1_getesetbyname(data1_handle dh, 
				    data1_absyn *a,
				    const char *name);
data1_element *data1_getelementbyname(data1_handle dh,
						 data1_absyn *absyn,
						 const char *name);
data1_node *data1_mk_node2(data1_handle dh, NMEM m,
                                      int type, data1_node *parent);

data1_node *data1_mk_tag (data1_handle dh, NMEM nmem, 
                                     const char *tag, const char **attr,
                                     data1_node *at);
data1_node *data1_mk_tag_n (data1_handle dh, NMEM nmem,
                                       const char *tag, size_t len,
                                       const char **attr,
                                       data1_node *at);
void data1_tag_add_attr (data1_handle dh, NMEM nmem,
                                    data1_node *res, const char **attr);

data1_node *data1_mk_text_n (data1_handle dh, NMEM mem,
                                        const char *buf, size_t len,
                                        data1_node *parent);
data1_node *data1_mk_text_nf (data1_handle dh, NMEM mem,
                                         const char *buf, size_t len,
                                         data1_node *parent);
data1_node *data1_mk_text (data1_handle dh, NMEM mem,
                                      const char *buf, data1_node *parent);

data1_node *data1_mk_comment_n (data1_handle dh, NMEM mem,
                                           const char *buf, size_t len,
                                           data1_node *parent);

data1_node *data1_mk_comment (data1_handle dh, NMEM mem,
                                         const char *buf, data1_node *parent);

data1_node *data1_mk_preprocess (data1_handle dh, NMEM nmem,
                                            const char *target,
                                            const char **attr,
                                            data1_node *at);

data1_node *data1_mk_root (data1_handle dh, NMEM nmem,
                                      const char *name);
void data1_set_root(data1_handle dh, data1_node *res,
                               NMEM nmem, const char *name);

data1_node *data1_mk_tag_data_int (data1_handle dh, data1_node *at,
                                              const char *tag, int num,
                                              NMEM nmem);
data1_node *data1_mk_tag_data_oid (data1_handle dh, data1_node *at,
                                              const char *tag, Odr_oid *oid,
                                              NMEM nmem);
data1_node *data1_mk_tag_data_text (data1_handle dh, data1_node *at,
                                               const char *tag,
                                               const char *str,
                                               NMEM nmem);
data1_node *data1_mk_tag_data_text_uni (data1_handle dh,
                                                   data1_node *at,
                                                   const char *tag,
                                                   const char *str,
                                                   NMEM nmem);

data1_absyn *data1_get_absyn (data1_handle dh, const char *name);

data1_node *data1_search_tag (data1_handle dh, data1_node *n,
                                         const char *tag);
data1_node *data1_mk_tag_uni (data1_handle dh, NMEM nmem, 
                                         const char *tag, data1_node *at);
data1_attset *data1_get_attset (data1_handle dh, const char *name);
data1_maptab *data1_read_maptab(data1_handle dh, const char *file);
data1_node *data1_map_record(data1_handle dh, data1_node *n,
					data1_maptab *map, NMEM m);
data1_marctab *data1_read_marctab (data1_handle dh,
					      const char *file);
char *data1_nodetomarc(data1_handle dh, data1_marctab *p,
				  data1_node *n, int selected, int *len);
char *data1_nodetoidsgml(data1_handle dh, data1_node *n,
				    int select, int *len);
Z_ExplainRecord *data1_nodetoexplain(data1_handle dh,
						data1_node *n, int select,
						ODR o);
Z_BriefBib *data1_nodetosummary(data1_handle dh, 
					   data1_node *n, int select,
					   ODR o);
char *data1_nodetosoif(data1_handle dh, data1_node *n, int select,
				  int *len);
WRBUF data1_get_wrbuf (data1_handle dp);
char **data1_get_read_buf (data1_handle dp, int **lenp);
char **data1_get_map_buf (data1_handle dp, int **lenp);
data1_absyn_cache *data1_absyn_cache_get (data1_handle dh);
data1_attset_cache *data1_attset_cache_get (data1_handle dh);
NMEM data1_nmem_get (data1_handle dh);

void data1_pr_tree (data1_handle dh, data1_node *n, FILE *out);
void data1_print_tree (data1_handle dh, data1_node *n);


char *data1_insert_string (data1_handle dh, data1_node *res,
				      NMEM m, const char *str);
char *data1_insert_string_n (data1_handle dh, data1_node *res,
                                        NMEM m, const char *str, size_t len);
data1_node *data1_read_sgml (data1_handle dh, NMEM m,
					const char *buf);
/*
data1_node *data1_read_xml (data1_handle dh,
                                       int (*rf)(void *, char *, size_t),
                                       void *fh, NMEM m);
*/
void data1_absyn_trav (data1_handle dh, void *handle,
				  void (*fh)(data1_handle dh,
					     void *h, data1_absyn *a));

data1_attset *data1_attset_search_id (data1_handle dh, int id);

char *data1_getNodeValue(data1_node* node, char* pTagPath);
data1_node *data1_LookupNode(data1_node* node, char* pTagPath);
int data1_CountOccurences(data1_node* node, char* pTagPath);

 
FILE *data1_path_fopen (data1_handle dh, const char *file,
                                   const char *mode);
void data1_set_tabpath(data1_handle dh, const char *path);
void data1_set_tabroot (data1_handle dp, const char *p);
const char *data1_get_tabpath(data1_handle dh);
const char *data1_get_tabroot(data1_handle dh);



/* =========================================================================
 * Filter stuff
 * ========================================================================= 
 */
int grs_perl_readf(struct perl_context *context, size_t len);
off_t grs_perl_seekf(struct perl_context *context, off_t offset);
off_t grs_perl_tellf(struct perl_context *context);
void grs_perl_endf(struct perl_context *context, off_t offset);

data1_handle *grs_perl_get_dh(struct perl_context *context);
NMEM *grs_perl_get_mem(struct perl_context *context);
void grs_perl_set_res(struct perl_context *context, data1_node *n);

