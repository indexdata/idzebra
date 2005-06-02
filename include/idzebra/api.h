/* $Id: api.h,v 1.25 2005-06-02 11:59:53 adam Exp $
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

/** \file api.h
    \brief Zebra API
    
    Return codes:
    Most functions return ZEBRA_RES, where ZEBRA_FAIL indicates
    failure; ZEBRA_OK indicates success.
*/

#ifndef IDZEBRA_API_H
#define IDZEBRA_API_H

#include <yaz/odr.h>
#include <yaz/oid.h>
#include <yaz/proto.h>
#include <idzebra/res.h>
#include <idzebra/version.h>

YAZ_BEGIN_CDECL

typedef struct {
    int processed;
    int inserted;
    int updated;
    int deleted;
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
    oid_value format;    /* record syntax */
    char *base; 
    SYSNO sysno;
    int  score;
} ZebraRetrievalRecord;

/** Scan Term Descriptor */
typedef struct {
    int occurrences;     /* scan term occurrences */
    char *term;          /* scan term string */
} ZebraScanEntry;

/** \var ZebraHandle
    \brief a Zebra Handle - (session)
*/
typedef struct zebra_session *ZebraHandle;

/** \var ZebraService
    \brief a Zebra Service handle
*/
typedef struct zebra_service *ZebraService;

/** \fn ZebraService zebra_start(const char *configName)
    \brief starts a Zebra service. 
    \param configName name of configuration file
    
    This function is a simplified version of zebra_start_res.
*/
YAZ_EXPORT
ZebraService zebra_start(const char *configName);

/** \fn ZebraService zebra_start_res(const char *configName, \
                                     Res def_res, Res over_res)
    \brief starts a Zebra service with resources.
    \param configName name of configuration file
    \param def_res default resources
    \param over_res overriding resources
    
    This function typically called once in a program. A Zebra Service
    acts as a factory for Zebra session handles.
*/
YAZ_EXPORT
ZebraService zebra_start_res(const char *configName,
			     Res def_res, Res over_res);

/**
   \brief stops a Zebra service.
   \param zs service handle
   
   Frees resources used by the service.
*/
YAZ_EXPORT
ZEBRA_RES zebra_stop(ZebraService zs);

/**
   \brief Lists enabled Zebra filters
   \param zs service handle
   \param cd callback parameter (opaque)
   \param cb callback function
 */
YAZ_EXPORT
void zebra_filter_info(ZebraService zs, void *cd,
		       void (*cb)(void *cd, const char *name));


/**
   \brief Creates a Zebra session handle within service.
   \param zs service handle.
   
   There should be one handle for each thread doing something
   with zebra, be that searching or indexing. In simple apps 
   one handle is sufficient 
*/
YAZ_EXPORT
ZebraHandle zebra_open(ZebraService zs);

/**
   \brief Destroys Zebra session handle.
   \param zh zebra session handle.
 */
YAZ_EXPORT
ZEBRA_RES zebra_close(ZebraHandle zh);

/**
   \brief Returns error code for last error
   \param zh zebra session handle.
*/
YAZ_EXPORT
int zebra_errCode(ZebraHandle zh);

/**
   \brief Returns error string for last error
   \param zh zebra session handle.
*/
YAZ_EXPORT
const char *zebra_errString(ZebraHandle zh);

/**
   \brief Returns additional info for last error
   \param zh zebra session handle.
*/
YAZ_EXPORT
char *zebra_errAdd(ZebraHandle zh);

/**
   \brief Returns error code and additional info for last error
   \param zh zebra session handle.
   \param code pointer to returned error code
   \param addinfo pointer to returned additional info
*/
YAZ_EXPORT
void zebra_result(ZebraHandle zh, int *code, char **addinfo);

/** 
    \brief Clears last error.
    \param zh zebra session handle.
*/
YAZ_EXPORT
void zebra_clearError(ZebraHandle zh);

/**
   \brief Search using PQF Query 
   \param zh session handle
   \param pqf_query query
   \param setname name of resultset
   \param hits of hits is returned
 */
YAZ_EXPORT
ZEBRA_RES zebra_search_PQF(ZebraHandle zh, const char *pqf_query,
		           const char *setname, zint *hits);

/** \fn ZEBRA_RES zebra_search_RPN(ZebraHandle zh, ODR o, Z_RPNQuery *query, \
		const char *setname, zint *hits)
    \brief Search using RPN Query 
    \param zh session handle
    \param o ODR handle
    \param query RPN query using YAZ structure
    \param setname name of resultset
    \param hits number of hits is returned
 */
YAZ_EXPORT
ZEBRA_RES zebra_search_RPN(ZebraHandle zh, ODR o, Z_RPNQuery *query,
			      const char *setname, zint *hits);

/** 
    \fn ZEBRA_RES zebra_records_retrieve(ZebraHandle zh, ODR stream, \
		const char *setname, Z_RecordComposition *comp, \
		oid_value input_format, int num_recs, \
		ZebraRetrievalRecord *recs)
    \brief Retrieve records from result set (after search)
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
				 oid_value input_format,
				 int num_recs,
				 ZebraRetrievalRecord *recs);
/**
   \brief Deletes one or more resultsets 
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



YAZ_EXPORT
ZEBRA_RES zebra_result_set_term_no(ZebraHandle zh, const char *setname,
				   int *num_terms);

YAZ_EXPORT
ZEBRA_RES zebra_result_set_term_info(ZebraHandle zh, const char *setname,
				     int no, zint *count, int *approx,
				     char *termbuf, size_t *termlen);


/**
   \brief performs Scan (Z39.50 style)
   \param zh session handle
   \param stream ODR handle for result
   \param zapt Attribute plus Term (start term)
   \param attributeset Attributeset for Attribute plus Term
   \param position input/output position
   \param num_entries number of terms requested / returned 
   \param entries list of resulting terms (ODR allocated)
   \param is_partial upon return 1=partial, 0=complete
*/
YAZ_EXPORT ZEBRA_RES zebra_scan(ZebraHandle zh, ODR stream,
				Z_AttributesPlusTerm *zapt,
				oid_value attributeset,
				int *position, int *num_entries,
				ZebraScanEntry **entries,
				int *is_partial);

/**
   \brief performs Scan (taking PQF string)
   \param zh session handle
   \param stream ODR handle for result
   \param query PQF scan query
   \param position input/output position
   \param num_entries number of terms requested / returned 
   \param entries list of resulting terms (ODR allocated)
   \param is_partial upon return 1=partial, 0=complete
*/
YAZ_EXPORT
ZEBRA_RES zebra_scan_PQF(ZebraHandle zh, ODR stream, const char *query,
		    int *position, int *num_entries, ZebraScanEntry **entries,
		    int *is_partial);

/**
   \brief authenticate user. Returns 0 if OK, != 0 on failure
   \param zh session handle
   \param user user name
   \param pass password
 */
YAZ_EXPORT
ZEBRA_RES zebra_auth(ZebraHandle zh, const char *user, const char *pass);

/**
   \brief Normalize zebra term for register (subject to change!)
   \param zh session handle
   \param reg_id register ID, 'w', 'p',..
   \param input_str input string buffer
   \param input_len input string length
   \param output_str output string buffer
   \param output_len output string length
 */
YAZ_EXPORT
int zebra_string_norm(ZebraHandle zh, unsigned reg_id, const char *input_str, 
		int input_len, char *output_str, int output_len);

/**
   \brief Creates a database
   \param zh session handle
   \param db database to be created
*/
YAZ_EXPORT
ZEBRA_RES zebra_create_database(ZebraHandle zh, const char *db);

/**
   \brief Deletes a database (drop)
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
ZEBRA_RES zebra_admin_exchange_record(ZebraHandle zh,
				      const char *rec_buf,
				      size_t rec_len,
				      const char *recid_buf, size_t recid_len,
				      int action);

YAZ_EXPORT 
ZEBRA_RES zebra_begin_trans(ZebraHandle zh, int rw);

YAZ_EXPORT
ZEBRA_RES zebra_end_trans(ZebraHandle zh);

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

YAZ_EXPORT int zebra_repository_update(ZebraHandle zh, const char *path);
YAZ_EXPORT int zebra_repository_delete(ZebraHandle zh, const char *path);
YAZ_EXPORT int zebra_repository_show(ZebraHandle zh, const char *path);

YAZ_EXPORT int zebra_add_record(ZebraHandle zh, const char *buf, int buf_size);
			       
YAZ_EXPORT 
ZEBRA_RES zebra_insert_record(ZebraHandle zh, 
			      const char *recordType,
			      SYSNO *sysno, const char *match,
			      const char *fname,
			      const char *buf, int buf_size,
			      int force_update);
YAZ_EXPORT
ZEBRA_RES zebra_update_record(ZebraHandle zh, 
			      const char *recordType,
			      SYSNO *sysno, const char *match,
			      const char *fname,
			      const char *buf, int buf_size,
			      int force_update);
YAZ_EXPORT 
ZEBRA_RES zebra_delete_record(ZebraHandle zh, 
			      const char *recordType,
			      SYSNO *sysno, const char *match, const char *fname,
			      const char *buf, int buf_size,
			      int force_update);

YAZ_EXPORT 
int zebra_resultSetTerms(ZebraHandle zh, const char *setname, 
			 int no, zint *count, 
			 int *type, char *out, size_t *len);

YAZ_EXPORT 
ZEBRA_RES zebra_sort(ZebraHandle zh, ODR stream,
		     int num_input_setnames,
		     const char **input_setnames,
		     const char *output_setname,
		     Z_SortKeySpecList *sort_sequence,
		     int *sort_status);

YAZ_EXPORT
ZEBRA_RES zebra_select_databases(ZebraHandle zh, int num_bases, 
				 const char **basenames);

YAZ_EXPORT
ZEBRA_RES zebra_select_database(ZebraHandle zh, const char *basename);

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

YAZ_END_CDECL				      

/** \mainpage Zebra
 *
 * \section intro_sec Introduction
 *
 * Zebra is a search engine for structure data, such as XML, MARC
 * and others. The following chapters briefly describe each of
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
