/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zinfo.c,v $
 * Revision 1.20  2000-11-29 14:24:01  adam
 * Script configure uses yaz pthreads options. Added locking for
 * zebra_register_{lock,unlock}.
 *
 * Revision 1.19  2000/07/07 12:49:20  adam
 * Optimized resultSetInsert{Rank,Sort}.
 *
 * Revision 1.18  2000/03/20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.17  1999/07/14 10:53:51  adam
 * Updated various routines to handle missing explain schema.
 *
 * Revision 1.16  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.15  1999/01/25 13:47:54  adam
 * Fixed bug.
 *
 * Revision 1.14  1998/11/04 16:31:32  adam
 * Fixed bug regarding recordBytes in databaseInfo.
 *
 * Revision 1.13  1998/11/03 10:17:09  adam
 * Fixed bug regarding creation of some data1 nodes for Explain records.
 *
 * Revision 1.12  1998/10/13 20:37:11  adam
 * Changed the way attribute sets are saved in Explain database to
 * reflect "dynamic" OIDs.
 *
 * Revision 1.11  1998/06/09 12:16:48  adam
 * Implemented auto-generation of CategoryList records.
 *
 * Revision 1.10  1998/06/08 14:43:15  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.9  1998/06/02 12:10:27  adam
 * Fixed bug related to attributeDetails.
 *
 * Revision 1.8  1998/05/20 10:12:20  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.7  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.6  1998/02/17 10:29:27  adam
 * Moved towards 'automatic' EXPLAIN database.
 *
 * Revision 1.5  1997/10/27 14:33:05  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.4  1997/09/25 14:57:08  adam
 * Added string.h.
 *
 * Revision 1.3  1996/05/22 08:21:59  adam
 * Added public ZebDatabaseInfo structure.
 *
 * Revision 1.2  1996/05/14 06:16:41  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.1  1996/05/13 14:23:07  adam
 * Work on compaction of set/use bytes in dictionary.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include <zebraver.h>
#include "zinfo.h"

#define ZINFO_DEBUG 0

struct zebSUInfo {
    int set;
    int use;
    int ordinal;
};

struct zebSUInfoB {
    struct zebSUInfo info;
    struct zebSUInfoB *next;
};

typedef struct zebAccessObjectB *zebAccessObject;
struct zebAccessObjectB {
    void *handle;
    int sysno;
    Odr_oid *oid;
    zebAccessObject next;
};

typedef struct zebAccessInfoB *zebAccessInfo;
struct zebAccessInfoB {
    zebAccessObject attributeSetIds;
    zebAccessObject schemas;
};

typedef struct {
    struct zebSUInfoB *SUInfo;
    int sysno;
    int dirty;
    int readFlag;
    data1_node *data1_tree;
} *zebAttributeDetails;

struct zebDatabaseInfoB {
    zebAttributeDetails attributeDetails;
    char *databaseName;
    data1_node *data1_database;
    int recordCount;     /* records in db */
    int recordBytes;     /* size of records */
    int sysno;           /* sysno of database info */
    int readFlag;        /* 1: read is needed when referenced; 0 if not */
    int dirty;           /* 1: database is dirty: write is needed */
    struct zebDatabaseInfoB *next;
    zebAccessInfo accessInfo;
};

struct zebraExplainAttset {
    char *name;
    int ordinal;
    struct zebraExplainAttset *next;
};

struct zebraCategoryListInfo {
    int dirty;
    int sysno;
    data1_node *data1_categoryList;
};

struct zebraExplainInfo {
    int  ordinalSU;
    int  runNumber;
    int  dirty;
    Records records;
    data1_handle dh;
    Res res;
    struct zebraExplainAttset *attsets;
    NMEM nmem;
    data1_node *data1_target;
    struct zebraCategoryListInfo *categoryList;
    struct zebDatabaseInfoB *databaseInfo;
    struct zebDatabaseInfoB *curDatabaseInfo;
    zebAccessInfo accessInfo;
    char date[15]; /* YYYY MMDD HH MM SS */
    int (*updateFunc)(void *handle, Record drec, data1_node *n);
    void *updateHandle;
};

static void zebraExplain_initCommonInfo (ZebraExplainInfo zei, data1_node *n);
static void zebraExplain_initAccessInfo (ZebraExplainInfo zei, data1_node *n);

static data1_node *read_sgml_rec (data1_handle dh, NMEM nmem, Record rec)
{
    return data1_read_sgml (dh, nmem, rec->info[recInfo_storeData]);
}

static data1_node *data1_search_tag (data1_handle dh, data1_node *n,
				    const char *tag)
{
    logf (LOG_DEBUG, "data1_search_tag %s", tag);
    for (; n; n = n->next)
	if (n->which == DATA1N_tag && n->u.tag.tag &&
	    !yaz_matchstr (tag, n->u.tag.tag))
	{
	    logf (LOG_DEBUG, " found");
	    return n;
	}
    logf (LOG_DEBUG, " not found");
    return 0;
}

static data1_node *data1_add_tag (data1_handle dh, data1_node *at,
				  const char *tag, NMEM nmem)
{
    data1_node *partag = get_parent_tag(dh, at);
    data1_node *res = data1_mk_node_type (dh, nmem, DATA1N_tag);
    data1_element *e = NULL;

    res->parent = at;
    res->u.tag.tag = data1_insert_string (dh, res, nmem, tag);
   
    if (partag)
	e = partag->u.tag.element;
    res->u.tag.element =
	data1_getelementbytagname (dh, at->root->u.root.absyn,
				   e, res->u.tag.tag);
    res->root = at->root;
    if (!at->child)
	at->child = res;
    else
    {
	assert (at->last_child);
	at->last_child->next = res;
    }
    at->last_child = res;
    return res;
}

static data1_node *data1_make_tag (data1_handle dh, data1_node *at,
				   const char *tag, NMEM nmem)
{
    data1_node *node = data1_search_tag (dh, at->child, tag);
    if (!node)
	node = data1_add_tag (dh, at, tag, nmem);
    else
	node->child = node->last_child = NULL;
    return node;
}

static data1_node *data1_add_tagdata_int (data1_handle dh, data1_node *at,
					  const char *tag, int num,
					  NMEM nmem)
{
    data1_node *node_data;
    
    node_data = data1_add_taggeddata (dh, at->root, at, tag, nmem);
    if (!node_data)
	return 0;
    node_data->u.data.what = DATA1I_num;
    node_data->u.data.data = node_data->lbuf;
    sprintf (node_data->u.data.data, "%d", num);
    node_data->u.data.len = strlen (node_data->u.data.data);
    return node_data;
}

static data1_node *data1_add_tagdata_oid (data1_handle dh, data1_node *at,
					   const char *tag, Odr_oid *oid,
					   NMEM nmem)
{
    data1_node *node_data;
    char str[128], *p = str;
    Odr_oid *ii;
    
    node_data = data1_add_taggeddata (dh, at->root, at, tag, nmem);
    if (!node_data)
	return 0;
    
    for (ii = oid; *ii >= 0; ii++)
    {
	if (ii != oid)
	    *p++ = '.';
	sprintf (p, "%d", *ii);
	p += strlen (p);
    }
    node_data->u.data.what = DATA1I_oid;
    node_data->u.data.len = strlen (str);
    node_data->u.data.data = data1_insert_string (dh, node_data, nmem, str);
    return node_data;
}


static data1_node *data1_add_tagdata_text (data1_handle dh, data1_node *at,
					   const char *tag, const char *str,
					   NMEM nmem)
{
    data1_node *node_data;
    
    node_data = data1_add_taggeddata (dh, at->root, at, tag, nmem);
    if (!node_data)
	return 0;
    node_data->u.data.what = DATA1I_text;
    node_data->u.data.len = strlen (str);
    node_data->u.data.data = data1_insert_string (dh, node_data, nmem, str);
    return node_data;
}

static data1_node *data1_make_tagdata_text (data1_handle dh, data1_node *at,
					    const char *tag, const char *str,
					    NMEM nmem)
{
    data1_node *node = data1_search_tag (dh, at->child, tag);
    if (!node)
        return data1_add_tagdata_text (dh, at, tag, str, nmem);
    else
    {
	data1_node *node_data = node->child;
	node_data->u.data.what = DATA1I_text;
	node_data->u.data.len = strlen (str);
	node_data->u.data.data = data1_insert_string (dh, node_data,
						      nmem, str);
	return node_data;
    }
}

static void zebraExplain_writeDatabase (ZebraExplainInfo zei,
                                        struct zebDatabaseInfoB *zdi,
					int key_flush);
static void zebraExplain_writeAttributeDetails (ZebraExplainInfo zei,
						zebAttributeDetails zad,
						const char *databaseName,
						int key_flush);
static void zebraExplain_writeTarget (ZebraExplainInfo zei, int key_flush);
static void zebraExplain_writeAttributeSet (ZebraExplainInfo zei,
					    zebAccessObject o,
					    int key_flush);
static void zebraExplain_writeCategoryList (ZebraExplainInfo zei,
					    struct zebraCategoryListInfo *zcl,
					    int key_flush);


static Record createRecord (Records records, int *sysno)
{
    Record rec;
    if (*sysno)
    {
	rec = rec_get (records, *sysno);
	xfree (rec->info[recInfo_storeData]);
    }
    else
    {
	rec = rec_new (records);
	*sysno = rec->sysno;
	
	rec->info[recInfo_fileType] =
	    rec_strdup ("grs.sgml", &rec->size[recInfo_fileType]);
	rec->info[recInfo_databaseName] =
	    rec_strdup ("IR-Explain-1",
			&rec->size[recInfo_databaseName]); 
    }
    return rec;
}

void zebraExplain_flush (ZebraExplainInfo zei, int writeFlag, void *handle)
{
    zei->updateHandle = handle;
    if (writeFlag)
    {
	struct zebDatabaseInfoB *zdi;
	zebAccessObject o;

	/* write each database info record */
	for (zdi = zei->databaseInfo; zdi; zdi = zdi->next)
	{
	    zebraExplain_writeDatabase (zei, zdi, 1);
	    zebraExplain_writeAttributeDetails (zei, zdi->attributeDetails,
						zdi->databaseName, 1);
	}
	zebraExplain_writeTarget (zei, 1);
	zebraExplain_writeCategoryList (zei,
					zei->categoryList,
					1);
	assert (zei->accessInfo);
	for (o = zei->accessInfo->attributeSetIds; o; o = o->next)
	    if (!o->sysno)
		zebraExplain_writeAttributeSet (zei, o, 1);
	for (o = zei->accessInfo->schemas; o; o = o->next)
	    if (!o->sysno)
	    {
/* 		zebraExplain_writeSchema (zei, o, 1); */
	    }

	for (zdi = zei->databaseInfo; zdi; zdi = zdi->next)
	{
	    zebraExplain_writeDatabase (zei, zdi, 0);
	    zebraExplain_writeAttributeDetails (zei, zdi->attributeDetails,
						zdi->databaseName, 0);
	}
	zebraExplain_writeTarget (zei, 0);
    }
}

void zebraExplain_close (ZebraExplainInfo zei, int writeFlag)
{
#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_close wr=%d", writeFlag);
#endif
    if (!zei)
	return;
    zebraExplain_flush (zei, writeFlag, zei->updateHandle);
    nmem_destroy (zei->nmem);
}

void zebraExplain_mergeOids (ZebraExplainInfo zei, data1_node *n,
			     zebAccessObject *op)
{
    data1_node *np;

    for (np = n->child; np; np = np->next)
    {
	char str[64];
	int len;
	Odr_oid *oid;
	zebAccessObject ao;

	if (np->which != DATA1N_tag || strcmp (np->u.tag.tag, "oid"))
	    continue;
	len = np->child->u.data.len;
	if (len > 63)
	    len = 63;
	memcpy (str, np->child->u.data.data, len);
	str[len] = '\0';
	
	oid = odr_getoidbystr_nmem (zei->nmem, str);

	for (ao = *op; ao; ao = ao->next)
	    if (!oid_oidcmp (oid, ao->oid))
	    {
		ao->sysno = 1;
		break;
	    }
	if (!ao)
	{
	    ao = (zebAccessObject) nmem_malloc (zei->nmem, sizeof(*ao));
	    ao->handle = NULL;
	    ao->sysno = 1;
	    ao->oid = oid;
	    ao->next = *op;
	    *op = ao;
	}
    }
}

void zebraExplain_mergeAccessInfo (ZebraExplainInfo zei, data1_node *n,
				   zebAccessInfo *accessInfo)
{
    data1_node *np;
    
    if (!n)
    {
	*accessInfo = (zebAccessInfo)
	    nmem_malloc (zei->nmem, sizeof(**accessInfo));
	(*accessInfo)->attributeSetIds = NULL;
	(*accessInfo)->schemas = NULL;
    }
    else
    {
	if (!(n = data1_search_tag (zei->dh, n->child, "accessInfo")))
	    return;
	if ((np = data1_search_tag (zei->dh, n->child, "attributeSetIds")))
	    zebraExplain_mergeOids (zei, np,
				    &(*accessInfo)->attributeSetIds);
	if ((np = data1_search_tag (zei->dh, n->child, "schemas")))
	    zebraExplain_mergeOids (zei, np,
				    &(*accessInfo)->schemas);
    }
}

ZebraExplainInfo zebraExplain_open (
    Records records, data1_handle dh,
    Res res,
    int writeFlag,
    void *updateHandle,
    int (*updateFunc)(void *handle, Record drec, data1_node *n))
{
    Record trec;
    ZebraExplainInfo zei;
    struct zebDatabaseInfoB **zdip;
    time_t our_time;
    struct tm *tm;
    NMEM nmem = nmem_create ();

#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_open wr=%d", writeFlag);
#endif
    zei = (ZebraExplainInfo) nmem_malloc (nmem, sizeof(*zei));
    zei->updateHandle = updateHandle;
    zei->updateFunc = updateFunc;
    zei->dirty = 0;
    zei->curDatabaseInfo = NULL;
    zei->records = records;
    zei->nmem = nmem;
    zei->dh = dh;
    zei->attsets = NULL;
    zei->res = res;
    zei->categoryList = (struct zebraCategoryListInfo *)
	nmem_malloc (zei->nmem, sizeof(*zei->categoryList));
    zei->categoryList->sysno = 0;
    zei->categoryList->dirty = 0;
    zei->categoryList->data1_categoryList = NULL;

    time (&our_time);
    tm = localtime (&our_time);
    sprintf (zei->date, "%04d%02d%02d%02d%02d%02d",
	     tm->tm_year+1900, tm->tm_mon+1,  tm->tm_mday,
	     tm->tm_hour, tm->tm_min, tm->tm_sec);

    zdip = &zei->databaseInfo;
    trec = rec_get (records, 1);      /* get "root" record */

    zei->ordinalSU = 1;
    zei->runNumber = 0;

    zebraExplain_mergeAccessInfo (zei, 0, &zei->accessInfo);
    if (trec)    /* targetInfo already exists ... */
    {
	data1_node *node_tgtinfo, *node_zebra, *node_list, *np;

	zei->data1_target = read_sgml_rec (zei->dh, zei->nmem, trec);
	if (!zei->data1_target || !zei->data1_target->u.root.absyn)
	{
	    logf (LOG_FATAL, "Explain schema missing. Check profilePath");
	    nmem_destroy (zei->nmem);
	    return 0;
	}
#if ZINFO_DEBUG
	data1_pr_tree (zei->dh, zei->data1_target, stderr);
#endif
	node_tgtinfo = data1_search_tag (zei->dh, zei->data1_target->child,
					 "targetInfo");
	zebraExplain_mergeAccessInfo (zei, node_tgtinfo,
				      &zei->accessInfo);

	node_zebra = data1_search_tag (zei->dh, node_tgtinfo->child,
				       "zebraInfo");
	np = 0;
	if (node_zebra)
	{
	    node_list = data1_search_tag (zei->dh, node_zebra->child,
					  "databaseList");
	    if (node_list)
		np = node_list->child;
	}
	for (; np; np = np->next)
	{
	    data1_node *node_name = NULL;
	    data1_node *node_id = NULL;
	    data1_node *node_aid = NULL;
	    data1_node *np2;
	    if (np->which != DATA1N_tag || strcmp (np->u.tag.tag, "database"))
		continue;
	    for (np2 = np->child; np2; np2 = np2->next)
	    {
		if (np2->which != DATA1N_tag)
		    continue;
		if (!strcmp (np2->u.tag.tag, "name"))
		    node_name = np2->child;
		else if (!strcmp (np2->u.tag.tag, "id"))
		    node_id = np2->child;
		else if (!strcmp (np2->u.tag.tag, "attributeDetailsId"))
		    node_aid = np2->child;
	    }
	    assert (node_id && node_name && node_aid);
	    
	    *zdip = (struct zebDatabaseInfoB *) 
		nmem_malloc (zei->nmem, sizeof(**zdip));
            (*zdip)->readFlag = 1;
            (*zdip)->dirty = 0;
	    (*zdip)->data1_database = NULL;
	    (*zdip)->recordCount = 0;
	    (*zdip)->recordBytes = 0;
	    zebraExplain_mergeAccessInfo (zei, 0, &(*zdip)->accessInfo);

	    (*zdip)->databaseName = (char *)
		nmem_malloc (zei->nmem, 1+node_name->u.data.len);
	    memcpy ((*zdip)->databaseName, node_name->u.data.data,
		    node_name->u.data.len);
	    (*zdip)->databaseName[node_name->u.data.len] = '\0';
	    (*zdip)->sysno = atoi_n (node_id->u.data.data,
				     node_id->u.data.len);
	    (*zdip)->attributeDetails = (zebAttributeDetails)
		nmem_malloc (zei->nmem, sizeof(*(*zdip)->attributeDetails));
	    (*zdip)->attributeDetails->sysno = atoi_n (node_aid->u.data.data,
						       node_aid->u.data.len);
	    (*zdip)->attributeDetails->readFlag = 1;
	    (*zdip)->attributeDetails->dirty = 0;
	    (*zdip)->attributeDetails->SUInfo = NULL;

	    zdip = &(*zdip)->next;
	}
	if (node_zebra)
	{
	    np = data1_search_tag (zei->dh, node_zebra->child,
				   "ordinalSU");
	    np = np->child;
	    assert (np && np->which == DATA1N_data);
	    zei->ordinalSU = atoi_n (np->u.data.data, np->u.data.len);
	    
	    np = data1_search_tag (zei->dh, node_zebra->child,
				   "runNumber");
	    np = np->child;
	    assert (np && np->which == DATA1N_data);
	    zei->runNumber = atoi_n (np->u.data.data, np->u.data.len);
	    *zdip = NULL;
	}
	rec_rm (&trec);
    }
    else  /* create initial targetInfo */
    {
	data1_node *node_tgtinfo;

	*zdip = NULL;
	if (writeFlag)
	{
	    char *sgml_buf;
	    int sgml_len;

	    zei->data1_target =
		data1_read_sgml (zei->dh, zei->nmem,
				 "<explain><targetInfo>TargetInfo\n"
				 "<name>Zebra</>\n"
				 "<namedResultSets>1</>\n"
				 "<multipleDBSearch>1</>\n"
				 "<nicknames><name>Zebra</></>\n"
				 "</></>\n" );
	    if (!zei->data1_target || !zei->data1_target->u.root.absyn)
	    {
		logf (LOG_FATAL, "Explain schema missing. Check profilePath");
		nmem_destroy (zei->nmem);
		return 0;
	    }
	    node_tgtinfo = data1_search_tag (zei->dh, zei->data1_target->child,
					    "targetInfo");
	    assert (node_tgtinfo);

	    zebraExplain_initCommonInfo (zei, node_tgtinfo);
	    zebraExplain_initAccessInfo (zei, node_tgtinfo);

	    /* write now because we want to be sure about the sysno */
	    trec = rec_new (records);
	    trec->info[recInfo_fileType] =
		rec_strdup ("grs.sgml", &trec->size[recInfo_fileType]);
	    trec->info[recInfo_databaseName] =
		rec_strdup ("IR-Explain-1", &trec->size[recInfo_databaseName]);
	    
	    sgml_buf = data1_nodetoidsgml(dh, zei->data1_target, 0, &sgml_len);
	    trec->info[recInfo_storeData] = (char *) xmalloc (sgml_len);
	    memcpy (trec->info[recInfo_storeData], sgml_buf, sgml_len);
	    trec->size[recInfo_storeData] = sgml_len;
	    
	    rec_put (records, &trec);
	    rec_rm (&trec);

	}
	zebraExplain_newDatabase (zei, "IR-Explain-1", 0);
	    
	if (!zei->categoryList->dirty)
	{
	    struct zebraCategoryListInfo *zcl = zei->categoryList;
	    data1_node *node_cl;
	    
	    zcl->dirty = 1;
	    zcl->data1_categoryList =
		data1_read_sgml (zei->dh, zei->nmem,
				 "<explain><categoryList>CategoryList\n"
				 "</></>\n");
	
	    if (zcl->data1_categoryList)
	    {
		assert (zcl->data1_categoryList->child);
		node_cl = data1_search_tag (zei->dh,
					    zcl->data1_categoryList->child,
					    "categoryList");
		assert (node_cl);
		zebraExplain_initCommonInfo (zei, node_cl);
	    }
	}
    }
    return zei;
}

static void zebraExplain_readAttributeDetails (ZebraExplainInfo zei,
					       zebAttributeDetails zad)
{
    Record rec;
    struct zebSUInfoB **zsuip = &zad->SUInfo;
    data1_node *node_adinfo, *node_zebra, *node_list, *np;

    assert (zad->sysno);
    rec = rec_get (zei->records, zad->sysno);

    zad->data1_tree = read_sgml_rec (zei->dh, zei->nmem, rec);

    node_adinfo = data1_search_tag (zei->dh, zad->data1_tree->child,
				    "attributeDetails");
    node_zebra = data1_search_tag (zei->dh, node_adinfo->child,
				 "zebraInfo");
    node_list = data1_search_tag (zei->dh, node_zebra->child,
				  "attrlist");
    for (np = node_list->child; np; np = np->next)
    {
	data1_node *node_set = NULL;
	data1_node *node_use = NULL;
	data1_node *node_ordinal = NULL;
	data1_node *np2;
	char oid_str[128];
	int oid_str_len;

	if (np->which != DATA1N_tag || strcmp (np->u.tag.tag, "attr"))
	    continue;
	for (np2 = np->child; np2; np2 = np2->next)
	{
	    if (np2->which != DATA1N_tag || !np2->child ||
		np2->child->which != DATA1N_data)
		continue;
	    if (!strcmp (np2->u.tag.tag, "set"))
		node_set = np2->child;
	    else if (!strcmp (np2->u.tag.tag, "use"))
		node_use = np2->child;
	    else if (!strcmp (np2->u.tag.tag, "ordinal"))
		node_ordinal = np2->child;
	}
	assert (node_set && node_use && node_ordinal);

	oid_str_len = node_set->u.data.len;
	if (oid_str_len >= (int) sizeof(oid_str))
	    oid_str_len = sizeof(oid_str)-1;
	memcpy (oid_str, node_set->u.data.data, oid_str_len);
	oid_str[oid_str_len] = '\0';

        *zsuip = (struct zebSUInfoB *)
	    nmem_malloc (zei->nmem, sizeof(**zsuip));
	(*zsuip)->info.set = oid_getvalbyname (oid_str);

	(*zsuip)->info.use = atoi_n (node_use->u.data.data,
				     node_use->u.data.len);
	(*zsuip)->info.ordinal = atoi_n (node_ordinal->u.data.data,
					 node_ordinal->u.data.len);
	logf (LOG_DEBUG, "set=%d use=%d ordinal=%d",
	      (*zsuip)->info.set, (*zsuip)->info.use, (*zsuip)->info.ordinal);
        zsuip = &(*zsuip)->next;
    }
    *zsuip = NULL;
    zad->readFlag = 0;
    rec_rm (&rec);
}

static void zebraExplain_readDatabase (ZebraExplainInfo zei,
				       struct zebDatabaseInfoB *zdi)
{
    Record rec;
    data1_node *node_dbinfo, *node_zebra, *np;

    assert (zdi->sysno);
    rec = rec_get (zei->records, zdi->sysno);

    zdi->data1_database = read_sgml_rec (zei->dh, zei->nmem, rec);
    
    node_dbinfo = data1_search_tag (zei->dh, zdi->data1_database->child,
				   "databaseInfo");
    zebraExplain_mergeAccessInfo (zei, node_dbinfo, &zdi->accessInfo);

    node_zebra = data1_search_tag (zei->dh, node_dbinfo->child,
				 "zebraInfo");
    if (node_zebra
	&& (np = data1_search_tag (zei->dh, node_zebra->child,
				   "recordBytes")) 
	&& np->child && np->child->which == DATA1N_data)
	zdi->recordBytes = atoi_n (np->child->u.data.data,
				   np->child->u.data.len);
    if ((np = data1_search_tag (zei->dh, node_dbinfo->child,
				"recordCount")) &&
	(np = data1_search_tag (zei->dh, np->child,
				"recordCountActual")) &&
	np->child->which == DATA1N_data)
    {
	zdi->recordCount = atoi_n (np->child->u.data.data,
				   np->child->u.data.len);
    }    
    zdi->readFlag = 0;
    rec_rm (&rec);
}

int zebraExplain_curDatabase (ZebraExplainInfo zei, const char *database)
{
    struct zebDatabaseInfoB *zdi;
    
    assert (zei);
    if (zei->curDatabaseInfo &&
        !strcmp (zei->curDatabaseInfo->databaseName, database))
        return 0;
    for (zdi = zei->databaseInfo; zdi; zdi=zdi->next)
    {
        if (!strcmp (zdi->databaseName, database))
            break;
    }
    if (!zdi)
        return -1;
#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_curDatabase: %s", database);
#endif
    if (zdi->readFlag)
    {
#if ZINFO_DEBUG
	logf (LOG_LOG, "zebraExplain_readDatabase: %s", database);
#endif
        zebraExplain_readDatabase (zei, zdi);
    }
    if (zdi->attributeDetails->readFlag)
    {
#if ZINFO_DEBUG
	logf (LOG_LOG, "zebraExplain_readAttributeDetails: %s", database);
#endif
        zebraExplain_readAttributeDetails (zei, zdi->attributeDetails);
    }
    zei->curDatabaseInfo = zdi;
    return 0;
}

static void zebraExplain_initCommonInfo (ZebraExplainInfo zei, data1_node *n)
{
    data1_node *c = data1_add_tag (zei->dh, n, "commonInfo", zei->nmem);

    data1_add_tagdata_text (zei->dh, c, "dateAdded", zei->date, zei->nmem);
    data1_add_tagdata_text (zei->dh, c, "dateChanged", zei->date, zei->nmem);
    data1_add_tagdata_text (zei->dh, c, "languageCode", "EN", zei->nmem);
}

static void zebraExplain_updateCommonInfo (ZebraExplainInfo zei, data1_node *n)
{
    data1_node *c = data1_search_tag (zei->dh, n->child, "commonInfo");
    assert (c);
    data1_make_tagdata_text (zei->dh, c, "dateChanged", zei->date, zei->nmem);
}

static void zebraExplain_initAccessInfo (ZebraExplainInfo zei, data1_node *n)
{
    data1_node *c = data1_add_tag (zei->dh, n, "accessInfo", zei->nmem);
    data1_node *d = data1_add_tag (zei->dh, c, "unitSystems", zei->nmem);
    data1_add_tagdata_text (zei->dh, d, "string", "ISO", zei->nmem);
}

static void zebraExplain_updateAccessInfo (ZebraExplainInfo zei, data1_node *n,
					   zebAccessInfo accessInfo)
{
    data1_node *c = data1_search_tag (zei->dh, n->child, "accessInfo");
    data1_node *d;
    zebAccessObject p;
    
    assert (c);

    if ((p = accessInfo->attributeSetIds))
    {
	d = data1_make_tag (zei->dh, c, "attributeSetIds", zei->nmem);
	for (; p; p = p->next)
	    data1_add_tagdata_oid (zei->dh, d, "oid", p->oid, zei->nmem);
    }
    if ((p = accessInfo->schemas))
    {
	d = data1_make_tag (zei->dh, c, "schemas", zei->nmem);
	for (; p; p = p->next)
	    data1_add_tagdata_oid (zei->dh, d, "oid", p->oid, zei->nmem);
    }
}

int zebraExplain_newDatabase (ZebraExplainInfo zei, const char *database,
			      int explain_database)
{
    struct zebDatabaseInfoB *zdi;
    data1_node *node_dbinfo, *node_adinfo;

#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_newDatabase: %s", database);
#endif
    assert (zei);
    for (zdi = zei->databaseInfo; zdi; zdi=zdi->next)
    {
        if (!strcmp (zdi->databaseName, database))
            break;
    }
    if (zdi)
        return -1;
    /* it's new really. make it */
    zdi = (struct zebDatabaseInfoB *) nmem_malloc (zei->nmem, sizeof(*zdi));
    zdi->next = zei->databaseInfo;
    zei->databaseInfo = zdi;
    zdi->sysno = 0;
    zdi->recordCount = 0;
    zdi->recordBytes = 0;
    zdi->readFlag = 0;
    zdi->databaseName = nmem_strdup (zei->nmem, database);

    zebraExplain_mergeAccessInfo (zei, 0, &zdi->accessInfo);
    
    assert (zei->dh);
    assert (zei->nmem);

    zdi->data1_database =
	data1_read_sgml (zei->dh, zei->nmem, 
			 "<explain><databaseInfo>DatabaseInfo\n"
			 "</></>\n");
    if (!zdi->data1_database)
	return -2;
    
    node_dbinfo = data1_search_tag (zei->dh, zdi->data1_database->child,
				   "databaseInfo");
    assert (node_dbinfo);

    zebraExplain_initCommonInfo (zei, node_dbinfo);
    zebraExplain_initAccessInfo (zei, node_dbinfo);

    data1_add_tagdata_text (zei->dh, node_dbinfo, "name",
			       database, zei->nmem);
    
    if (explain_database)
	data1_add_tagdata_text (zei->dh, node_dbinfo, "explainDatabase",
				"", zei->nmem);
    
    data1_add_tagdata_text (zei->dh, node_dbinfo, "userFee",
			    "0", zei->nmem);
    
    data1_add_tagdata_text (zei->dh, node_dbinfo, "available",
			    "1", zei->nmem);
    
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zdi->data1_database, stderr);
#endif
    zdi->dirty = 1;
    zei->dirty = 1;
    zei->curDatabaseInfo = zdi;

    zdi->attributeDetails = (zebAttributeDetails)
	nmem_malloc (zei->nmem, sizeof(*zdi->attributeDetails));
    zdi->attributeDetails->readFlag = 0;
    zdi->attributeDetails->sysno = 0;
    zdi->attributeDetails->dirty = 1;
    zdi->attributeDetails->SUInfo = NULL;
    zdi->attributeDetails->data1_tree =
	data1_read_sgml (zei->dh, zei->nmem,
			 "<explain><attributeDetails>AttributeDetails\n"
			 "</></>\n");

    node_adinfo =
	data1_search_tag (zei->dh, zdi->attributeDetails->data1_tree->child,
			  "attributeDetails");
    assert (node_adinfo);

    zebraExplain_initCommonInfo (zei, node_adinfo);

    return 0;
}

static void writeAttributeValueDetails (ZebraExplainInfo zei,
				  zebAttributeDetails zad,
				  data1_node *node_atvs, data1_attset *attset)

{
    struct zebSUInfoB *zsui;
    int set_ordinal = attset->reference;
    data1_attset_child *c;

    for (c = attset->children; c; c = c->next)
	writeAttributeValueDetails (zei, zad, node_atvs, c->child);
    for (zsui = zad->SUInfo; zsui; zsui = zsui->next)
    {
	data1_node *node_attvalue, *node_value;
	if (set_ordinal != zsui->info.set)
	    continue;
	node_attvalue = data1_add_tag (zei->dh, node_atvs, "attributeValue",
				       zei->nmem);
	node_value = data1_add_tag (zei->dh, node_attvalue, "value",
				    zei->nmem);
	data1_add_tagdata_int (zei->dh, node_value, "numeric",
			       zsui->info.use, zei->nmem);
    }
}

static void zebraExplain_writeCategoryList (ZebraExplainInfo zei,
					    struct zebraCategoryListInfo *zcl,
					    int key_flush)
{
    char *sgml_buf;
    int sgml_len;
    int i;
    Record drec;
    data1_node *node_ci, *node_categoryList;
    int sysno = 0;
    static char *category[] = {
	"CategoryList",
	"TargetInfo",
	"DatabaseInfo",
	"AttributeDetails",
	NULL
    };

    assert (zcl);
    if (!zcl->dirty)
	return ;
    zcl->dirty = 1;
    node_categoryList = zcl->data1_categoryList;

#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_writeCategoryList");
#endif

    drec = createRecord (zei->records, &sysno);

    node_ci = data1_search_tag (zei->dh, node_categoryList->child,
				"categoryList");
    assert (node_ci);
    node_ci = data1_add_tag (zei->dh, node_ci, "categories", zei->nmem);
    assert (node_ci);
    
    for (i = 0; category[i]; i++)
    {
	data1_node *node_cat = data1_add_tag (zei->dh, node_ci, 
					      "category", zei->nmem);

	data1_add_tagdata_text (zei->dh, node_cat, "name",
				category[i], zei->nmem);
    }
    /* extract *searchable* keys from it. We do this here, because
       record count, etc. is affected */
    if (key_flush)
	(*zei->updateFunc)(zei->updateHandle, drec, node_categoryList);

    /* convert to "SGML" and write it */
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, node_categoryList, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, node_categoryList, 0, &sgml_len);
    drec->info[recInfo_storeData] = (char *) xmalloc (sgml_len);
    memcpy (drec->info[recInfo_storeData], sgml_buf, sgml_len);
    drec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &drec);
}

static void zebraExplain_writeAttributeDetails (ZebraExplainInfo zei,
						zebAttributeDetails zad,
						const char *databaseName,
						int key_flush)
{
    char *sgml_buf;
    int sgml_len;
    Record drec;
    data1_node *node_adinfo, *node_list, *node_zebra, *node_attributesBySet;
    struct zebSUInfoB *zsui;
    int set_min;
    
    if (!zad->dirty)
	return;
    
    zad->dirty = 0;
#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_writeAttributeDetails");    
#endif

    drec = createRecord (zei->records, &zad->sysno);
    assert (zad->data1_tree);
    node_adinfo = data1_search_tag (zei->dh, zad->data1_tree->child,
				   "attributeDetails");
    zebraExplain_updateCommonInfo (zei, node_adinfo);

    data1_add_tagdata_text (zei->dh, node_adinfo, "name",
			    databaseName, zei->nmem);

    /* extract *searchable* keys from it. We do this here, because
       record count, etc. is affected */
    if (key_flush)
	(*zei->updateFunc)(zei->updateHandle, drec, zad->data1_tree);

    node_attributesBySet = data1_make_tag (zei->dh, node_adinfo,
					   "attributesBySet", zei->nmem);
    set_min = -1;
    while (1)
    {
	data1_node *node_asd;
	data1_attset *attset;
	int set_ordinal = -1;
	for (zsui = zad->SUInfo; zsui; zsui = zsui->next)
	{
	    if ((set_ordinal < 0 || set_ordinal > zsui->info.set)
		&& zsui->info.set > set_min)
		set_ordinal = zsui->info.set;
	}
	if (set_ordinal < 0)
	    break;
	set_min = set_ordinal;
	node_asd = data1_add_tag (zei->dh, node_attributesBySet,
				  "attributeSetDetails", zei->nmem);

	attset = data1_attset_search_id (zei->dh, set_ordinal);
	if (!attset)
	{
	    zebraExplain_loadAttsets (zei->dh, zei->res);
	    attset = data1_attset_search_id (zei->dh, set_ordinal);
	}
	if (attset)
	{
	    int oid[OID_SIZE];
	    oident oe;
	    
	    oe.proto = PROTO_Z3950;
	    oe.oclass = CLASS_ATTSET;
	    oe.value = (enum oid_value) set_ordinal;
	    
	    if (oid_ent_to_oid (&oe, oid))
	    {
		data1_node *node_abt, *node_atd, *node_atvs;
		data1_add_tagdata_oid (zei->dh, node_asd, "oid",
				       oid, zei->nmem);
		
		node_abt = data1_add_tag (zei->dh, node_asd,
					  "attributesByType", zei->nmem);
		node_atd = data1_add_tag (zei->dh, node_abt,
					  "attributeTypeDetails", zei->nmem);
		data1_add_tagdata_int (zei->dh, node_atd,
				       "type", 1, zei->nmem);
		node_atvs = data1_add_tag (zei->dh, node_atd, 
					   "attributeValues", zei->nmem);
		writeAttributeValueDetails (zei, zad, node_atvs, attset);
	    }
	}
    }
    /* zebra info (private) */
    node_zebra = data1_make_tag (zei->dh, node_adinfo,
				 "zebraInfo", zei->nmem);
    node_list = data1_make_tag (zei->dh, node_zebra,
				 "attrlist", zei->nmem);
    for (zsui = zad->SUInfo; zsui; zsui = zsui->next)
    {
	struct oident oident;
	int oid[OID_SIZE];
	data1_node *node_attr;
	
	node_attr = data1_add_tag (zei->dh, node_list, "attr", zei->nmem);
	
	oident.proto = PROTO_Z3950;
	oident.oclass = CLASS_ATTSET;
	oident.value = (enum oid_value) zsui->info.set;
	oid_ent_to_oid (&oident, oid);
	
	data1_add_tagdata_text (zei->dh, node_attr, "set",
				oident.desc, zei->nmem);
	data1_add_tagdata_int (zei->dh, node_attr, "use",
			       zsui->info.use, zei->nmem);
	data1_add_tagdata_int (zei->dh, node_attr, "ordinal",
			       zsui->info.ordinal, zei->nmem);
    }
    /* convert to "SGML" and write it */
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zad->data1_tree, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, zad->data1_tree,
				  0, &sgml_len);
    drec->info[recInfo_storeData] = (char *) xmalloc (sgml_len);
    memcpy (drec->info[recInfo_storeData], sgml_buf, sgml_len);
    drec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &drec);
}

static void zebraExplain_writeDatabase (ZebraExplainInfo zei,
                                        struct zebDatabaseInfoB *zdi,
					int key_flush)
{
    char *sgml_buf;
    int sgml_len;
    Record drec;
    data1_node *node_dbinfo, *node_count, *node_zebra;
    
    if (!zdi->dirty)
	return;

    zdi->dirty = 0;
#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_writeDatabase %s", zdi->databaseName);
#endif
    drec = createRecord (zei->records, &zdi->sysno);
    assert (zdi->data1_database);
    node_dbinfo = data1_search_tag (zei->dh, zdi->data1_database->child,
				   "databaseInfo");

    zebraExplain_updateCommonInfo (zei, node_dbinfo);
    zebraExplain_updateAccessInfo (zei, node_dbinfo, zdi->accessInfo);

    /* extract *searchable* keys from it. We do this here, because
       record count, etc. is affected */
    if (key_flush)
	(*zei->updateFunc)(zei->updateHandle, drec, zdi->data1_database);
    /* record count */
    node_count = data1_make_tag (zei->dh, node_dbinfo,
				 "recordCount", zei->nmem);
    data1_add_tagdata_int (zei->dh, node_count, "recordCountActual",
			      zdi->recordCount, zei->nmem);

    /* zebra info (private) */
    node_zebra = data1_make_tag (zei->dh, node_dbinfo,
				 "zebraInfo", zei->nmem);
    data1_add_tagdata_int (zei->dh, node_zebra,
			   "recordBytes", zdi->recordBytes, zei->nmem);
    /* convert to "SGML" and write it */
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zdi->data1_database, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, zdi->data1_database,
				  0, &sgml_len);
    drec->info[recInfo_storeData] = (char *) xmalloc (sgml_len);
    memcpy (drec->info[recInfo_storeData], sgml_buf, sgml_len);
    drec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &drec);
}

static void writeAttributeValues (ZebraExplainInfo zei,
				  data1_node *node_values,
				  data1_attset *attset)
{
    data1_att *atts;
    data1_attset_child *c;

    if (!attset)
	return;

    for (c = attset->children; c; c = c->next)
	writeAttributeValues (zei, node_values, c->child);
    for (atts = attset->atts; atts; atts = atts->next)
    {
	data1_node *node_value;
	
	node_value = data1_add_tag (zei->dh, node_values, "attributeValue",
				    zei->nmem);
	data1_add_tagdata_text (zei->dh, node_value, "name",
				atts->name, zei->nmem);
        node_value = data1_add_tag (zei->dh, node_value, "value", zei->nmem);
	data1_add_tagdata_int (zei->dh, node_value, "numeric",
			       atts->value, zei->nmem);
    }
}


static void zebraExplain_writeAttributeSet (ZebraExplainInfo zei,
					    zebAccessObject o,
					    int key_flush)
{
    char *sgml_buf;
    int sgml_len;
    Record drec;
    data1_node *node_root, *node_attinfo, *node_attributes, *node_atttype;
    data1_node *node_values;
    struct oident *entp;
    struct data1_attset *attset = NULL;
    
    if ((entp = oid_getentbyoid (o->oid)))
	attset = data1_attset_search_id (zei->dh, entp->value);
	    
#if ZINFO_DEBUG
    logf (LOG_LOG, "zebraExplain_writeAttributeSet %s",
	  attset ? attset->name : "<unknown>");    
#endif

    drec = createRecord (zei->records, &o->sysno);
    node_root =
	data1_read_sgml (zei->dh, zei->nmem,
			 "<explain><attributeSetInfo>AttributeSetInfo\n"
			 "</></>\n" );

    node_attinfo = data1_search_tag (zei->dh, node_root->child,
				   "attributeSetInfo");

    zebraExplain_initCommonInfo (zei, node_attinfo);
    zebraExplain_updateCommonInfo (zei, node_attinfo);

    data1_add_tagdata_oid (zei->dh, node_attinfo,
			    "oid", o->oid, zei->nmem);
    if (attset && attset->name)
	data1_add_tagdata_text (zei->dh, node_attinfo,
				"name", attset->name, zei->nmem);
    
    node_attributes = data1_make_tag (zei->dh, node_attinfo,
				      "attributes", zei->nmem);
    node_atttype = data1_make_tag (zei->dh, node_attributes,
				   "attributeType", zei->nmem);
    data1_add_tagdata_text (zei->dh, node_atttype,
			    "name", "Use", zei->nmem);
    data1_add_tagdata_text (zei->dh, node_atttype,
			    "description", "Use Attribute", zei->nmem);
    data1_add_tagdata_int (zei->dh, node_atttype,
			   "type", 1, zei->nmem);
    node_values = data1_add_tag (zei->dh, node_atttype,
				 "attributeValues", zei->nmem);
    if (attset)
	writeAttributeValues (zei, node_values, attset);

    /* extract *searchable* keys from it. We do this here, because
       record count, etc. is affected */
    if (key_flush)
	(*zei->updateFunc)(zei->updateHandle, drec, node_root);
    /* convert to "SGML" and write it */
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, node_root, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, node_root, 0, &sgml_len);
    drec->info[recInfo_storeData] = (char *) xmalloc (sgml_len);
    memcpy (drec->info[recInfo_storeData], sgml_buf, sgml_len);
    drec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &drec);
}

static void zebraExplain_writeTarget (ZebraExplainInfo zei, int key_flush)
{
    struct zebDatabaseInfoB *zdi;
    data1_node *node_tgtinfo, *node_list, *node_zebra;
    Record trec;
    int sgml_len;
    char *sgml_buf;

    if (!zei->dirty)
	return;
    zei->dirty = 0;

    trec = rec_get (zei->records, 1);
    xfree (trec->info[recInfo_storeData]);

    node_tgtinfo = data1_search_tag (zei->dh, zei->data1_target->child,
				   "targetInfo");
    assert (node_tgtinfo);

    zebraExplain_updateCommonInfo (zei, node_tgtinfo);
    zebraExplain_updateAccessInfo (zei, node_tgtinfo, zei->accessInfo);

    /* convert to "SGML" and write it */
    if (key_flush)
	(*zei->updateFunc)(zei->updateHandle, trec, zei->data1_target);

    node_zebra = data1_make_tag (zei->dh, node_tgtinfo,
				 "zebraInfo", zei->nmem);
    data1_add_tagdata_text (zei->dh, node_zebra, "version",
			       ZEBRAVER, zei->nmem);
    node_list = data1_add_tag (zei->dh, node_zebra,
				  "databaseList", zei->nmem);
    for (zdi = zei->databaseInfo; zdi; zdi = zdi->next)
    {
	data1_node *node_db;
	node_db = data1_add_tag (zei->dh, node_list,
				    "database", zei->nmem);
	data1_add_tagdata_text (zei->dh, node_db, "name",
				   zdi->databaseName, zei->nmem);
	data1_add_tagdata_int (zei->dh, node_db, "id",
				  zdi->sysno, zei->nmem);
	data1_add_tagdata_int (zei->dh, node_db, "attributeDetailsId",
				  zdi->attributeDetails->sysno, zei->nmem);
    }
    data1_add_tagdata_int (zei->dh, node_zebra, "ordinalSU",
			      zei->ordinalSU, zei->nmem);

    data1_add_tagdata_int (zei->dh, node_zebra, "runNumber",
			      zei->runNumber, zei->nmem);

#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zei->data1_target, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, zei->data1_target,
				  0, &sgml_len);
    trec->info[recInfo_storeData] = (char *) xmalloc (sgml_len);
    memcpy (trec->info[recInfo_storeData], sgml_buf, sgml_len);
    trec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &trec);
}

int zebraExplain_lookupSU (ZebraExplainInfo zei, int set, int use)
{
    struct zebSUInfoB *zsui;

    assert (zei->curDatabaseInfo);
    for (zsui = zei->curDatabaseInfo->attributeDetails->SUInfo;
	 zsui; zsui=zsui->next)
        if (zsui->info.use == use && zsui->info.set == set)
            return zsui->info.ordinal;
    return -1;
}

zebAccessObject zebraExplain_announceOid (ZebraExplainInfo zei,
					  zebAccessObject *op,
					  Odr_oid *oid)
{
    zebAccessObject ao;
    
    for (ao = *op; ao; ao = ao->next)
	if (!oid_oidcmp (oid, ao->oid))
	    break;
    if (!ao)
    {
	ao = (zebAccessObject) nmem_malloc (zei->nmem, sizeof(*ao));
	ao->handle = NULL;
	ao->sysno = 0;
	ao->oid = odr_oiddup_nmem (zei->nmem, oid);
	ao->next = *op;
	*op = ao;
    }
    return ao;
}

void zebraExplain_addAttributeSet (ZebraExplainInfo zei, int set)
{
    oident oe;
    int oid[OID_SIZE];

    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_ATTSET;
    oe.value = (enum oid_value) set;

    if (oid_ent_to_oid (&oe, oid))
    {
	zebraExplain_announceOid (zei, &zei->accessInfo->attributeSetIds, oid);
	zebraExplain_announceOid (zei, &zei->curDatabaseInfo->
				  accessInfo->attributeSetIds, oid);
    }
}

int zebraExplain_addSU (ZebraExplainInfo zei, int set, int use)
{
    struct zebSUInfoB *zsui;

    assert (zei->curDatabaseInfo);
    for (zsui = zei->curDatabaseInfo->attributeDetails->SUInfo;
	 zsui; zsui=zsui->next)
        if (zsui->info.use == use && zsui->info.set == set)
            return -1;
    zebraExplain_addAttributeSet (zei, set);
    zsui = (struct zebSUInfoB *) nmem_malloc (zei->nmem, sizeof(*zsui));
    zsui->next = zei->curDatabaseInfo->attributeDetails->SUInfo;
    zei->curDatabaseInfo->attributeDetails->SUInfo = zsui;
    zei->curDatabaseInfo->attributeDetails->dirty = 1;
    zei->dirty = 1;
    zsui->info.set = set;
    zsui->info.use = use;
    zsui->info.ordinal = (zei->ordinalSU)++;
    return zsui->info.ordinal;
}

void zebraExplain_addSchema (ZebraExplainInfo zei, Odr_oid *oid)
{
    zebraExplain_announceOid (zei, &zei->accessInfo->schemas, oid);
    zebraExplain_announceOid (zei, &zei->curDatabaseInfo->
			      accessInfo->schemas, oid);
}

void zebraExplain_recordBytesIncrement (ZebraExplainInfo zei, int adjust_num)
{
    assert (zei->curDatabaseInfo);

    if (adjust_num)
    {
	zei->curDatabaseInfo->recordBytes += adjust_num;
	zei->curDatabaseInfo->dirty = 1;
    }
}

void zebraExplain_recordCountIncrement (ZebraExplainInfo zei, int adjust_num)
{
    assert (zei->curDatabaseInfo);

    if (adjust_num)
    {
	zei->curDatabaseInfo->recordCount += adjust_num;
	zei->curDatabaseInfo->dirty = 1;
    }
}

int zebraExplain_runNumberIncrement (ZebraExplainInfo zei, int adjust_num)
{
    if (adjust_num)
	zei->dirty = 1;
    return zei->runNumber += adjust_num;
}

RecordAttr *rec_init_attr (ZebraExplainInfo zei, Record rec)
{
    RecordAttr *recordAttr;

    if (rec->info[recInfo_attr])
	return (RecordAttr *) rec->info[recInfo_attr];
    recordAttr = (RecordAttr *) xmalloc (sizeof(*recordAttr));
    rec->info[recInfo_attr] = (char *) recordAttr;
    rec->size[recInfo_attr] = sizeof(*recordAttr);
    
    recordAttr->recordSize = 0;
    recordAttr->recordOffset = 0;
    recordAttr->runNumber = zei->runNumber;
    return recordAttr;
}

static void att_loadset(void *p, const char *n, const char *name)
{
    data1_handle dh = (data1_handle) p;
    if (!data1_get_attset (dh, name))
	logf (LOG_WARN, "Couldn't load attribute set %s", name);
}

void zebraExplain_loadAttsets (data1_handle dh, Res res)
{
    res_trav(res, "attset", dh, att_loadset);
}

/*
     zebraExplain_addSU adds to AttributeDetails for a database and
     adds attributeSet (in AccessInfo area) to DatabaseInfo if it doesn't
     exist for the database.

     If the database doesn't exist globally (in TargetInfo) an 
     AttributeSetInfo must be added (globally).
 */
