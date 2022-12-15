/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#ifndef DATA1_H
#define DATA1_H

#include <stdio.h>

#include <yaz/nmem.h>
#include <yaz/proto.h>
#include <yaz/yaz-util.h>

#include <idzebra/util.h>

#define d1_isspace(c) strchr(" \r\n\t\f", c)
#define d1_isdigit(c) ((c) <= '9' && (c) >= '0')

YAZ_BEGIN_CDECL

#define data1_matchstr(s1, s2) yaz_matchstr(s1, s2)

#define DATA1_MAX_SYMBOL 31

/*
 * This structure describes a attset, perhaps made up by inclusion
 * (supersetting) of other attribute sets. When indexing and searching,
 * we perform a normalisation, where we associate a given tag with
 * the set that originally defined it, rather than the superset. This
 * allows the most flexible access. Eg, the tags common to GILS and BIB-1
 * should be searchable by both names.
 */

struct data1_attset;

typedef struct data1_attset data1_attset;
typedef struct data1_att data1_att;
typedef struct data1_attset_child data1_attset_child;

struct data1_att
{
    data1_attset *parent;          /* attribute set */
    char *name;                    /* symbolic name of this attribute */
    int value;                     /* attribute value */
    data1_att *next;
};

struct data1_attset_child {
    data1_attset *child;
    data1_attset_child *next;
};

struct data1_attset
{
    char *name;          /* symbolic name */
    Odr_oid *oid;        /* attribute set OID */
    data1_att *atts;          /* attributes */
    data1_attset_child *children;  /* included attset */
    data1_attset *next;       /* next in cache */
};

typedef struct data1_handle_info *data1_handle;

YAZ_EXPORT data1_att *data1_getattbyname(data1_handle dh, data1_attset *s,
                                         const char *name);
YAZ_EXPORT data1_attset *data1_read_attset(data1_handle dh, const char *file);

YAZ_EXPORT data1_attset *data1_empty_attset(data1_handle dh);

typedef struct data1_maptag
{
    int new_field;
    int type;
#define D1_MAPTAG_numeric 1
#define D1_MAPTAG_string 2
    int which;
    union
    {
        int numeric;
        char *string;
    } value;
    struct data1_maptag *next;
} data1_maptag;

typedef struct data1_mapunit data1_mapunit;

typedef struct data1_maptab
{
    char *name;
    Odr_oid *oid;  /* target abstract syntax  */
    char *target_absyn_name;
    data1_mapunit *map;
    struct data1_maptab *next;
} data1_maptab;


typedef struct data1_name
{
    char *name;
    struct data1_name *next;
} data1_name;

typedef struct data1_absyn_cache_info *data1_absyn_cache;
typedef struct data1_attset_cache_info *data1_attset_cache;
typedef struct data1_absyn data1_absyn;

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

typedef struct data1_marctab
{
    char *name;
    Odr_oid *oid; /* MARC OID */

    char record_status[2];
    char implementation_codes[5];
    int  indicator_length;
    int  identifier_length;
    char user_systems[4];

    int  length_data_entry;
    int  length_starting;
    int  length_implementation;
    char future_use[2];

    int  force_indicator_length;
    int  force_identifier_length;
    char leader[24]; /* Fixme! Need linear access to LEADER of MARC record */
    struct data1_marctab *next;
} data1_marctab;

typedef struct data1_esetname
{
    char *name;
    Z_Espec1 *spec;
    struct data1_esetname *next;
} data1_esetname;

/*
 * Variant set definitions.
 */

typedef struct data1_vartype
{
    char *name;
    struct data1_varclass *zclass;
    int type;
    data1_datatype datatype;
    struct data1_vartype *next;
} data1_vartype;

typedef struct data1_varclass
{
    char *name;
    struct data1_varset *set;
    int zclass;
    data1_vartype *types;
    struct data1_varclass *next;
} data1_varclass;

typedef struct data1_varset
{
    char *name;
    Odr_oid *oid; /* variant OID */
    data1_varclass *classes;
} data1_varset;

/*
 * Tagset definitions
 */

struct data1_tagset;

typedef struct data1_tag
{
    data1_name *names;
#define DATA1T_numeric 1
#define DATA1T_string 2
    int which;
    union
    {
        int numeric;
        char *string;
    } value;
    data1_datatype kind;

    struct data1_tagset *tagset;
    struct data1_tag *next;
} data1_tag;

typedef struct data1_tagset data1_tagset;

struct data1_tagset
{
    int type;                        /* type of tagset in current context */
    char *name;                      /* symbolic name */
    Odr_oid *oid;                        /* variant OID */
    data1_tag *tags;                 /* tags defined by this set */
    data1_tagset *children;          /* children */
    data1_tagset *next;              /* sibling */
};

typedef struct data1_termlist
{
    char *index_name;
    char *structure;
    char *source;
    struct data1_termlist *next;
} data1_termlist;

/*
 * abstract syntax specification
 */

typedef struct data1_element
{
    char *name;
    data1_tag *tag;
    data1_termlist *termlists;
    char *sub_name;
    struct data1_element *children;
    struct data1_element *next;
    struct data1_hash_table *hash;
} data1_element;

typedef struct data1_sub_elements {
    char *name;
    struct data1_sub_elements *next;
    data1_element *elements;
} data1_sub_elements;

typedef struct data1_xattr {
    char *name;
    char *value;
    struct data1_xattr *next;
    unsigned short what;  /* DATA1I_text, .. see data1_node.u.data */
} data1_xattr;


/*
 * record data node (tag/data/variant)
 */

typedef struct data1_node
{
    /* the root of a record (containing global data) */
#define DATA1N_root 1
    /* a tag */
#define DATA1N_tag  2
    /* some data under a leaf tag or variant */
#define DATA1N_data 3
    /* variant specification (a triple, actually) */
#define DATA1N_variant 4
    /* comment (same as data) */
#define DATA1N_comment 5
    /* preprocessing instruction */
#define DATA1N_preprocess 6
    int which;
    union
    {
        struct
        {
            char *type;
            struct data1_absyn *absyn;  /* abstract syntax for this type */
        } root;

        struct
        {
            char *tag;
            data1_element *element;
            int no_data_requested;
            int get_bytes;
            unsigned node_selected : 1;
            unsigned make_variantlist : 1;
            data1_xattr *attributes;
        } tag;

        struct
        {
            char *data;      /* filename or data */
            int len;
            /* text inclusion */
#define DATA1I_inctxt 1
            /* binary data inclusion */
#define DATA1I_incbin 2
        /* text data */
#define DATA1I_text 3
            /* numerical data */
#define DATA1I_num 4
            /* object identifier */
#define DATA1I_oid 5
            /* XML text */
#define DATA1I_xmltext 6
            unsigned what:7;
            unsigned formatted_text : 1;   /* newlines are significant */
        } data;

        struct
        {
            data1_vartype *type;
            char *value;
        } variant;

        struct
        {
            char *target;
            data1_xattr *attributes;
        } preprocess;
    } u;

#define DATA1_LOCALDATA 12
    char lbuf[DATA1_LOCALDATA]; /* small buffer for local data */
    struct data1_node *next;
    struct data1_node *child;
    struct data1_node *last_child;
    struct data1_node *parent;
    struct data1_node *root;
} data1_node;

enum DATA1_XPATH_INDEXING {
    DATA1_XPATH_INDEXING_DISABLE,
    DATA1_XPATH_INDEXING_ENABLE
};

YAZ_EXPORT data1_handle data1_create (void);


YAZ_EXPORT void data1_destroy(data1_handle dh);
YAZ_EXPORT data1_node *get_parent_tag(data1_handle dh, data1_node *n);
YAZ_EXPORT data1_node *data1_read_node(data1_handle dh, const char **buf,
                                       NMEM m);
YAZ_EXPORT data1_node *data1_read_nodex (data1_handle dh, NMEM m,
                                         int (*get_byte)(void *fh), void *fh,
                                         WRBUF wrbuf);
YAZ_EXPORT data1_node *data1_read_record(data1_handle dh,
                                         int (*rf)(void *, char *, size_t),
                                         void *fh, NMEM m);

YAZ_EXPORT void data1_remove_node (data1_handle dh, data1_node *n);
YAZ_EXPORT void data1_remove_idzebra_subtree (data1_handle dh, data1_node *n);
YAZ_EXPORT data1_tag *data1_gettagbynum(data1_handle dh,
                                        data1_tagset *s,
                                        int type, int value);
YAZ_EXPORT data1_tagset *data1_empty_tagset (data1_handle dh);
YAZ_EXPORT data1_tagset *data1_read_tagset(data1_handle dh,
                                           const char *file,
                                           int type);
YAZ_EXPORT data1_element *data1_getelementbytagname(data1_handle dh,
                                                    data1_absyn *abs,
                                                    data1_element *parent,
                                                    const char *tagname);
YAZ_EXPORT Z_GenericRecord *data1_nodetogr(data1_handle dh, data1_node *n,
                                           int select, ODR o,
                                           int *len);
YAZ_EXPORT data1_tag *data1_gettagbyname(data1_handle dh, data1_tagset *s,
                                         const char *name);
YAZ_EXPORT char *data1_nodetobuf(data1_handle dh, data1_node *n,
                                 int select, int *len);
YAZ_EXPORT data1_node *data1_mk_tag_data_wd(data1_handle dh,
                                            data1_node *at,
                                            const char *tagname, NMEM m);
YAZ_EXPORT data1_node *data1_mk_tag_data(data1_handle dh, data1_node *at,
                                         const char *tagname, NMEM m);
YAZ_EXPORT data1_datatype data1_maptype(data1_handle dh, char *t);
YAZ_EXPORT data1_varset *data1_read_varset(data1_handle dh, const char *file);
YAZ_EXPORT data1_vartype *data1_getvartypebyct(data1_handle dh,
                                               data1_varset *set,
                                               const char *zclass,
                                               const char *type);
YAZ_EXPORT data1_vartype *data1_getvartypeby_absyn(data1_handle dh,
                                                   data1_absyn *absyn,
                                                   char *zclass, char *type);
YAZ_EXPORT Z_Espec1 *data1_read_espec1(data1_handle dh, const char *file);
YAZ_EXPORT int data1_doespec1(data1_handle dh, data1_node *n, Z_Espec1 *e);
YAZ_EXPORT data1_esetname *data1_getesetbyname(data1_handle dh,
                                               data1_absyn *a,
                                               const char *name);
YAZ_EXPORT data1_element *data1_getelementbyname(data1_handle dh,
                                                 data1_absyn *absyn,
                                                 const char *name);
YAZ_EXPORT data1_node *data1_mk_node2(data1_handle dh, NMEM m,
                                      int type, data1_node *parent);

YAZ_EXPORT data1_node *data1_mk_tag (data1_handle dh, NMEM nmem,
                                     const char *tag, const char **attr,
                                     data1_node *at);
YAZ_EXPORT data1_node *data1_mk_tag_n (data1_handle dh, NMEM nmem,
                                       const char *tag, size_t len,
                                       const char **attr,
                                       data1_node *at);
YAZ_EXPORT void data1_tag_add_attr (data1_handle dh, NMEM nmem,
                                    data1_node *res, const char **attr);

YAZ_EXPORT data1_node *data1_mk_text_n (data1_handle dh, NMEM mem,
                                        const char *buf, size_t len,
                                        data1_node *parent);
YAZ_EXPORT data1_node *data1_mk_text_nf (data1_handle dh, NMEM mem,
                                         const char *buf, size_t len,
                                         data1_node *parent);
YAZ_EXPORT data1_node *data1_mk_text (data1_handle dh, NMEM mem,
                                      const char *buf, data1_node *parent);

YAZ_EXPORT data1_node *data1_mk_comment_n (data1_handle dh, NMEM mem,
                                           const char *buf, size_t len,
                                           data1_node *parent);

YAZ_EXPORT data1_node *data1_mk_comment (data1_handle dh, NMEM mem,
                                         const char *buf, data1_node *parent);

YAZ_EXPORT data1_node *data1_mk_preprocess_n (data1_handle dh, NMEM nmem,
                                              const char *target, size_t len,
                                              const char **attr,
                                              data1_node *at);

YAZ_EXPORT data1_node *data1_mk_preprocess (data1_handle dh, NMEM nmem,
                                            const char *target,
                                            const char **attr,
                                            data1_node *at);

YAZ_EXPORT data1_node *data1_insert_preprocess_n (data1_handle dh, NMEM nmem,
                                                  const char *target,
                                                  size_t len,
                                                  const char **attr,
                                                  data1_node *at);

YAZ_EXPORT data1_node *data1_insert_preprocess (data1_handle dh, NMEM nmem,
                                                const char *target,
                                                const char **attr,
                                                data1_node *at);

YAZ_EXPORT data1_node *data1_mk_root (data1_handle dh, NMEM nmem,
                                      const char *name);
YAZ_EXPORT void data1_set_root(data1_handle dh, data1_node *res,
                               NMEM nmem, const char *name);

YAZ_EXPORT data1_node *data1_mk_tag_data_zint(data1_handle dh, data1_node *at,
                                              const char *tag, zint num,
                                              NMEM nmem);
YAZ_EXPORT data1_node *data1_mk_tag_data_int(data1_handle dh, data1_node *at,
                                             const char *tag, int num,
                                             NMEM nmem);
YAZ_EXPORT data1_node *data1_mk_tag_data_oid(data1_handle dh, data1_node *at,
                                             const char *tag, Odr_oid *oid,
                                             NMEM nmem);
YAZ_EXPORT data1_node *data1_mk_tag_data_text(data1_handle dh, data1_node *at,
                                              const char *tag,
                                              const char *str,
                                               NMEM nmem);
YAZ_EXPORT data1_node *data1_mk_tag_data_text_uni(data1_handle dh,
                                                  data1_node *at,
                                                  const char *tag,
                                                  const char *str,
                                                  NMEM nmem);

YAZ_EXPORT data1_absyn *data1_get_absyn(data1_handle dh, const char *name,
                                        enum DATA1_XPATH_INDEXING en);

YAZ_EXPORT data1_node *data1_search_tag(data1_handle dh, data1_node *n,
                                        const char *tag);
YAZ_EXPORT data1_node *data1_mk_tag_uni(data1_handle dh, NMEM nmem,
                                        const char *tag, data1_node *at);
YAZ_EXPORT data1_attset *data1_get_attset(data1_handle dh, const char *name);
YAZ_EXPORT data1_maptab *data1_read_maptab(data1_handle dh, const char *file);
YAZ_EXPORT data1_node *data1_map_record(data1_handle dh, data1_node *n,
                                        data1_maptab *map, NMEM m);
YAZ_EXPORT data1_marctab *data1_read_marctab(data1_handle dh,
                                             const char *file);
YAZ_EXPORT data1_marctab *data1_absyn_getmarctab(data1_handle dh,
                                                 data1_node *root);
YAZ_EXPORT data1_element *data1_absyn_getelements(data1_handle dh,
                                                 data1_node *root);
YAZ_EXPORT char *data1_nodetomarc(data1_handle dh, data1_marctab *p,
                                  data1_node *n, int selected, int *len);
YAZ_EXPORT char *data1_nodetoidsgml(data1_handle dh, data1_node *n,
                                    int select, int *len);
YAZ_EXPORT Z_ExplainRecord *data1_nodetoexplain(data1_handle dh,
                                                data1_node *n, int select,
                                                ODR o);
YAZ_EXPORT Z_BriefBib *data1_nodetosummary(data1_handle dh,
                                           data1_node *n, int select,
                                           ODR o);
YAZ_EXPORT char *data1_nodetosoif(data1_handle dh, data1_node *n, int select,
                                  int *len);
YAZ_EXPORT void data1_set_tabpath(data1_handle dh, const char *path);
YAZ_EXPORT void data1_set_tabroot (data1_handle dp, const char *p);
YAZ_EXPORT const char *data1_get_tabpath(data1_handle dh);
YAZ_EXPORT const char *data1_get_tabroot(data1_handle dh);

YAZ_EXPORT WRBUF data1_get_wrbuf (data1_handle dp);
YAZ_EXPORT char **data1_get_read_buf(data1_handle dp, int **lenp);
YAZ_EXPORT char **data1_get_map_buf(data1_handle dp, int **lenp);
YAZ_EXPORT data1_absyn_cache *data1_absyn_cache_get (data1_handle dh);
YAZ_EXPORT data1_attset_cache *data1_attset_cache_get (data1_handle dh);
YAZ_EXPORT NMEM data1_nmem_get(data1_handle dh);
YAZ_EXPORT void data1_pr_tree(data1_handle dh, data1_node *n, FILE *out);
YAZ_EXPORT char *data1_insert_string(data1_handle dh, data1_node *res,
                                     NMEM m, const char *str);
YAZ_EXPORT char *data1_insert_string_n(data1_handle dh, data1_node *res,
                                       NMEM m, const char *str, size_t len);
YAZ_EXPORT char *data1_insert_zint(data1_handle dh, data1_node *res,
                                   NMEM m, zint num);
YAZ_EXPORT void data1_set_data_string_n(data1_handle dh, data1_node *res,
                                        NMEM m, const char *str, size_t len);
YAZ_EXPORT void data1_set_data_string(data1_handle dh, data1_node *res,
                                      NMEM m, const char *str);
YAZ_EXPORT void data1_set_data_zint(data1_handle dh, data1_node *res, NMEM m, zint num);
YAZ_EXPORT data1_node *data1_read_sgml(data1_handle dh, NMEM m,
                                       const char *buf);
YAZ_EXPORT data1_node *data1_read_xml(data1_handle dh,
                                      int (*rf)(void *, char *, size_t),
                                      void *fh, NMEM m);
YAZ_EXPORT void data1_absyn_trav(data1_handle dh, void *handle,
                                 void (*fh)(data1_handle dh,
                                            void *h, data1_absyn *a));

YAZ_EXPORT data1_attset *data1_attset_search_id(data1_handle dh,
                                                const Odr_oid *oid);

YAZ_EXPORT char *data1_getNodeValue(data1_node* node, char* pTagPath);
YAZ_EXPORT data1_node *data1_LookupNode(data1_node* node, char* pTagPath);
YAZ_EXPORT int data1_CountOccurences(data1_node* node, char* pTagPath);

YAZ_EXPORT FILE *data1_path_fopen(data1_handle dh, const char *file,
                                  const char *mode);

/* obsolete functions ... */

YAZ_EXPORT data1_node *data1_mk_node (data1_handle dh, NMEM m);
YAZ_EXPORT data1_node *data1_insert_taggeddata (data1_handle dh,
                                                data1_node *root,
                                                data1_node *at,
                                                const char *tagname, NMEM m);
YAZ_EXPORT data1_node *data1_mk_node_type (data1_handle dh, NMEM m, int type);
YAZ_EXPORT data1_node *data1_add_taggeddata (data1_handle dh, data1_node *root,
                                             data1_node *at,
                                             const char *tagname,
                                             NMEM m);

YAZ_EXPORT data1_node *data1_get_root_tag(data1_handle dh, data1_node *n);

YAZ_EXPORT int data1_iconv(data1_handle dh, NMEM m, data1_node *n,
                           const char *tocode,
                           const char *fromcode);

YAZ_EXPORT const char *data1_get_encoding(data1_handle dh, data1_node *n);

YAZ_EXPORT int data1_is_xmlmode(data1_handle dh);

YAZ_EXPORT const char *data1_systag_lookup(data1_absyn *absyn, const char *tag,
                                           const char *default_value);

YAZ_EXPORT void data1_concat_text(data1_handle dh, NMEM m, data1_node *n);
YAZ_EXPORT void data1_chop_text(data1_handle dh, NMEM m, data1_node *n);

YAZ_EXPORT void data1_absyn_destroy(data1_handle dh);

YAZ_EXPORT const char *data1_absyn_get_staticrank(data1_absyn *absyn);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

