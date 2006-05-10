/* $Id: zebramap.c,v 1.47 2006-05-10 08:13:46 adam Exp $
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

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <charmap.h>
#include <yaz/yaz-util.h>

#include <idzebra/zebramap.h>

#define ZEBRA_MAP_TYPE_SORT  1
#define ZEBRA_MAP_TYPE_INDEX 2

#define ZEBRA_REPLACE_ANY  300

struct zebra_map {
    unsigned reg_id;
    int completeness;
    int positioned;
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
    struct zebra_map **zm = 0, *zp;

    if (!(f = yaz_fopen(zms->tabpath, fname, "r", zms->tabroot)))
    {
	yaz_log(YLOG_ERRNO|YLOG_FATAL, "%s", fname);
	return ZEBRA_FAIL;
    }
    while ((argc = readconf_line(f, &lineno, line, 512, argv, 10)))
    {
	if (!yaz_matchstr(argv[0], "index") && argc == 2)
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
	    zms->no_maps++;
	}
	else if (!yaz_matchstr(argv[0], "sort") && argc == 2)
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
	    zms->no_maps++;
	}
	else if (zm && !yaz_matchstr(argv[0], "charmap") && argc == 2)
	{
	    (*zm)->maptab_name = nmem_strdup(zms->nmem, argv[1]);
	}
	else if (zm && !yaz_matchstr(argv[0], "completeness") && argc == 2)
	{
	    (*zm)->completeness = atoi(argv[1]);
	}
	else if (zm && !yaz_matchstr(argv[0], "position") && argc == 2)
	{
	    (*zm)->positioned = atoi(argv[1]);
	}
        else if (zm && !yaz_matchstr(argv[0], "entrysize") && argc == 2)
        {
            if ((*zm)->type == ZEBRA_MAP_TYPE_SORT)
		(*zm)->u.sort.entry_size = atoi(argv[1]);
        }
    }
    if (zm)
	(*zm)->next = NULL;
    yaz_fclose(f);

    for (zp = zms->map_list; zp; zp = zp->next)
	zms->lookup_array[zp->reg_id] = zp;

    return ZEBRA_OK;
}

ZebraMaps zebra_maps_open(Res res, const char *base_path,
			  const char *profile_path)
{
    ZebraMaps zms = (ZebraMaps) xmalloc(sizeof(*zms));
    int i;

    zms->nmem = nmem_create();
    zms->no_maps = 0;
    zms->tabpath = nmem_strdup(zms->nmem, profile_path);
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

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributeElement **attributeList;
    int num_attributes;
} AttrType;

static int attr_find(AttrType *src, oid_value *attributeSetP)
{
    while (src->major < src->num_attributes)
    {
        Z_AttributeElement *element;

        element = src->attributeList[src->major];
        if (src->type == *element->attributeType)
        {
            switch (element->which) 
            {
            case Z_AttributeValue_numeric:
                ++(src->major);
                if (element->attributeSet && attributeSetP)
                {
                    oident *attrset;

                    attrset = oid_getentbyoid(element->attributeSet);
                    *attributeSetP = attrset->value;
                }
                return *element->value.numeric;
                break;
            case Z_AttributeValue_complex:
                if (src->minor >= element->value.complex->num_list ||
                    element->value.complex->list[src->minor]->which !=  
                    Z_StringOrNumeric_numeric)
                    break;
                ++(src->minor);
                if (element->attributeSet && attributeSetP)
                {
                    oident *attrset;

                    attrset = oid_getentbyoid(element->attributeSet);
                    *attributeSetP = attrset->value;
                }
                return *element->value.complex->list[src->minor-1]->u.numeric;
            default:
                assert(0);
            }
        }
        ++(src->major);
    }
    return -1;
}

static void attr_init_APT(AttrType *src, Z_AttributesPlusTerm *zapt, int type)
{
    src->attributeList = zapt->attributes->attributes;
    src->num_attributes = zapt->attributes->num_attributes;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

static void attr_init_AttrList(AttrType *src, Z_AttributeList *list, int type)
{
    src->attributeList = list->attributes;
    src->num_attributes = list->num_attributes;
    src->type = type;
    src->major = 0;
    src->minor = 0;
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
    
int zebra_maps_is_sort(ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get(zms, reg_id);
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_SORT;
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
    if (relation_value == 103)
    {
        *search_type = "always";
        return 0;
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

