/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebramap.c,v $
 * Revision 1.1  1997-10-27 14:33:06  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 */

#include <assert.h>
#include <ctype.h>

#include <yaz-util.h>
#include <charmap.h>
#include <zebramap.h>

struct zebra_map {
    int reg_type;
    chrmaptab maptab;
    struct zebra_map *next;
};

struct zebra_maps {
    char *tabpath;
    NMEM nmem;
    struct zebra_map *map_list;
};

void zebra_maps_close (ZebraMaps zms)
{
    struct zebra_map *zm = zms->map_list;
    while (zm)
    {
	struct zebra_map *zm_next = zm->next;

	chrmaptab_destroy (zm->maptab);
	xfree (zm);
	zm = zm_next;
    }
    nmem_destroy (zms->nmem);
    xfree (zms);
}

ZebraMaps zebra_maps_open (const char *tabpath)
{
    ZebraMaps zms = xmalloc (sizeof(*zms));

    zms->nmem = nmem_create ();
    zms->tabpath = nmem_strdup (zms->nmem, tabpath);
    zms->map_list = NULL;
    return zms;
}

chrmaptab zebra_map_get (ZebraMaps zms, int reg_type)
{
    char name[512];
    struct zebra_map *zm;

    for (zm = zms->map_list; zm; zm = zm->next)
    {
	if (reg_type == zm->reg_type)
	    return zm->maptab;
    }
    *name = '\0';
    switch (reg_type)
    {
    case 'w':
    case 'p':
	strcat (name, "string");
	break;
    case 'n':
	strcat (name, "numeric");
	break;
    case 'u':
	strcat (name, "urx");
	break;
    default:
	strcat (name, "null");
    }
    strcat (name, ".chr");

    zm = xmalloc (sizeof(*zm));
    zm->reg_type = reg_type;
    zm->next = zms->map_list;
    zms->map_list = zm;
    if (!(zm->maptab = chrmaptab_create (zms->tabpath, name, 0)))
	logf(LOG_WARN, "Failed to read character table %s", name);
    else
	logf(LOG_LOG, "Read table %s", name);
    return zm->maptab;
}

const char **zebra_maps_input (ZebraMaps zms, int reg_type,
			       const char **from, int len)
{
    static char str[2] = {0,0};
    static const char *buf[2] = {0,0};
    chrmaptab maptab;

    maptab = zebra_map_get (zms, reg_type);
    if (maptab)
	return chr_map_input(maptab, from, len);
	
    if (isalnum(**from))
    {
	str[0] = isupper(**from) ? tolower(**from) : **from;
	buf[0] = str;
    }
    else
	buf[0] = (char*) CHR_SPACE;
    (*from)++;
    return buf;
}

const char *zebra_maps_output(ZebraMaps zms, int reg_type, const char **from)
{
    chrmaptab maptab;
    unsigned char i = (unsigned char) **from;
    static char buf[2] = {0,0};

    maptab = zebra_map_get (zms, reg_type);
    if (maptab)
	return chr_map_output (maptab, from, 1);
    (*from)++;
    buf[0] = i;
    return buf;
}


/* ------------------------------------ */

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributesPlusTerm *zapt;
} AttrType;

static int attr_find (AttrType *src, oid_value *attributeSetP)
{
    while (src->major < src->zapt->num_attributes)
    {
        Z_AttributeElement *element;

        element = src->zapt->attributeList[src->major];
        if (src->type == *element->attributeType)
        {
            switch (element->which) 
            {
            case Z_AttributeValue_numeric:
                ++(src->major);
                if (element->attributeSet && attributeSetP)
                {
                    oident *attrset;

                    attrset = oid_getentbyoid (element->attributeSet);
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

                    attrset = oid_getentbyoid (element->attributeSet);
                    *attributeSetP = attrset->value;
                }
                return *element->value.complex->list[src->minor-1]->u.numeric;
            default:
                assert (0);
            }
        }
        ++(src->major);
    }
    return -1;
}

static void attr_init (AttrType *src, Z_AttributesPlusTerm *zapt,
                       int type)
{
    src->zapt = zapt;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

/* ------------------------------------ */

int zebra_maps_is_complete (ZebraMaps zms, int reg_type)
{
    if (reg_type == 'p')
	return 1;
    return 0;
}

int zebra_maps_attr (ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		     int *reg_type, char **search_type, int *complete_flag)
{
    AttrType completeness;
    AttrType structure;
    AttrType relation;
    int completeness_value;
    int structure_value;
    int relation_value;

    attr_init (&structure, zapt, 4);
    attr_init (&completeness, zapt, 6);
    attr_init (&relation, zapt, 2);

    completeness_value = attr_find (&completeness, NULL);
    structure_value = attr_find (&structure, NULL);

    if (completeness_value == 2 || completeness_value == 3)
	*complete_flag = 1;
    else
	*complete_flag = 0;
    *reg_type = 0;

    *search_type = "phrase";
    if (relation_value == 102)
	*search_type = "ranked";
    
    switch (structure_value)
    {
    case -1:
    case 1:   /* phrase */
    case 2:   /* word */
    case 3:   /* key */
    case 6:   /* word list */
    case 105: /* free-form-text */
    case 106: /* document-text */
    case 108: /* string */ 
	if (*complete_flag)
	    *reg_type = 'p';
	else
	    *reg_type = 'w';
	break;
    case 107: /* local-number */
	*search_type = "local";
	*reg_type = 0;
    case 109: /* numeric string */
	*reg_type = 'n';
        break;
    case 104: /* urx */
	*reg_type = 'u';
	break;
    default:
	return -1;
    }
    return 0;
}
