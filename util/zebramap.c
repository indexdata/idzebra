/* $Id: zebramap.c,v 1.56 2007-01-22 18:15:04 adam Exp $
   Copyright (C) 1995-2007
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

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <charmap.h>
#include <attrfind.h>
#include <yaz/yaz-util.h>

#include <zebramap.h>

#define ZEBRA_MAP_TYPE_SORT  1
#define ZEBRA_MAP_TYPE_INDEX 2
#define ZEBRA_MAP_TYPE_STATICRANK 3

#define ZEBRA_REPLACE_ANY  300

struct zebra_map {
    unsigned reg_id;
    int completeness;
    int positioned;
    int alwaysmatches;
    int first_in_field;
    int type;
    union {
        struct {
            int dummy;
        } index;
        struct {
            int entry_size;
        } sort;
    } u;
    chrmaptab maptab;
    const char *maptab_name;
    struct zebra_map *next;
};

struct zebra_maps {
    char *tabpath;
    char *tabroot;
    NMEM nmem;
    struct zebra_map *map_list;
    char temp_map_str[2];
    const char *temp_map_ptr[2];
    struct zebra_map **lookup_array;
    WRBUF wrbuf_1;
    int no_maps;
};

void zebra_maps_close(ZebraMaps zms)
{
    struct zebra_map *zm = zms->map_list;
    while (zm)
    {
	if (zm->maptab)
	    chrmaptab_destroy(zm->maptab);
	zm = zm->next;
    }
    wrbuf_free(zms->wrbuf_1, 1);
    nmem_destroy(zms->nmem);
    xfree(zms);
}

ZEBRA_RES zebra_maps_read_file(ZebraMaps zms, const char *fname)
{
    FILE *f;
    char line[512];
    char *argv[10];
    int argc;
    int lineno = 0;
    int failures = 0;
    struct zebra_map **zm = 0, *zp;

    if (!(f = yaz_fopen(zms->tabpath, fname, "r", zms->tabroot)))
    {
	yaz_log(YLOG_ERRNO|YLOG_FATAL, "%s", fname);
	return ZEBRA_FAIL;
    }
    while ((argc = readconf_line(f, &lineno, line, 512, argv, 10)))
    {
        if (argc == 1)
        {
            yaz_log(YLOG_WARN, "%s:%d: Missing arguments for '%s'",
                    fname, lineno, argv[0]);
            failures++;
            break;
        }
        if (argc > 2)
        {
            yaz_log(YLOG_WARN, "%s:%d: Too many arguments for '%s'",
                    fname, lineno, argv[0]);
            failures++;
            break;
        }
	if (!yaz_matchstr(argv[0], "index"))
	{
	    if (!zm)
		zm = &zms->map_list;
	    else
		zm = &(*zm)->next;
	    *zm = (struct zebra_map *) nmem_malloc(zms->nmem, sizeof(**zm));
	    (*zm)->reg_id = argv[1][0];
	    (*zm)->maptab_name = NULL;
	    (*zm)->maptab = NULL;
	    (*zm)->type = ZEBRA_MAP_TYPE_INDEX;
	    (*zm)->completeness = 0;
	    (*zm)->positioned = 1;
	    (*zm)->alwaysmatches = 0;
	    (*zm)->first_in_field = 0;
	    zms->no_maps++;
	}
	else if (!yaz_matchstr(argv[0], "sort"))
	{
	    if (!zm)
		zm = &zms->map_list;
	    else
		zm = &(*zm)->next;
	    *zm = (struct zebra_map *) nmem_malloc(zms->nmem, sizeof(**zm));
	    (*zm)->reg_id = argv[1][0];
	    (*zm)->maptab_name = NULL;
	    (*zm)->type = ZEBRA_MAP_TYPE_SORT;
            (*zm)->u.sort.entry_size = 80;
	    (*zm)->maptab = NULL;
	    (*zm)->completeness = 0;
	    (*zm)->positioned = 0;
	    (*zm)->alwaysmatches = 0;
	    (*zm)->first_in_field = 0;
	    zms->no_maps++;
	}
	else if (!yaz_matchstr(argv[0], "staticrank"))
	{
	    if (!zm)
		zm = &zms->map_list;
	    else
		zm = &(*zm)->next;
	    *zm = (struct zebra_map *) nmem_malloc(zms->nmem, sizeof(**zm));
	    (*zm)->reg_id = argv[1][0];
	    (*zm)->maptab_name = NULL;
	    (*zm)->type = ZEBRA_MAP_TYPE_STATICRANK;
	    (*zm)->maptab = NULL;
	    (*zm)->completeness = 1;
	    (*zm)->positioned = 0;
	    (*zm)->alwaysmatches = 0;
	    (*zm)->first_in_field = 0;
	    zms->no_maps++;
	}
        else if (!zm)
        {
            yaz_log(YLOG_WARN, "%s:%d: Missing sort/index before '%s'",  
                    fname, lineno, argv[0]);
            failures++;
        }
	else if (!yaz_matchstr(argv[0], "charmap") && argc == 2)
	{
            if ((*zm)->type != ZEBRA_MAP_TYPE_STATICRANK)
                (*zm)->maptab_name = nmem_strdup(zms->nmem, argv[1]);
            else
            {
                yaz_log(YLOG_WARN|YLOG_FATAL, "%s:%d: charmap for "
                        "staticrank is invalid", fname, lineno);
                yaz_log(YLOG_LOG, "Type is %d", (*zm)->type);
                failures++;
            }
	}
	else if (!yaz_matchstr(argv[0], "completeness") && argc == 2)
	{
	    (*zm)->completeness = atoi(argv[1]);
	}
	else if (!yaz_matchstr(argv[0], "position") && argc == 2)
	{
	    (*zm)->positioned = atoi(argv[1]);
	}
	else if (!yaz_matchstr(argv[0], "alwaysmatches") && argc == 2)
	{
            if ((*zm)->type != ZEBRA_MAP_TYPE_STATICRANK)
                (*zm)->alwaysmatches = atoi(argv[1]);
            else
            {
                yaz_log(YLOG_WARN|YLOG_FATAL, "%s:%d: alwaysmatches for "
                        "staticrank is invalid", fname, lineno);
                failures++;
            }
	}
	else if (!yaz_matchstr(argv[0], "firstinfield") && argc == 2)
	{
	    (*zm)->first_in_field = atoi(argv[1]);
	}
        else if (!yaz_matchstr(argv[0], "entrysize") && argc == 2)
        {
            if ((*zm)->type == ZEBRA_MAP_TYPE_SORT)
		(*zm)->u.sort.entry_size = atoi(argv[1]);
        }
        else
        {
            yaz_log(YLOG_WARN, "%s:%d: Unrecognized directive '%s'",  
                    fname, lineno, argv[0]);
            failures++;
        }
    }
    if (zm)
	(*zm)->next = NULL;
    yaz_fclose(f);

    for (zp = zms->map_list; zp; zp = zp->next)
	zms->lookup_array[zp->reg_id] = zp;

    if (failures)
        return ZEBRA_FAIL;
    return ZEBRA_OK;
}

ZebraMaps zebra_maps_open(Res res, const char *base_path,
			  const char *profile_path)
{
    ZebraMaps zms = (ZebraMaps) xmalloc(sizeof(*zms));
    int i;

    zms->nmem = nmem_create();
    zms->no_maps = 0;
    zms->tabpath = profile_path ? nmem_strdup(zms->nmem, profile_path) : 0;
    zms->tabroot = 0;
    if (base_path)
        zms->tabroot = nmem_strdup(zms->nmem, base_path);
    zms->map_list = NULL;

    zms->temp_map_str[0] = '\0';
    zms->temp_map_str[1] = '\0';

    zms->temp_map_ptr[0] = zms->temp_map_str;
    zms->temp_map_ptr[1] = NULL;

    zms->lookup_array = (struct zebra_map**)
	nmem_malloc(zms->nmem, sizeof(*zms->lookup_array)*256);
    zms->wrbuf_1 = wrbuf_alloc();

    for (i = 0; i<256; i++)
	zms->lookup_array[i] = 0;
    return zms;
}

struct zebra_map *zebra_map_get(ZebraMaps zms, unsigned reg_id)
{
    assert(reg_id >= 0 && reg_id <= 255);
    return zms->lookup_array[reg_id];
}

chrmaptab zebra_charmap_get(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (!zm)
    {
	zm = (struct zebra_map *) nmem_malloc(zms->nmem, sizeof(*zm));
	
	/* no reason to warn if no maps are installed at ALL */
	if (zms->no_maps)
	    yaz_log(YLOG_WARN, "Unknown register type: %c", reg_id);

	zm->reg_id = reg_id;
	zm->maptab_name = nmem_strdup(zms->nmem, "@");
	zm->maptab = NULL;
	zm->type = ZEBRA_MAP_TYPE_INDEX;
	zm->completeness = 0;
	zm->next = zms->map_list;
	zms->map_list = zm->next;

	zms->lookup_array[zm->reg_id & 255] = zm;
    }
    if (!zm->maptab)
    {
	if (!zm->maptab_name || !yaz_matchstr(zm->maptab_name, "@"))
	    return NULL;
	if (!(zm->maptab = chrmaptab_create(zms->tabpath,
					    zm->maptab_name,
					    zms->tabroot)))
	    yaz_log(YLOG_WARN, "Failed to read character table %s",
		    zm->maptab_name);
	else
	    yaz_log(YLOG_DEBUG, "Read character table %s", zm->maptab_name);
    }
    return zm->maptab;
}

const char **zebra_maps_input(ZebraMaps zms, unsigned reg_id,
			      const char **from, int len, int first)
{
    chrmaptab maptab;

    maptab = zebra_charmap_get(zms, reg_id);
    if (maptab)
	return chr_map_input(maptab, from, len, first);
    
    zms->temp_map_str[0] = **from;

    (*from)++;
    return zms->temp_map_ptr;
}

const char **zebra_maps_search(ZebraMaps zms, unsigned reg_id,
			       const char **from, int len,  int *q_map_match)
{
    chrmaptab maptab;
    
    *q_map_match = 0;
    maptab = zebra_charmap_get(zms, reg_id);
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
    zms->temp_map_str[0] = **from;

    (*from)++;
    return zms->temp_map_ptr;
}

const char *zebra_maps_output(ZebraMaps zms, unsigned reg_id,
			      const char **from)
{
    chrmaptab maptab = zebra_charmap_get(zms, reg_id);
    if (!maptab)
	return 0;
    return chr_map_output(maptab, from, 1);
}


/* ------------------------------------ */

int zebra_maps_is_complete(ZebraMaps zms, unsigned reg_id)
{ 
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->completeness;
    return 0;
}

int zebra_maps_is_positioned(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->positioned;
    return 0;
}

int zebra_maps_is_index(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_INDEX;
    return 0;
}

int zebra_maps_is_staticrank(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_STATICRANK;
    return 0;
}
    
int zebra_maps_is_sort(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_SORT;
    return 0;
}

int zebra_maps_is_alwaysmatches(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->alwaysmatches;
    return 0;
}

int zebra_maps_is_first_in_field(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->first_in_field;
    return 0;
}

int zebra_maps_sort(ZebraMaps zms, Z_SortAttributes *sortAttributes,
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

int zebra_maps_attr(ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		    unsigned *reg_id, char **search_type, char *rank_type,
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
    structure_value = attr_find(&structure, NULL);
    relation_value = attr_find(&relation, NULL);
    sort_relation_value = attr_find(&sort_relation, NULL);
    weight_value = attr_find(&weight, NULL);
    use_value = attr_find(&use, NULL);

    if (completeness_value == 2 || completeness_value == 3)
	*complete_flag = 1;
    else
	*complete_flag = 0;
    *reg_id = 0;

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
	*reg_id = 'p';
    else
	*reg_id = 'w';
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
	*reg_id = 0;
	break;
    case 109: /* numeric string */
	*reg_id = 'n';
	*search_type = "numeric";
        break;
    case 104: /* urx */
	*reg_id = 'u';
	*search_type = "phrase";
	break;
    case 3:   /* key */
        *reg_id = '0';
        *search_type = "phrase";
        break;
    case 4:  /* year */
        *reg_id = 'y';
        *search_type = "phrase";
        break;
    case 5:  /* date */
        *reg_id = 'd';
        *search_type = "phrase";
        break;
    default:
	return -1;
    }
    return 0;
}

WRBUF zebra_replace(ZebraMaps zms, unsigned reg_id, const char *ex_list,
		    const char *input_str, int input_len)
{
    wrbuf_rewind(zms->wrbuf_1);
    wrbuf_write(zms->wrbuf_1, input_str, input_len);
    return zms->wrbuf_1;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

