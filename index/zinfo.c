/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zinfo.c,v $
 * Revision 1.7  1998-03-05 08:45:13  adam
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

struct zebDatabaseInfoB {
    struct zebSUInfoB *SUInfo;
    char *databaseName;
    data1_node *data1_database;
    int recordCount;     /* records in db */
    int recordBytes;     /* size of records */
    int sysno;           /* sysno of database info */
    int readFlag;        /* 1: read is needed when referenced; 0 if not */
    int dirty;           /* 1: database is dirty: write is needed */
    struct zebDatabaseInfoB *next;
};

struct zebraExplainAttset {
    char *name;
    int ordinal;
    struct zebraExplainAttset *next;
};

struct zebraExplainInfo {
    int  ordinalSU;
    int  runNumber;
    int  dirty;
    Records records;
    data1_handle dh;
    struct zebraExplainAttset *attsets;
    NMEM nmem;
    data1_node *data1_target;
    struct zebDatabaseInfoB *databaseInfo;
    struct zebDatabaseInfoB *curDatabaseInfo;
};

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
    data1_node *res = data1_mk_node (dh, nmem);
    data1_element *e = NULL;

    res->parent = at;
    res->which = DATA1N_tag;
    res->u.tag.tag = data1_insert_string (dh, res, nmem, tag);
    res->u.tag.node_selected = 0;
    res->u.tag.make_variantlist = 0;
    res->u.tag.no_data_requested = 0;
    res->u.tag.get_bytes = -1;
   
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
    data1_node *node;

    node = data1_search_tag (dh, at->child, tag);
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

static data1_node *data1_add_tagdata_text (data1_handle dh, data1_node *at,
					   const char *tag, const char *str,
					   NMEM nmem)
{
    data1_node *node_data;
    
    node_data = data1_add_taggeddata (dh, at->root, at, tag, nmem);
    if (!node_data)
	return 0;
    node_data->u.data.what = DATA1I_text;
    node_data->u.data.data = node_data->lbuf;
    strcpy (node_data->u.data.data, str);
    node_data->u.data.len = strlen (node_data->u.data.data);
    return node_data;
}

static void zebraExplain_writeDatabase (ZebraExplainInfo zei,
                                        struct zebDatabaseInfoB *zdi);
static void zebraExplain_writeTarget (ZebraExplainInfo zei);

void zebraExplain_close (ZebraExplainInfo zei, int writeFlag)
{
    struct zebDatabaseInfoB *zdi, *zdi_next;
    
    logf (LOG_DEBUG, "zebraExplain_close wr=%d", writeFlag);
    if (writeFlag)
    {
	/* write each database info record */
	for (zdi = zei->databaseInfo; zdi; zdi = zdi->next)
	    zebraExplain_writeDatabase (zei, zdi);
	zebraExplain_writeTarget (zei);
    }
    for (zdi = zei->databaseInfo; zdi; zdi = zdi_next)
    {
        struct zebSUInfoB *zsui, *zsui_next;

        zdi_next = zdi->next;
        for (zsui = zdi->SUInfo; zsui; zsui = zsui_next)
        {
            zsui_next = zsui->next;
            xfree (zsui);
        }
        xfree (zdi);
    }
    nmem_destroy (zei->nmem);
    xfree (zei);
}


ZebraExplainInfo zebraExplain_open (Records records, data1_handle dh,
				    int writeFlag)
{
    Record trec;
    ZebraExplainInfo zei;
    struct zebDatabaseInfoB **zdip;

    logf (LOG_DEBUG, "zebraExplain_open wr=%d", writeFlag);
    zei = xmalloc (sizeof(*zei));
    zei->dirty = 0;
    zei->curDatabaseInfo = NULL;
    zei->records = records;
    zei->nmem = nmem_create ();
    zei->dh = dh;
    zei->attsets = NULL;
    zdip = &zei->databaseInfo;
    trec = rec_get (records, 1);

    if (trec)
    {
	data1_node *node_tgtinfo, *node_zebra, *node_list, *np;

	zei->data1_target = read_sgml_rec (zei->dh, zei->nmem, trec);

#if ZINFO_DEBUG
	data1_pr_tree (zei->dh, zei->data1_target, stderr);
#endif
	node_tgtinfo = data1_search_tag (zei->dh, zei->data1_target->child,
					 "targetInfo");
	node_zebra = data1_search_tag (zei->dh, node_tgtinfo->child,
				       "zebraInfo");
	node_list = data1_search_tag (zei->dh, node_zebra->child,
				      "databaseList");
	for (np = node_list->child; np; np = np->next)
	{
	    data1_node *node_name = NULL;
	    data1_node *node_id = NULL;
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
	    }
	    assert (node_id && node_name);
	    
	    *zdip = xmalloc (sizeof(**zdip));

            (*zdip)->readFlag = 1;
            (*zdip)->dirty = 0;
	    (*zdip)->data1_database = NULL;
	    (*zdip)->recordCount = 0;
	    (*zdip)->recordBytes = 0;
	    (*zdip)->SUInfo = NULL;

	    (*zdip)->databaseName = nmem_malloc (zei->nmem,
						 1+node_name->u.data.len);
	    memcpy ((*zdip)->databaseName, node_name->u.data.data,
		    node_name->u.data.len);
	    (*zdip)->databaseName[node_name->u.data.len] = '\0';
	    (*zdip)->sysno = atoi_n (node_id->u.data.data,
				     node_id->u.data.len);
	    zdip = &(*zdip)->next;
	}
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
    }
    else
    {
        zei->ordinalSU = 1;
	zei->runNumber = 0;
	if (writeFlag)
	{
	    char *sgml_buf;
	    int sgml_len;

	    zei->data1_target =
		data1_read_sgml (zei->dh, zei->nmem,
				 "<explain><targetInfo>targetInfo\n"
				 "<name>Zebra</>\n"
				 "<namedResultSets>1</>\n"
				 "<multipleDBSearch>1</>\n"
				 "<nicknames><name>Zebra</></>\n"
				 "</></>\n" );
	    /* write now because we want to be sure about the sysno */
	    trec = rec_new (records);
	    trec->info[recInfo_fileType] =
		rec_strdup ("grs.sgml", &trec->size[recInfo_fileType]);
	    trec->info[recInfo_databaseName] =
		rec_strdup ("IR-Explain-1", &trec->size[recInfo_databaseName]);
	    
	    sgml_buf = data1_nodetoidsgml(dh, zei->data1_target, 0, &sgml_len);
	    trec->info[recInfo_storeData] = xmalloc (sgml_len);
	    memcpy (trec->info[recInfo_storeData], sgml_buf, sgml_len);
	    trec->size[recInfo_storeData] = sgml_len;
	    
	    rec_put (records, &trec);
	}
    }
    *zdip = NULL;
    rec_rm (&trec);
    zebraExplain_newDatabase (zei, "IR-Explain-1");
    return zei;
}


static void zebraExplain_readDatabase (ZebraExplainInfo zei,
				       struct zebDatabaseInfoB *zdi)
{
    Record rec;
    data1_node *node_dbinfo, *node_zebra, *node_list, *np;
    struct zebSUInfoB **zsuip = &zdi->SUInfo;

    assert (zdi->sysno);
    rec = rec_get (zei->records, zdi->sysno);

    zdi->data1_database = read_sgml_rec (zei->dh, zei->nmem, rec);
    
    node_dbinfo = data1_search_tag (zei->dh, zdi->data1_database->child,
				   "databaseInfo");

    node_zebra = data1_search_tag (zei->dh, node_dbinfo->child,
				 "zebraInfo");
    np  = data1_search_tag (zei->dh, node_dbinfo->child,
			    "recordBytes");
    if (np && np->child && np->child->which == DATA1N_data)
    {
	zdi->recordBytes = atoi_n (np->child->u.data.data,
				   np->child->u.data.len);
    }
    node_list = data1_search_tag (zei->dh, node_zebra->child,
				 "attrlist");
    for (np = node_list->child; np; np = np->next)
    {
	data1_node *node_set = NULL;
	data1_node *node_use = NULL;
	data1_node *node_ordinal = NULL;
	data1_node *np2;
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
	
        *zsuip = xmalloc (sizeof(**zsuip));
	(*zsuip)->info.set = atoi_n (node_set->u.data.data,
				     node_set->u.data.len);
	(*zsuip)->info.use = atoi_n (node_use->u.data.data,
				     node_use->u.data.len);
	(*zsuip)->info.ordinal = atoi_n (node_ordinal->u.data.data,
					 node_ordinal->u.data.len);
	logf (LOG_DEBUG, "set=%d use=%d ordinal=%d",
	      (*zsuip)->info.set, (*zsuip)->info.use, (*zsuip)->info.ordinal);
        zsuip = &(*zsuip)->next;
    }
    *zsuip = NULL;

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
    if (zdi->readFlag)
        zebraExplain_readDatabase (zei, zdi);
    zei->curDatabaseInfo = zdi;
    return 0;
}

int zebraExplain_newDatabase (ZebraExplainInfo zei, const char *database)
{
    struct zebDatabaseInfoB *zdi;
    data1_node *node_dbinfo;

    assert (zei);
    for (zdi = zei->databaseInfo; zdi; zdi=zdi->next)
    {
        if (!strcmp (zdi->databaseName, database))
            break;
    }
    if (zdi)
        return -1;
    /* it's new really. make it */
    zdi = xmalloc (sizeof(*zdi));
    zdi->next = zei->databaseInfo;
    zei->databaseInfo = zdi;
    zdi->sysno = 0;
    zdi->recordCount = 0;
    zdi->recordBytes = 0;
    zdi->readFlag = 0;
    zdi->databaseName = nmem_strdup (zei->nmem, database);
    zdi->SUInfo = NULL;
    
    assert (zei->dh);
    assert (zei->nmem);

    zdi->data1_database =
	data1_read_sgml (zei->dh, zei->nmem, 
			 "<explain><databaseInfo>databaseInfo\n"
			 "<userFee>0</>\n"
			 "<available>1</>\n"
			 "</></>\n");
    
    node_dbinfo = data1_search_tag (zei->dh, zdi->data1_database->child,
				   "databaseInfo");
    assert (node_dbinfo);

    data1_add_tagdata_text (zei->dh, node_dbinfo, "name",
			       database, zei->nmem);

#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zdi->data1_database, stderr);
#endif
    zdi->dirty = 1;
    zei->dirty = 1;
    zei->curDatabaseInfo = zdi;
    return 0;
}

static void zebraExplain_writeDatabase (ZebraExplainInfo zei,
                                        struct zebDatabaseInfoB *zdi)
{
    char *sgml_buf;
    int sgml_len;
    Record drec;
    data1_node *node_dbinfo, *node_list, *node_count, *node_zebra;
    struct zebSUInfoB *zsui;
    
    if (!zdi->dirty)
	return;
    
    if (zdi->sysno)
    {
	drec = rec_get (zei->records, zdi->sysno);
	xfree (drec->info[recInfo_storeData]);
    }
    else
    {
	drec = rec_new (zei->records);
	zdi->sysno = drec->sysno;
	
	drec->info[recInfo_fileType] =
	    rec_strdup ("grs.sgml", &drec->size[recInfo_fileType]);
	drec->info[recInfo_databaseName] =
	    rec_strdup ("IR-Explain-1",
			&drec->size[recInfo_databaseName]); 
    }
    assert (zdi->data1_database);
    node_dbinfo = data1_search_tag (zei->dh, zdi->data1_database->child,
				   "databaseInfo");
    /* record count */
    node_count = data1_make_tag (zei->dh, node_dbinfo,
				 "recordCount", zei->nmem);
    data1_add_tagdata_int (zei->dh, node_count, "recordCountActual",
			      zdi->recordCount, zei->nmem);

    /* zebra info (private) */
    node_zebra = data1_make_tag (zei->dh, node_dbinfo,
				 "zebraInfo", zei->nmem);
    node_list = data1_make_tag (zei->dh, node_zebra,
				 "attrlist", zei->nmem);
    for (zsui = zdi->SUInfo; zsui; zsui = zsui->next)
    {
	data1_node *node_attr;
	node_attr = data1_add_tag (zei->dh, node_list,
				      "attr", zei->nmem);
	data1_add_tagdata_int (zei->dh, node_attr, "set",
				  zsui->info.set, zei->nmem);
	data1_add_tagdata_int (zei->dh, node_attr, "use",
				  zsui->info.use, zei->nmem);
	data1_add_tagdata_int (zei->dh, node_attr, "ordinal",
				  zsui->info.ordinal, zei->nmem);
    }
    data1_add_tagdata_int (zei->dh, node_zebra,
			   "recordBytes", zdi->recordBytes, zei->nmem);
    /* convert to "SGML" and write it */
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zdi->data1_database, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, zdi->data1_database,
				  0, &sgml_len);
    drec->info[recInfo_storeData] = xmalloc (sgml_len);
    memcpy (drec->info[recInfo_storeData], sgml_buf, sgml_len);
    drec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &drec);
}

static void trav_attset (data1_handle dh, ZebraExplainInfo zei,
			 data1_attset *p_this)
{
    struct zebraExplainAttset *p_reg = zei->attsets;

    if (!p_this)
	return ;
    while (p_reg)
    {
	if (!strcmp (p_this->name, p_reg->name))
	    break;
	p_reg = p_reg->next;
    }
    if (!p_this)
    {
	p_reg = nmem_malloc (zei->nmem, sizeof (*p_reg));
	p_reg->name = nmem_strdup (zei->nmem, p_this->name);
	p_reg->ordinal = p_this->ordinal;
	p_reg->next = zei->attsets;
	zei->attsets = p_reg;
    }
    trav_attset (dh, zei, p_this->children);
}

static void trav_absyn (data1_handle dh, void *h, data1_absyn *a)
{
    logf (LOG_LOG, "absyn %s", a->name);
    trav_attset (dh, (ZebraExplainInfo) h, a->attset);
}

static void zebraExplain_writeTarget (ZebraExplainInfo zei)
{
    struct zebDatabaseInfoB *zdi;
    data1_node *node_tgtinfo, *node_list, *node_zebra;
    Record trec;
    int sgml_len;
    char *sgml_buf;

    if (!zei->dirty)
	return;

    trec = rec_get (zei->records, 1);
    xfree (trec->info[recInfo_storeData]);

    node_tgtinfo = data1_search_tag (zei->dh, zei->data1_target->child,
				   "targetInfo");
    assert (node_tgtinfo);

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
    }
    data1_add_tagdata_int (zei->dh, node_zebra, "ordinalSU",
			      zei->ordinalSU, zei->nmem);

    data1_add_tagdata_int (zei->dh, node_zebra, "runNumber",
			      zei->runNumber, zei->nmem);

    node_list = data1_add_tag (zei->dh, node_zebra,
			       "attsetList", zei->nmem);
    /* convert to "SGML" and write it */
#if ZINFO_DEBUG
    data1_pr_tree (zei->dh, zei->data1_target, stderr);
#endif
    sgml_buf = data1_nodetoidsgml(zei->dh, zei->data1_target,
				  0, &sgml_len);
    trec->info[recInfo_storeData] = xmalloc (sgml_len);
    memcpy (trec->info[recInfo_storeData], sgml_buf, sgml_len);
    trec->size[recInfo_storeData] = sgml_len;
    
    rec_put (zei->records, &trec);
}

int zebraExplain_lookupSU (ZebraExplainInfo zei, int set, int use)
{
    struct zebSUInfoB *zsui;

    assert (zei->curDatabaseInfo);
    for (zsui = zei->curDatabaseInfo->SUInfo; zsui; zsui=zsui->next)
        if (zsui->info.use == use && zsui->info.set == set)
            return zsui->info.ordinal;
    return -1;
}

int zebraExplain_addSU (ZebraExplainInfo zei, int set, int use)
{
    struct zebSUInfoB *zsui;

    assert (zei->curDatabaseInfo);
    for (zsui = zei->curDatabaseInfo->SUInfo; zsui; zsui=zsui->next)
        if (zsui->info.use == use && zsui->info.set == set)
            return -1;
    zsui = xmalloc (sizeof(*zsui));
    zsui->next = zei->curDatabaseInfo->SUInfo;
    zei->curDatabaseInfo->SUInfo = zsui;
    zei->curDatabaseInfo->dirty = 1;
    zei->dirty = 1;
    zsui->info.set = set;
    zsui->info.use = use;
    zsui->info.ordinal = (zei->ordinalSU)++;
    return zsui->info.ordinal;
}

void zebraExplain_recordBytesIncrement (ZebraExplainInfo zei, int adjust_num)
{
    assert (zei->curDatabaseInfo);

    zei->curDatabaseInfo->recordBytes += adjust_num;
    zei->curDatabaseInfo->dirty = 1;
}

void zebraExplain_recordCountIncrement (ZebraExplainInfo zei, int adjust_num)
{
    assert (zei->curDatabaseInfo);

    zei->curDatabaseInfo->recordCount += adjust_num;
    zei->curDatabaseInfo->dirty = 1;
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
    recordAttr = xmalloc (sizeof(*recordAttr));
    rec->info[recInfo_attr] = (char *) recordAttr;
    rec->size[recInfo_attr] = sizeof(*recordAttr);
    
    recordAttr->recordSize = 0;
    recordAttr->recordOffset = 0;
    recordAttr->runNumber = zei->runNumber;
    return recordAttr;
}

