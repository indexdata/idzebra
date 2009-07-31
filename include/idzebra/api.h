/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** \file api.h
    \brief Zebra API
    
    Return codes:
    Most functions has return type ZEBRA_RES, where ZEBRA_FAIL indicates
    failure; ZEBRA_OK indicates success.
*/

#ifndef IDZEBRA_API_H
#define IDZEBRA_API_H

#include <yaz/odr.h>
#include <yaz/proto.h>
#include <idzebra/res.h>
#include <idzebra/version.h>
#include <idzebra/recctrl.h>

YAZ_BEGIN_CDECL

typedef struct {
    zint processed;
    zint inserted;
    zint updated;
    zint deleted;
    long utime;
    long stime;
} ZebraTransactionStatus;

/** Retrieval Record Descriptor */
typedef struct {
    int errCode;         /* non-zero if error when fetching this */
    char *errString;     /* error string */
    int position;        /* position of record in result set (1,2,..) */
    char *buf;           /* record buffer (void pointer really) */
    int len;             /* length */
    const Odr_oid *format; /* record syntax */
    char *base; 
    zint sysno;
    int  score;
} ZebraRetrievalRecord;

/** Scan Term Descriptor */
typedef struct {
    zint occurrences;    /* scan term occurrences */
    char *term;          /* scan term string */
    char *display_term;  /* display scan term entry */
} ZebraScanEntry;

/** \var ZebraHandle
    \brief a Zebra Handle - (session)
*/
typedef struct zebra_session *ZebraHandle;

/** \var ZebraService
    \brief a Zebra Service handle
*/
typedef struct zebra_service *ZebraService;

/** \brief Creates a Zebra Service.
    \param configName name of configuration file
    
    This function is a simplified version of zebra_start_res.
*/
YAZ_EXPORT
ZebraService zebra_start(const char *configName
    ) ZEBRA_GCC_ATTR((warn_unused_result));

/** \brief Creates a Zebra service with resources.
    \param configName name of configuration file
    \param def_res default resources
    \param over_res overriding resources
    
    This function typically called once in a program. A Zebra Service
    acts as a factory for Zebra session handles.
*/
YAZ_EXPORT
ZebraService zebra_start_res(const char *configName,
			     Res def_res, Res over_res
    ) ZEBRA_GCC_ATTR((warn_unused_result));

/** \brief stops a Zebra service.
    \param zs service handle
    
    Frees resources used by the service.
*/
YAZ_EXPORT
ZEBRA_RES zebra_stop(ZebraService zs);

/** \brief Lists enabled Zebra filters
    \param zs service handle
    \param cd callback parameter (opaque)
    \param cb callback function
*/
YAZ_EXPORT
void zebra_filter_info(ZebraService zs, void *cd,
		       void (*cb)(void *cd, const char *name));


/** \brief Creates a Zebra session handle within service.
    \param zs service handle.
    \param res resources to be used for the service (NULL for none)
    
    There should be one handle for each thread doing something
    with zebra, be that searching or indexing. In simple apps 
    one handle is sufficient 
*/
YAZ_EXPORT
ZebraHandle zebra_open(ZebraService zs, Res res
    ) ZEBRA_GCC_ATTR((warn_unused_result));

/** \brief Destroys Zebra session handle.
    \param zh zebra session handle.
 */
YAZ_EXPORT
ZEBRA_RES zebra_close(ZebraHandle zh);

/** \brief Returns error code for last error
    \param zh zebra session handle.
*/
YAZ_EXPORT
int zebra_errCode(ZebraHandle zh);

/** \brief Returns error string for last error
    \param zh zebra session handle.
*/
YAZ_EXPORT
const char *zebra_errString(ZebraHandle zh);

/** \brief Returns additional info for last error
    \param zh zebra session handle.
*/
YAZ_EXPORT
char *zebra_errAdd(ZebraHandle zh);

/** \brief Returns error code and additional info for last error
    \param zh zebra session handle.
    \param code pointer to returned error code
    \param addinfo pointer to returned additional info
*/
YAZ_EXPORT
void zebra_result(ZebraHandle zh, int *code, char **addinfo);


/** \brief Returns character set encoding for session
    \param zh zebra session handle.
    \returns encoding name (e.g. "iso-8859-1")
*/
YAZ_EXPORT
const char *zebra_get_encoding(ZebraHandle zh);

/** \brief Set limit before Zebra does approx hit count
    \param zh session handle
    \param approx_limit the limit
    
    Results will be approximiate if hit count is greater than the
    limit specified. By default there is a high-limit (no limit).
*/
ZEBRA_RES zebra_set_approx_limit(ZebraHandle zh, zint approx_limit);

/** \brief Search using PQF Query String
    \param zh session handle
    \param pqf_query query
    \param setname name of resultset
    \param hits of hits is returned
*/
YAZ_EXPORT
ZEBRA_RES zebra_search_PQF(ZebraHandle zh, const char *pqf_query,
		           const char *setname, zint *hits);

/** \brief Search using RPN Query structure (from ASN.1)
    \param zh session handle
    \param o ODR handle
    \param query RPN query using YAZ structure
    \param setname name of resultset
    \param hits number of hits is returned
    \param estimated_hit_count whether hit count is an estimate
    \param partial_resultset whether result is only partially evaluated
*/
YAZ_EXPORT
ZEBRA_RES zebra_search_RPN_x(ZebraHandle zh, ODR o, Z_RPNQuery *query,
                           const char *setname, zint *hits,
                           int *estimated_hit_count,
                           int *partial_resultset);


/** \brief Search using RPN Query structure (from ASN.1)
    \param zh session handle
    \param o ODR handle
    \param query RPN query using YAZ structure
    \param setname name of resultset
    \param hits number of hits is returned
*/
YAZ_EXPORT
ZEBRA_RES zebra_search_RPN(ZebraHandle zh, ODR o, Z_RPNQuery *query,
			      const char *setname, zint *hits);

/** \brief Retrieve records from result set (after search)
    \param zh session handle
    \param stream allocate records returned using this ODR
    \param setname name of result set to retrieve records from
    \param comp Z39.50 record composition
    \param input_format transfer syntax (OID)
    \param num_recs number of records to retrieve
    \param recs store records in this structure (size is num_recs)
*/
YAZ_EXPORT
ZEBRA_RES zebra_records_retrieve(ZebraHandle zh, ODR stream,
				 const char *setname,
				 Z_RecordComposition *comp,
				 const Odr_oid *input_format,
				 int num_recs,
				 ZebraRetrievalRecord *recs);
/** \brief Deletes one or more resultsets 
    \param zh session handle
    \param function Z_DeleteResultSetRequest_{list,all}
    \param num_setnames number of result sets
    \param setnames result set names
    \param statuses status result
*/
YAZ_EXPORT
int zebra_deleteResultSet(ZebraHandle zh, int function,
			  int num_setnames, char **setnames,
			  int *statuses);


/** \brief returns number of term info terms assocaited with result set
    \param zh session handle
    \param setname result set name
    \param num_terms number of terms returned in this integer
    
    This function is used in conjunction with zebra_result_set_term_info.
    If operation was successful, ZEBRA_OK is returned; otherwise
    ZEBRA_FAIL is returned (typically non-existing setname)
*/
YAZ_EXPORT
ZEBRA_RES zebra_result_set_term_no(ZebraHandle zh, const char *setname,
				   int *num_terms);

/** \brief returns information about a term assocated with a result set
    \param zh session handle
    \param setname result set name
    \param no the term we want to know about (0=first, 1=second,..)
    \param count the number of occurrences of this term, aka hits (output) 
    \param approx about hits: 0=exact,1=approx (output)
    \param termbuf buffer for term string (intput, output)
    \param termlen size of termbuf (input=max, output=actual length)
    \param term_ref_id if non-NULL *term_ref_id holds term reference
    
    Returns information about one search term associated with result set.
    Use zebra_result_set_term_no to read total number of terms associated
    with result set. If this function can not return information,
    due to no out of range or bad result set name, ZEBRA_FAIL is
    returned.
    The passed termbuf must be able to hold at least *termlen characters.
    Upon completion, *termlen holds actual length of search term.
*/
YAZ_EXPORT
ZEBRA_RES zebra_result_set_term_info(ZebraHandle zh, const char *setname,
				     int no, zint *count, int *approx,
				     char *termbuf, size_t *termlen,
				     const char **term_ref_id);


/** \brief performs Scan (Z39.50 style)
    \param zh session handle
    \param stream ODR handle for result
    \param zapt Attribute plus Term (start term)
    \param attributeset Attributeset for Attribute plus Term
    \param position input/output position
    \param num_entries number of terms requested / returned 
    \param entries list of resulting terms (ODR allocated)
    \param is_partial upon return 1=partial, 0=complete
    \param setname limit scan by this set (NULL means no limit)
*/
YAZ_EXPORT ZEBRA_RES zebra_scan(ZebraHandle zh, ODR stream,
				Z_AttributesPlusTerm *zapt,
				const Odr_oid *attributeset,
				int *position, int *num_entries,
				ZebraScanEntry **entries,
				int *is_partial,
				const char *setname);

/** \brief performs Scan (taking PQF string)
    \param zh session handle
    \param stream ODR handle for result
    \param query PQF scan query
    \param position input/output position
    \param num_entries number of terms requested / returned 
    \param entries list of resulting terms (ODR allocated)
    \param is_partial upon return 1=partial, 0=complete
    \param setname limit scan by this set (NULL means no limit)
*/
YAZ_EXPORT
ZEBRA_RES zebra_scan_PQF(ZebraHandle zh, ODR stream, const char *query,
			 int *position, int *num_entries,
			 ZebraScanEntry **entries,
			 int *is_partial, const char *setname);

/** \brief authenticate user. Returns 0 if OK, != 0 on failure
    \param zh session handle
    \param user user name
    \param pass password
*/
YAZ_EXPORT
ZEBRA_RES zebra_auth(ZebraHandle zh, const char *user, const char *pass);

/** \brief Normalize zebra term for register (subject to change!)
    \param zh session handle
    \param index_type "w", "p",..
    \param input_str input string buffer
    \param input_len input string length
    \param output_str output string buffer
    \param output_len output string length
*/
YAZ_EXPORT
int zebra_string_norm(ZebraHandle zh, const char *index_type,
                      const char *input_str, 
		      int input_len, char *output_str, int output_len);

/** \brief Creates a database
    \param zh session handle
    \param db database to be created
*/
YAZ_EXPORT
ZEBRA_RES zebra_create_database(ZebraHandle zh, const char *db);

/** \brief Deletes a database (drop)
    \param zh session handle
    \param db database to be deleted
*/
YAZ_EXPORT
ZEBRA_RES zebra_drop_database(ZebraHandle zh, const char *db);

YAZ_EXPORT
ZEBRA_RES zebra_admin_shutdown(ZebraHandle zh);

YAZ_EXPORT
ZEBRA_RES zebra_admin_start(ZebraHandle zh);

YAZ_EXPORT
ZEBRA_RES zebra_shutdown(ZebraService zs);

YAZ_EXPORT
ZEBRA_RES zebra_admin_import_begin(ZebraHandle zh, const char *database,
				   const char *record_type);

YAZ_EXPORT 
ZEBRA_RES zebra_admin_import_segment(ZebraHandle zh,
				     Z_Segment *segment);

YAZ_EXPORT 
ZEBRA_RES zebra_admin_import_end(ZebraHandle zh);

YAZ_EXPORT 
ZEBRA_RES zebra_begin_trans(ZebraHandle zh, int rw
    ) ZEBRA_GCC_ATTR((warn_unused_result));

YAZ_EXPORT
ZEBRA_RES zebra_end_trans(ZebraHandle zh
    ) ZEBRA_GCC_ATTR((warn_unused_result));

YAZ_EXPORT
ZEBRA_RES zebra_end_transaction(ZebraHandle zh,
				ZebraTransactionStatus *stat);

YAZ_EXPORT
ZEBRA_RES zebra_commit(ZebraHandle zh);

YAZ_EXPORT
ZEBRA_RES zebra_clean(ZebraHandle zh);

YAZ_EXPORT
ZEBRA_RES zebra_init(ZebraHandle zh);

YAZ_EXPORT
ZEBRA_RES zebra_compact(ZebraHandle zh);

YAZ_EXPORT 
ZEBRA_RES zebra_repository_index(ZebraHandle zh, const char *path,
                                 enum zebra_recctrl_action_t action, char *useIndexDriver);

YAZ_EXPORT 
ZEBRA_RES zebra_repository_update(ZebraHandle zh, const char *path);

YAZ_EXPORT 
ZEBRA_RES zebra_repository_delete(ZebraHandle zh, const char *path);

YAZ_EXPORT 
ZEBRA_RES zebra_repository_show(ZebraHandle zh, const char *path);

/** \brief Simple update record
    \param zh session handle
    \param buf record buffer
    \param buf_size record buffer size

    This function is a simple wrapper or zebra_update_record with
    action=action_update (insert or replace) .
*/
YAZ_EXPORT 
ZEBRA_RES zebra_add_record(ZebraHandle zh, const char *buf, int buf_size);
			       
/** \brief Updates record
    \param zh session handle
    \param action (insert,replace,delete or update (replace/insert)
    \param recordType filter type (0 indicates default)
    \param sysno system id (0 may be passed for no known id)
    \param match match criteria (0 may be passed for no known criteria)
    \param fname filename to be printed for logging (0 may be passed)
    \param buf record buffer
    \param buf_size record buffer size
*/
YAZ_EXPORT
ZEBRA_RES zebra_update_record(ZebraHandle zh, 
                              enum zebra_recctrl_action_t action,
                              const char *recordType,
                              zint *sysno, const char *match,
                              const char *fname,
                              const char *buf, int buf_size);

YAZ_EXPORT 
ZEBRA_RES zebra_sort(ZebraHandle zh, ODR stream,
		     int num_input_setnames,
		     const char **input_setnames,
		     const char *output_setname,
		     Z_SortKeySpecList *sort_sequence,
		     int *sort_status
    ) ZEBRA_GCC_ATTR((warn_unused_result));    

YAZ_EXPORT
ZEBRA_RES zebra_select_databases(ZebraHandle zh, int num_bases, 
				 const char **basenames
    ) ZEBRA_GCC_ATTR((warn_unused_result));

YAZ_EXPORT
ZEBRA_RES zebra_select_database(ZebraHandle zh, const char *basename
    ) ZEBRA_GCC_ATTR((warn_unused_result));

YAZ_EXPORT
void zebra_shadow_enable(ZebraHandle zh, int value);

YAZ_EXPORT
int zebra_register_statistics(ZebraHandle zh, int dumpdict);

YAZ_EXPORT
ZEBRA_RES zebra_record_encoding(ZebraHandle zh, const char *encoding);

YAZ_EXPORT
ZEBRA_RES zebra_octet_term_encoding(ZebraHandle zh, const char *encoding);

/* Resources */
YAZ_EXPORT
void zebra_set_resource(ZebraHandle zh, const char *name, const char *value);
YAZ_EXPORT
const char *zebra_get_resource(ZebraHandle zh, 
			       const char *name, const char *defaultvalue);


YAZ_EXPORT
void zebra_pidfname(ZebraService zs, char *path);

typedef struct {
    char *term;
    char *db;
    zint sysno;
    int score;
} ZebraMetaRecord;

YAZ_EXPORT
ZebraMetaRecord *zebra_meta_records_create(ZebraHandle zh,
					   const char *name,
					   int num, zint *positions);


YAZ_EXPORT
ZebraMetaRecord *zebra_meta_records_create_range(ZebraHandle zh,
						 const char *name, 
						 zint start, int num);

YAZ_EXPORT
void zebra_meta_records_destroy(ZebraHandle zh, ZebraMetaRecord *records,
				int num);

YAZ_EXPORT 
struct BFiles_struct *zebra_get_bfs(ZebraHandle zh);

YAZ_EXPORT
ZEBRA_RES zebra_set_limit(ZebraHandle zh, int complement_flag, zint *ids);

YAZ_EXPORT
ZEBRA_RES zebra_set_break_handler(ZebraHandle zh, 
                                  int (*f)(void *client_data),
                                  void *client_data);

YAZ_END_CDECL				      

/** \mainpage Zebra
 *
 * \section intro_sec Introduction
 *
 * Zebra is a search engine for structure data, such as XML, MARC
 * and others.
 *
 * API users should read the api.h for all the public definitions.
 *
 * The remaining sections briefly describe each of
 * Zebra major modules/components.
 *
 * \section util Base Utilities
 *
 * The Zebra utilities (util.h) defines fundamental types and a few
 * utilites for Zebra.
 *
 * \section res Resources
 *
 * The resources system (res.h) is a manager of configuration 
 * resources. The resources can be viewed as a simple database.
 * Resources can be read from a configurtion file, they can be
 * read or written by an application. Resources can also be written,
 * but that facility is not currently in use.
 *
 * \section bfile Bfiles
 *
 * The Bfiles (bfile.h) provides a portable interface to the
 * local file system. It also provides a facility for safe updates
 * (shadow updates). All file system access is handle by this module
 * (except for trival reads of configuration files).
 *
 * \section dict Dictionary
 *
 * The Zebra dictionary (dict.h) maps a search term (key) to a value. The
 * value is a reference to the list of records identifers in which
 * the term occurs. Zebra uses an ISAM data structure for the list
 * of term occurrences. The Dictionary uses \ref bfile.
 *
 * \section isam ISAM
 *
 * Zebra maintains an ISAM for each term where each ISAM is a list
 * of record identifiers corresponding to the records in which the
 * term occur. Unlike traditional ISAM systems, the Zebra ISAM
 * is compressed. The ISAM system uses \ref bfile.
 *
 * Zebra has more than one ISAM system. The old and stable ISAM system
 * is named isamc (see isamc.h). Another version isams is a write-once
 * isam system that is quite compact - suitable for CD-ROMs (isams.h). 
 * The newest ISAM system, isamb, is implemented as a B-Tree (see isamb.h).
 *
 * \section data1 Data-1
 *
 * The data1 (data1.h) module deals with structured documents. The module can
 * can read, modify and write documents. The document structure was
 * originally based on GRS-1 - a Z39.50 v3 structure that predates
 * DOM. These days the data1 structure may describe XML/SGML as well.
 * The data1, like DOM, is a tree structure. Each node in the tree
 * can be of type element, text (cdata), preprocessing instruction,
 * comment. Element nodes can point to attribute nodes.
 *
 * \section recctrl Record Control
 *
 * The record control module (recctrl.h) is responsible for
 * managing the various record types ("classes" or filters).
 *
 * \section rset Result-Set
 *
 * The Result-Set module (rset.h) defines an interface that all
 * Zebra Search Results must implement. Each operation (AND, OR, ..)
 * correspond to an implementation of that interface.
 *
 * \section dfa DFA
 *
 * DFA (dfa.h) Deterministic Finite Automa is a regular expression engine.
 * The module compiles a regular expression to a DFA. The DFA can then
 * be used in various application to perform fast match against the
 * origianl expression. The \ref Dict uses DFA to perform lookup
 * using regular expressions.
 */

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

