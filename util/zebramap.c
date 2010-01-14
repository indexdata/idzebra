/* This file is part of the Zebra server.
   Copyright (C) 1994-2010 Index Data

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

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <charmap.h>
#include <attrfind.h>
#include <yaz/yaz-util.h>

#if YAZ_HAVE_ICU
#include <yaz/icu.h>
#endif
#include <zebramap.h>

#define ZEBRA_MAP_TYPE_SORT  1
#define ZEBRA_MAP_TYPE_INDEX 2
#define ZEBRA_MAP_TYPE_STATICRANK 3

#define ZEBRA_REPLACE_ANY  300

struct zebra_map {
    const char *id;
    int completeness;
    int positioned;
    int alwaysmatches;
    int first_in_field;
    int type;
    int use_chain;
    int debug;
    union {
        struct {
            int entry_size;
        } sort;
    } u;
    chrmaptab maptab;
    const char *maptab_name;
    zebra_maps_t zebra_maps;
#if YAZ_HAVE_XML2
    xmlDocPtr doc;
#endif
#if YAZ_HAVE_ICU
    struct icu_chain *icu_chain;
#endif
    WRBUF input_str;
    WRBUF print_str;
    size_t simple_off;
    struct zebra_map *next;
};

struct zebra_maps_s {
    char *tabpath;
    char *tabroot;
    NMEM nmem;
    char temp_map_str[2];
    const char *temp_map_ptr[2];
    WRBUF wrbuf_1;
    int no_files_read;
    zebra_map_t map_list;
    zebra_map_t last_map;
};

void zebra_maps_close(zebra_maps_t zms)
{
    struct zebra_map *zm = zms->map_list;
    while (zm)
    {
	if (zm->maptab)
	    chrmaptab_destroy(zm->maptab);
#if YAZ_HAVE_ICU
        if (zm->icu_chain)
            icu_chain_destroy(zm->icu_chain);
#endif
#if YAZ_HAVE_XML2
        xmlFreeDoc(zm->doc);
#endif
        wrbuf_destroy(zm->input_str);
        wrbuf_destroy(zm->print_str);
	zm = zm->next;
    }
    wrbuf_destroy(zms->wrbuf_1);
    nmem_destroy(zms->nmem);
    xfree(zms);
}

zebra_map_t zebra_add_map(zebra_maps_t zms, const char *index_type,
                          int map_type)
{
    zebra_map_t zm = (zebra_map_t) nmem_malloc(zms->nmem, sizeof(*zm));

    zm->zebra_maps = zms;
    zm->id = nmem_strdup(zms->nmem, index_type);
    zm->maptab_name = 0;
    zm->use_chain = 0;
    zm->debug = 0;
    zm->maptab = 0;
    zm->type = map_type;
    zm->completeness = 0;
    zm->positioned = 0;
    zm->alwaysmatches = 0;
    zm->first_in_field = 0;

    if (zms->last_map)
        zms->last_map->next = zm;
    else
        zms->map_list = zm;
    zms->last_map = zm;
    zm->next = 0;
#if YAZ_HAVE_ICU
    zm->icu_chain = 0;
#endif
#if YAZ_HAVE_XML2
    zm->doc = 0;
#endif
    zm->input_str = wrbuf_alloc();
    zm->print_str = wrbuf_alloc();
    return zm;
}

static int parse_command(zebra_maps_t zms, int argc, char **argv,
                         const char *fname, int lineno)
{
    zebra_map_t zm = zms->last_map;
    if (argc == 1)
    {
        yaz_log(YLOG_WARN, "%s:%d: Missing arguments for '%s'",
                fname, lineno, argv[0]);
        return -1;
    }
    if (argc > 2)
    {
        yaz_log(YLOG_WARN, "%s:%d: Too many arguments for '%s'",
                fname, lineno, argv[0]);
        return -1;
    }
    if (!yaz_matchstr(argv[0], "index"))
    {
        zm = zebra_add_map(zms, argv[1], ZEBRA_MAP_TYPE_INDEX);
        zm->positioned = 1;
    }
    else if (!yaz_matchstr(argv[0], "sort"))
    {
        zm = zebra_add_map(zms, argv[1], ZEBRA_MAP_TYPE_SORT);
        zm->u.sort.entry_size = 80;
    }
    else if (!yaz_matchstr(argv[0], "staticrank"))
    {
        zm = zebra_add_map(zms, argv[1], ZEBRA_MAP_TYPE_STATICRANK);
        zm->completeness = 1;
    }
    else if (!zm)
    {
        yaz_log(YLOG_WARN, "%s:%d: Missing sort/index before '%s'",  
                fname, lineno, argv[0]);
        return -1;
    }
    else if (!yaz_matchstr(argv[0], "charmap") && argc == 2)
    {
        if (zm->type != ZEBRA_MAP_TYPE_STATICRANK)
            zm->maptab_name = nmem_strdup(zms->nmem, argv[1]);
        else
        {
            yaz_log(YLOG_WARN|YLOG_FATAL, "%s:%d: charmap for "
                    "staticrank is invalid", fname, lineno);
            yaz_log(YLOG_LOG, "Type is %d", zm->type);
            return -1;
        }
    }
    else if (!yaz_matchstr(argv[0], "completeness") && argc == 2)
    {
        zm->completeness = atoi(argv[1]);
    }
    else if (!yaz_matchstr(argv[0], "position") && argc == 2)
    {
        zm->positioned = atoi(argv[1]);
    }
    else if (!yaz_matchstr(argv[0], "alwaysmatches") && argc == 2)
    {
        if (zm->type != ZEBRA_MAP_TYPE_STATICRANK)
            zm->alwaysmatches = atoi(argv[1]);
        else
        {
            yaz_log(YLOG_WARN|YLOG_FATAL, "%s:%d: alwaysmatches for "
                    "staticrank is invalid", fname, lineno);
            return -1;
        }
    }
    else if (!yaz_matchstr(argv[0], "firstinfield") && argc == 2)
    {
        zm->first_in_field = atoi(argv[1]);
    }
    else if (!yaz_matchstr(argv[0], "entrysize") && argc == 2)
    {
        if (zm->type == ZEBRA_MAP_TYPE_SORT)
            zm->u.sort.entry_size = atoi(argv[1]);
        else
        {
            yaz_log(YLOG_WARN, 
                    "%s:%d: entrysize only valid in sort section",  
                    fname, lineno);
            return -1;
        }
    }
    else if (!yaz_matchstr(argv[0], "simplechain"))
    {
        zm->use_chain = 1;
#if YAZ_HAVE_ICU
        zm->icu_chain = 0;
#endif
    }
    else if (!yaz_matchstr(argv[0], "icuchain"))
    {
        char full_path[1024];
        if (!yaz_filepath_resolve(argv[1], zms->tabpath, zms->tabroot,
                                  full_path))
        {
            yaz_log(YLOG_WARN, "%s:%d: Could not locate icuchain config '%s'",
                    fname, lineno, argv[1]);
            return -1;
        }
#if YAZ_HAVE_XML2
        zm->doc = xmlParseFile(full_path);
        if (!zm->doc)
        {
            yaz_log(YLOG_WARN, "%s:%d: Could not load icuchain config '%s'",
                    fname, lineno, argv[1]);
            return -1;
        }
        else
        {
#if YAZ_HAVE_ICU
            UErrorCode status;
            xmlNode *xml_node = xmlDocGetRootElement(zm->doc);
            zm->icu_chain = 
                icu_chain_xml_config(xml_node,
/* not sure about sort for this function yet.. */
#if 1
                                     1,
#else
                                     zm->type == ZEBRA_MAP_TYPE_SORT,
#endif                                    
                                     &status);
            if (!zm->icu_chain)
            {
                yaz_log(YLOG_WARN, "%s:%d: Failed to load ICU chain %s",
                        fname, lineno, argv[1]);
            }
            zm->use_chain = 1;
#else
            yaz_log(YLOG_WARN, "%s:%d: ICU support unavailable",
                    fname, lineno);
            return -1;
#endif
        }
#else
        yaz_log(YLOG_WARN, "%s:%d: XML support unavailable",
                fname, lineno);
        return -1;
#endif
    }
    else if (!yaz_matchstr(argv[0], "debug") && argc == 2)
    {
        zm->debug = atoi(argv[1]);
    }
    else
    {
        yaz_log(YLOG_WARN, "%s:%d: Unrecognized directive '%s'",  
                fname, lineno, argv[0]);
        return -1;
    }
    return 0;
}

ZEBRA_RES zebra_maps_read_file(zebra_maps_t zms, const char *fname)
{
    FILE *f;
    char line[512];
    char *argv[10];
    int argc;
    int lineno = 0;
    int failures = 0;

    if (!(f = yaz_fopen(zms->tabpath, fname, "r", zms->tabroot)))
    {
	yaz_log(YLOG_ERRNO|YLOG_FATAL, "%s", fname);
	return ZEBRA_FAIL;
    }
    while ((argc = readconf_line(f, &lineno, line, 512, argv, 10)))
    {
        int r = parse_command(zms, argc, argv, fname, lineno);
        if (r)
            failures++;
    }
    yaz_fclose(f);

    if (failures)
        return ZEBRA_FAIL;

    (zms->no_files_read)++;
    return ZEBRA_OK;
}

zebra_maps_t zebra_maps_open(Res res, const char *base_path,
                             const char *profile_path)
{
    zebra_maps_t zms = (zebra_maps_t) xmalloc(sizeof(*zms));

    zms->nmem = nmem_create();
    zms->tabpath = profile_path ? nmem_strdup(zms->nmem, profile_path) : 0;
    zms->tabroot = 0;
    if (base_path)
        zms->tabroot = nmem_strdup(zms->nmem, base_path);
    zms->map_list = 0;
    zms->last_map = 0;

    zms->temp_map_str[0] = '\0';
    zms->temp_map_str[1] = '\0';

    zms->temp_map_ptr[0] = zms->temp_map_str;
    zms->temp_map_ptr[1] = NULL;

    zms->wrbuf_1 = wrbuf_alloc();

    zms->no_files_read = 0;
    return zms;
}

void zebra_maps_define_default_sort(zebra_maps_t zms)
{
    zebra_map_t zm = zebra_add_map(zms, "s", ZEBRA_MAP_TYPE_SORT);
    zm->u.sort.entry_size = 80;
}

zebra_map_t zebra_map_get(zebra_maps_t zms, const char *id)
{
    zebra_map_t zm;
    for (zm = zms->map_list; zm; zm = zm->next)
        if (!strcmp(zm->id, id))
            break;
    return zm;
}

zebra_map_t zebra_map_get_or_add(zebra_maps_t zms, const char *id)
{
    struct zebra_map *zm = zebra_map_get(zms, id);
    if (!zm)
    {
        zm = zebra_add_map(zms, id, ZEBRA_MAP_TYPE_INDEX);
	
	/* no reason to warn if no maps are read from file */
	if (zms->no_files_read)
	    yaz_log(YLOG_WARN, "Unknown register type: %s", id);

	zm->maptab_name = nmem_strdup(zms->nmem, "@");
	zm->completeness = 0;
        zm->positioned = 1;
    }
    return zm;
}

chrmaptab zebra_charmap_get(zebra_map_t zm)
{
    if (!zm->maptab)
    {
	if (!zm->maptab_name || !yaz_matchstr(zm->maptab_name, "@"))
	    return NULL;
	if (!(zm->maptab = chrmaptab_create(zm->zebra_maps->tabpath,
					    zm->maptab_name,
					    zm->zebra_maps->tabroot)))
	    yaz_log(YLOG_WARN, "Failed to read character table %s",
		    zm->maptab_name);
	else
	    yaz_log(YLOG_DEBUG, "Read character table %s", zm->maptab_name);
    }
    return zm->maptab;
}

const char **zebra_maps_input(zebra_map_t zm,
			      const char **from, int len, int first)
{
    chrmaptab maptab = zebra_charmap_get(zm);
    if (maptab)
	return chr_map_input(maptab, from, len, first);
    
    zm->zebra_maps->temp_map_str[0] = **from;

    (*from)++;
    return zm->zebra_maps->temp_map_ptr;
}

const char **zebra_maps_search(zebra_map_t zm,
			       const char **from, int len,  int *q_map_match)
{
    chrmaptab maptab;
    
    *q_map_match = 0;
    maptab = zebra_charmap_get(zm);
    if (maptab)
    {
	const char **map;
	map = chr_map_q_input(maptab, from, len, 0);
	if (map && map[0])
	{
	    *q_map_match = 1;
	    return map;
	}
	map = chr_map_input(maptab, from, len, 0);
	if (map)
	    return map;
    }
    zm->zebra_maps->temp_map_str[0] = **from;

    (*from)++;
    return zm->zebra_maps->temp_map_ptr;
}

const char *zebra_maps_output(zebra_map_t zm,
			      const char **from)
{
    chrmaptab maptab = zebra_charmap_get(zm);
    if (!maptab)
	return 0;
    return chr_map_output(maptab, from, 1);
}


/* ------------------------------------ */

int zebra_maps_is_complete(zebra_map_t zm)
{ 
    if (zm)
	return zm->completeness;
    return 0;
}

int zebra_maps_is_positioned(zebra_map_t zm)
{
    if (zm)
	return zm->positioned;
    return 0;
}

int zebra_maps_is_index(zebra_map_t zm)
{
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_INDEX;
    return 0;
}

int zebra_maps_is_staticrank(zebra_map_t zm)
{
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_STATICRANK;
    return 0;
}
    
int zebra_maps_is_sort(zebra_map_t zm)
{
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_SORT;
    return 0;
}

int zebra_maps_is_alwaysmatches(zebra_map_t zm)
{
    if (zm)
	return zm->alwaysmatches;
    return 0;
}

int zebra_maps_is_first_in_field(zebra_map_t zm)
{
    if (zm)
	return zm->first_in_field;
    return 0;
}

int zebra_maps_sort(zebra_maps_t zms, Z_SortAttributes *sortAttributes,
		    int *numerical)
{
    AttrType use;
    AttrType structure;
    int structure_value;
    attr_init_AttrList(&use, sortAttributes->list, 1);
    attr_init_AttrList(&structure, sortAttributes->list, 4);

    *numerical = 0;
    structure_value = attr_find(&structure, 0);
    if (structure_value == 109)
        *numerical = 1;
    return attr_find(&use, NULL);
}

int zebra_maps_attr(zebra_maps_t zms, Z_AttributesPlusTerm *zapt,
		    const char **index_type, char **search_type, char *rank_type,
		    int *complete_flag, int *sort_flag)
{
    AttrType completeness;
    AttrType structure;
    AttrType relation;
    AttrType sort_relation;
    AttrType weight;
    AttrType use;
    int completeness_value;
    int structure_value;
    const char *structure_str = 0;
    int relation_value;
    int sort_relation_value;
    int weight_value;
    int use_value;

    attr_init_APT(&structure, zapt, 4);
    attr_init_APT(&completeness, zapt, 6);
    attr_init_APT(&relation, zapt, 2);
    attr_init_APT(&sort_relation, zapt, 7);
    attr_init_APT(&weight, zapt, 9);
    attr_init_APT(&use, zapt, 1);

    completeness_value = attr_find(&completeness, NULL);
    structure_value = attr_find_ex(&structure, NULL, &structure_str);
    relation_value = attr_find(&relation, NULL);
    sort_relation_value = attr_find(&sort_relation, NULL);
    weight_value = attr_find(&weight, NULL);
    use_value = attr_find(&use, NULL);

    if (completeness_value == 2 || completeness_value == 3)
	*complete_flag = 1;
    else
	*complete_flag = 0;
    *index_type = 0;

    *sort_flag =(sort_relation_value > 0) ? 1 : 0;
    *search_type = "phrase";
    strcpy(rank_type, "void");
    if (relation_value == 102)
    {
        if (weight_value == -1)
            weight_value = 34;
        sprintf(rank_type, "rank,w=%d,u=%d", weight_value, use_value);
    }
    if (*complete_flag)
	*index_type = "p";
    else
	*index_type = "w";
    switch (structure_value)
    {
    case 6:   /* word list */
	*search_type = "and-list";
	break;
    case 105: /* free-form-text */
	*search_type = "or-list";
	break;
    case 106: /* document-text */
        *search_type = "or-list";
	break;	
    case -1:
    case 1:   /* phrase */
    case 2:   /* word */
    case 108: /* string */ 
	*search_type = "phrase";
	break;
    case 107: /* local-number */
	*search_type = "local";
	*index_type = 0;
	break;
    case 109: /* numeric string */
	*index_type = "n";
	*search_type = "numeric";
        break;
    case 104: /* urx */
	*index_type = "u";
	*search_type = "phrase";
	break;
    case 3:   /* key */
        *index_type = "0";
        *search_type = "phrase";
        break;
    case 4:  /* year */
        *index_type = "y";
        *search_type = "phrase";
        break;
    case 5:  /* date */
        *index_type = "d";
        *search_type = "phrase";
        break;
    case -2:
        if (structure_str && *structure_str)
            *index_type = structure_str;
        else
            return -1;
        break;
    default:
	return -1;
    }
    return 0;
}

WRBUF zebra_replace(zebra_map_t zm, const char *ex_list,
		    const char *input_str, int input_len)
{
    wrbuf_rewind(zm->zebra_maps->wrbuf_1);
    wrbuf_write(zm->zebra_maps->wrbuf_1, input_str, input_len);
    return zm->zebra_maps->wrbuf_1;
}

#define SE_CHARS ";,.()-/?<> \r\n\t"

static int tokenize_simple(zebra_map_t zm,
                           const char **result_buf, size_t *result_len)
{
    char *buf = wrbuf_buf(zm->input_str);
    size_t len = wrbuf_len(zm->input_str);
    size_t i = zm->simple_off;
    size_t start;

    while (i < len && strchr(SE_CHARS, buf[i]))
        i++;
    start = i;
    while (i < len && !strchr(SE_CHARS, buf[i]))
    {
        if (buf[i] > 32 && buf[i] < 127)
            buf[i] = tolower(buf[i]);
        i++;
    }

    zm->simple_off = i;
    if (start != i)
    {
        *result_buf = buf + start;
        *result_len = i - start;
        return 1;
    }
    return 0;
 }


int zebra_map_tokenize_next(zebra_map_t zm,
                            const char **result_buf, size_t *result_len,
                            const char **display_buf, size_t *display_len)
{
    assert(zm->use_chain);

#if YAZ_HAVE_ICU
    if (!zm->icu_chain)
        return tokenize_simple(zm, result_buf, result_len);
    else
    {
        UErrorCode status;
        while (icu_chain_next_token(zm->icu_chain, &status))
        {
            if (!U_SUCCESS(status))
                return 0;
            *result_buf = icu_chain_token_sortkey(zm->icu_chain);
            assert(*result_buf);

            *result_len = strlen(*result_buf);

            if (display_buf)
            {
                *display_buf = icu_chain_token_display(zm->icu_chain);
                if (display_len)
                    *display_len = strlen(*display_buf);
            }
            if (zm->debug)
            {
                wrbuf_rewind(zm->print_str);
                wrbuf_write_escaped(zm->print_str, *result_buf, *result_len);
                yaz_log(YLOG_LOG, "output %s", wrbuf_cstr(zm->print_str));
            }

            if (**result_buf != '\0')
                return 1;
        }
    }
    return 0;
#else
    return tokenize_simple(zm, result_buf, result_len);
#endif
}

int zebra_map_tokenize_start(zebra_map_t zm,
                             const char *buf, size_t len)
{
    assert(zm->use_chain);

    wrbuf_rewind(zm->input_str);
    wrbuf_write(zm->input_str, buf, len);
    zm->simple_off = 0;
#if YAZ_HAVE_ICU
    if (zm->icu_chain)
    {
        UErrorCode status;
        if (zm->debug)
        {
            wrbuf_rewind(zm->print_str);
            wrbuf_write_escaped(zm->print_str, wrbuf_buf(zm->input_str),
                                wrbuf_len(zm->input_str));
            
            yaz_log(YLOG_LOG, "input %s", 
                    wrbuf_cstr(zm->print_str)); 
        }
        icu_chain_assign_cstr(zm->icu_chain,
                              wrbuf_cstr(zm->input_str),
                              &status);
        if (!U_SUCCESS(status))
        {
            if (zm->debug)
            {
                yaz_log(YLOG_WARN, "bad encoding for input");
            }
            return -1;
        }
    }
#endif
    return 0;
}

int zebra_maps_is_icu(zebra_map_t zm)
{
    assert(zm);
#if YAZ_HAVE_ICU
    return zm->use_chain;
#else
    return 0;
#endif
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

