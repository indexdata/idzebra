/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebramap.c,v $
 * Revision 1.5  1997-11-19 10:22:14  adam
 * Bug fix (introduced by previous commit).
 *
 * Revision 1.4  1997/11/18 10:05:08  adam
 * Changed character map facility so that admin can specify character
 * mapping files for each register type, w, p, etc.
 *
 * Revision 1.3  1997/11/17 15:35:26  adam
 * Bug fix. Relation=relevance wasn't observed.
 *
 * Revision 1.2  1997/10/31 12:39:30  adam
 * Changed log message.
 *
 * Revision 1.1  1997/10/27 14:33:06  adam
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
    int completeness;
    chrmaptab maptab;
    const char *maptab_name;
    struct zebra_map *next;
};

struct zebra_maps {
    char *tabpath;
    NMEM nmem;
    struct zebra_map *map_list;
    char temp_map_str[2];
    const char *temp_map_ptr[2];
};

void zebra_maps_close (ZebraMaps zms)
{
    struct zebra_map *zm = zms->map_list;
    while (zm)
    {
	if (zm->maptab)
	    chrmaptab_destroy (zm->maptab);
	zm = zm->next;
    }
    nmem_destroy (zms->nmem);
    xfree (zms);
}

static void zebra_map_read (ZebraMaps zms, const char *name)
{
    FILE *f;
    char line[512];
    char *argv[10];
    int argc;
    struct zebra_map **zm = 0;

    if (!(f = yaz_path_fopen(zms->tabpath, name, "r")))
    {
	logf(LOG_WARN|LOG_ERRNO, "%s", name);
	return ;
    }
    while ((argc = readconf_line(f, line, 512, argv, 10)))
    {
	if (!strcmp (argv[0], "index") && argc == 2)
	{
	    if (!zm)
		zm = &zms->map_list;
	    else
		zm = &(*zm)->next;
	    *zm = nmem_malloc (zms->nmem, sizeof(**zm));
	    (*zm)->reg_type = argv[1][0];
	    (*zm)->maptab_name = NULL;
	    (*zm)->maptab = NULL;
	    (*zm)->completeness = 0;
	}
	else if (zm && !strcmp (argv[0], "charmap") && argc == 2)
	{
	    (*zm)->maptab_name = nmem_strdup (zms->nmem, argv[1]);
	}
	else if (zm && !strcmp (argv[0], "completeness") && argc == 2)
	{
	    (*zm)->completeness = atoi (argv[1]);
	}
    }
    if (zm)
	(*zm)->next = NULL;
    fclose (f);
}
static void zms_map_handle (void *p, const char *name, const char *value)
{
    ZebraMaps zms = p;
    
    zebra_map_read (zms, value);
}

ZebraMaps zebra_maps_open (const char *tabpath, Res res)
{
    ZebraMaps zms = xmalloc (sizeof(*zms));

    zms->nmem = nmem_create ();
    zms->tabpath = nmem_strdup (zms->nmem, tabpath);
    zms->map_list = NULL;

    zms->temp_map_str[0] = '\0';
    zms->temp_map_str[1] = '\0';

    zms->temp_map_ptr[0] = zms->temp_map_str;
    zms->temp_map_ptr[1] = NULL;
    
    if (!res_trav (res, "index", zms, zms_map_handle))
	zebra_map_read (zms, "default.idx");
    return zms;
}

chrmaptab zebra_map_get (ZebraMaps zms, int reg_type)
{
    struct zebra_map *zm;
    
    for (zm = zms->map_list; zm; zm = zm->next)
	if (reg_type == zm->reg_type)
	    break;
    if (!zm)
    {
	logf (LOG_WARN, "Unknown register type: %c", reg_type);
	return NULL;
    }
    if (!zm->maptab)
    {
	if (!strcmp (zm->maptab_name, "@"))
	    return NULL;
	if (!(zm->maptab = chrmaptab_create (zms->tabpath,
					     zm->maptab_name, 0)))
	    logf(LOG_WARN, "Failed to read character table %s",
		 zm->maptab_name);
	else
	    logf(LOG_DEBUG, "Read character table %s", zm->maptab_name);
    }
    return zm->maptab;
}

const char **zebra_maps_input (ZebraMaps zms, int reg_type,
			       const char **from, int len)
{
    chrmaptab maptab;

    maptab = zebra_map_get (zms, reg_type);
    if (maptab)
	return chr_map_input(maptab, from, len);
    
    zms->temp_map_str[0] = **from;

    (*from)++;
    return zms->temp_map_ptr;
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
    struct zebra_map *zm;
    
    for (zm = zms->map_list; zm; zm = zm->next)
	if (reg_type == zm->reg_type)
	    return zm->completeness;
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
    relation_value = attr_find (&relation, NULL);

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
