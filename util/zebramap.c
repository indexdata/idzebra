/*
 * Copyright (C) 1994-1999, Index Data 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebramap.c,v $
 * Revision 1.19  1999-11-30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.18  1999/10/15 08:27:46  adam
 * Fixed replace handler. 8-bit fix.
 *
 * Revision 1.17  1999/09/08 12:13:21  adam
 * Fixed minor bug "replace"-mappings. Removed some logging messages.
 *
 * Revision 1.16  1999/09/07 07:19:21  adam
 * Work on character mapping. Implemented replace rules.
 *
 * Revision 1.15  1999/05/26 07:49:14  adam
 * C++ compilation.
 *
 * Revision 1.14  1999/02/19 10:37:40  adam
 * Minor fix.
 *
 * Revision 1.13  1999/02/18 15:01:04  adam
 * Structure=key uses register type 0.
 *
 * Revision 1.12  1999/02/12 13:29:25  adam
 * Implemented position-flag for registers.
 *
 * Revision 1.11  1998/10/13 20:09:19  adam
 * Changed call to readconf_line.
 *
 * Revision 1.10  1998/06/23 15:33:37  adam
 * Added feature to specify sort criteria in query (type 7 specifies
 * sort flags).
 *
 * Revision 1.9  1998/04/02 14:35:30  adam
 * First version of Zebra that works with compiled ASN.1.
 *
 * Revision 1.8  1998/03/05 08:42:44  adam
 * Minor changes to zebramap data structures. Query mapping rules changed.
 *
 * Revision 1.7  1998/02/10 12:03:07  adam
 * Implemented Sort.
 *
 * Revision 1.6  1998/01/29 13:36:01  adam
 * Structure word-list, free-form-text and document-text all
 * trigger ranked search.
 *
 * Revision 1.5  1997/11/19 10:22:14  adam
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

#include <yaz/yaz-util.h>
#include <charmap.h>
#include <zebramap.h>

#define ZEBRA_MAP_TYPE_SORT  1
#define ZEBRA_MAP_TYPE_INDEX 2

#define ZEBRA_REPLACE_ANY  300

struct zm_token {
    int *token_from;
    char *token_to;
    int token_min;
    struct zm_token *next;
};

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
    struct zm_token *replace_tokens;
};

struct zebra_maps {
    char *tabpath;
    NMEM nmem;
    struct zebra_map *map_list;
    char temp_map_str[2];
    const char *temp_map_ptr[2];
    struct zebra_map **lookup_array;
    WRBUF wrbuf_1, wrbuf_2;
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
    wrbuf_free (zms->wrbuf_1, 1);
    wrbuf_free (zms->wrbuf_2, 1);
    nmem_destroy (zms->nmem);
    xfree (zms);
}

static void zebra_map_read (ZebraMaps zms, const char *name)
{
    FILE *f;
    char line[512];
    char *argv[10];
    int argc;
    int lineno = 0;
    struct zebra_map **zm = 0, *zp;

    if (!(f = yaz_path_fopen(zms->tabpath, name, "r")))
    {
	logf(LOG_WARN|LOG_ERRNO, "%s", name);
	return ;
    }
    while ((argc = readconf_line(f, &lineno, line, 512, argv, 10)))
    {
	if (!yaz_matchstr (argv[0], "index") && argc == 2)
	{
	    if (!zm)
		zm = &zms->map_list;
	    else
		zm = &(*zm)->next;
	    *zm = (struct zebra_map *) nmem_malloc (zms->nmem, sizeof(**zm));
	    (*zm)->reg_id = argv[1][0];
	    (*zm)->maptab_name = NULL;
	    (*zm)->maptab = NULL;
	    (*zm)->type = ZEBRA_MAP_TYPE_INDEX;
	    (*zm)->completeness = 0;
	    (*zm)->positioned = 1;
	    (*zm)->replace_tokens = 0;
	}
	else if (!yaz_matchstr (argv[0], "sort") && argc == 2)
	{
	    if (!zm)
		zm = &zms->map_list;
	    else
		zm = &(*zm)->next;
	    *zm = (struct zebra_map *) nmem_malloc (zms->nmem, sizeof(**zm));
	    (*zm)->reg_id = argv[1][0];
	    (*zm)->maptab_name = NULL;
	    (*zm)->type = ZEBRA_MAP_TYPE_SORT;
            (*zm)->u.sort.entry_size = 80;
	    (*zm)->maptab = NULL;
	    (*zm)->completeness = 0;
	    (*zm)->positioned = 0;
	    (*zm)->replace_tokens = 0;
	}
	else if (zm && !yaz_matchstr (argv[0], "charmap") && argc == 2)
	{
	    (*zm)->maptab_name = nmem_strdup (zms->nmem, argv[1]);
	}
	else if (zm && !yaz_matchstr (argv[0], "completeness") && argc == 2)
	{
	    (*zm)->completeness = atoi (argv[1]);
	}
	else if (zm && !yaz_matchstr (argv[0], "position") && argc == 2)
	{
	    (*zm)->positioned = atoi (argv[1]);
	}
        else if (zm && !yaz_matchstr (argv[0], "entrysize") && argc == 2)
        {
            if ((*zm)->type == ZEBRA_MAP_TYPE_SORT)
		(*zm)->u.sort.entry_size = atoi (argv[1]);
        }
        else if (zm && !yaz_matchstr (argv[0], "replace") && argc >= 2)
        {
	    struct zm_token *token = nmem_malloc (zms->nmem, sizeof(*token));
	    token->next = (*zm)->replace_tokens;
	    (*zm)->replace_tokens = token;
#if 0
	    logf (LOG_LOG, "replace %s", argv[1]);
#endif
	    token->token_from = 0;
            if (argc >= 2)
            {
	        char *cp = argv[1];
	        int *dp = token->token_from = (int *)
                    nmem_malloc (zms->nmem, (1+strlen(cp))*sizeof(int));
	        while (*cp)
		    if (*cp == '$')
		    {
		        *dp++ = ' ';
		        cp++;
		    }
		    else if (*cp == '.')
                    {
                        *dp++ = ZEBRA_REPLACE_ANY;
                        cp++;
                    }
                    else
		    {
		        *dp++ = zebra_prim(&cp);
#if 0
			logf (LOG_LOG, "  char %2X %c", dp[-1], dp[-1]);
#endif
		    }
	        *dp = '\0';
	    }
	    if (argc >= 3)
	    {
                char *cp = argv[2];
		char *dp = token->token_to =
                    nmem_malloc (zms->nmem, strlen(cp)+1);
		while (*cp)
		    if (*cp == '$')
		    {
			*dp++ = ' ';
			cp++;
		    }
		    else
			*dp++ = zebra_prim(&cp);
		*dp = '\0';
	    }
	    else
		token->token_to = 0;
        }
    }
    if (zm)
	(*zm)->next = NULL;
    fclose (f);

    for (zp = zms->map_list; zp; zp = zp->next)
	zms->lookup_array[zp->reg_id] = zp;
}

static void zms_map_handle (void *p, const char *name, const char *value)
{
    ZebraMaps zms = (ZebraMaps) p;
    
    zebra_map_read (zms, value);
}

ZebraMaps zebra_maps_open (Res res)
{
    ZebraMaps zms = (ZebraMaps) xmalloc (sizeof(*zms));
    int i;

    zms->nmem = nmem_create ();
    zms->tabpath = nmem_strdup (zms->nmem,
				res_get_def (res, "profilePath", "."));
    zms->map_list = NULL;

    zms->temp_map_str[0] = '\0';
    zms->temp_map_str[1] = '\0';

    zms->temp_map_ptr[0] = zms->temp_map_str;
    zms->temp_map_ptr[1] = NULL;

    zms->lookup_array = (struct zebra_map**)
	nmem_malloc (zms->nmem, sizeof(*zms->lookup_array)*256);
    for (i = 0; i<256; i++)
	zms->lookup_array[i] = 0;
    if (!res || !res_trav (res, "index", zms, zms_map_handle))
	zebra_map_read (zms, "default.idx");

    zms->wrbuf_1 = wrbuf_alloc();
    zms->wrbuf_2 = wrbuf_alloc();
    return zms;
}

struct zebra_map *zebra_map_get (ZebraMaps zms, unsigned reg_id)
{
    return zms->lookup_array[reg_id];
}

chrmaptab zebra_charmap_get (ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get (zms, reg_id);
    if (!zm)
    {
	zm = (struct zebra_map *) nmem_malloc (zms->nmem, sizeof(*zm));
	logf (LOG_WARN, "Unknown register type: %c", reg_id);

	zm->reg_id = reg_id;
	zm->maptab_name = NULL;
	zm->maptab = NULL;
	zm->type = ZEBRA_MAP_TYPE_INDEX;
	zm->completeness = 0;
	zm->next = zms->map_list;
	zms->map_list = zm->next;

	zms->lookup_array[zm->reg_id & 255] = zm;
    }
    if (!zm->maptab)
    {
	if (!zm->maptab_name || !yaz_matchstr (zm->maptab_name, "@"))
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

const char **zebra_maps_input (ZebraMaps zms, unsigned reg_id,
			       const char **from, int len)
{
    chrmaptab maptab;

    maptab = zebra_charmap_get (zms, reg_id);
    if (maptab)
	return chr_map_input(maptab, from, len);
    
    zms->temp_map_str[0] = **from;

    (*from)++;
    return zms->temp_map_ptr;
}

#if 0
int zebra_maps_input_tokens (ZebraMaps zms, unsigned reg_id,
			     const char *input_str, int input_len,
			     WRBUF wrbuf)
{
    chrmaptab maptab = zebra_charmap_get (zms, reg_id);
    int len[4];
    char *str[3];
    int input_i = 0;
    int first = 1;
    const char **out;
	
    if (!maptab)
    {
	wrbuf_write (wrbuf, input_str, input_len);
	return -1;
    }
    str[0] = " ";
    len[0] = 1;
    str[1] = input_str;
    len[1] = input_len;
    str[2] = " ";
    len[2] = 1;
    len[3] = -1;
    
    out = chr_map_input (maptab, str, len);
    while (len[1] > 0)
    {
	while (out && *out && **out == *CHR_SPACE)
	    out = chr_map_input (maptab, str, len);
    }
}
#endif

const char *zebra_maps_output(ZebraMaps zms, unsigned reg_id,
			      const char **from)
{
    chrmaptab maptab = zebra_charmap_get (zms, reg_id);
    if (!maptab)
	return 0;
    return chr_map_output (maptab, from, 1);
}


/* ------------------------------------ */

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributeElement **attributeList;
    int num_attributes;
} AttrType;

static int attr_find (AttrType *src, oid_value *attributeSetP)
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

static void attr_init_APT (AttrType *src, Z_AttributesPlusTerm *zapt, int type)
{
#ifdef ASN_COMPILED
    src->attributeList = zapt->attributes->attributes;
    src->num_attributes = zapt->attributes->num_attributes;
#else
    src->attributeList = zapt->attributeList;
    src->num_attributes = zapt->num_attributes;
#endif
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

static void attr_init_AttrList (AttrType *src, Z_AttributeList *list, int type)
{
    src->attributeList = list->attributes;
    src->num_attributes = list->num_attributes;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

/* ------------------------------------ */

int zebra_maps_is_complete (ZebraMaps zms, unsigned reg_id)
{ 
    struct zebra_map *zm = zebra_map_get (zms, reg_id);
    if (zm)
	return zm->completeness;
    return 0;
}

int zebra_maps_is_positioned (ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get (zms, reg_id);
    if (zm)
	return zm->positioned;
    return 0;
}
    
int zebra_maps_is_sort (ZebraMaps zms, unsigned reg_id)
{
    struct zebra_map *zm = zebra_map_get (zms, reg_id);
    if (zm)
	return zm->type == ZEBRA_MAP_TYPE_SORT;
    return 0;
}

int zebra_maps_sort (ZebraMaps zms, Z_SortAttributes *sortAttributes)
{
    AttrType use;
    attr_init_AttrList (&use, sortAttributes->list, 1);

    return attr_find (&use, NULL);
}

int zebra_maps_attr (ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		     unsigned *reg_id, char **search_type, char **rank_type,
		     int *complete_flag, int *sort_flag)
{
    AttrType completeness;
    AttrType structure;
    AttrType relation;
    AttrType sort_relation;
    int completeness_value;
    int structure_value;
    int relation_value;
    int sort_relation_value;

    attr_init_APT (&structure, zapt, 4);
    attr_init_APT (&completeness, zapt, 6);
    attr_init_APT (&relation, zapt, 2);
    attr_init_APT (&sort_relation, zapt, 7);

    completeness_value = attr_find (&completeness, NULL);
    structure_value = attr_find (&structure, NULL);
    relation_value = attr_find (&relation, NULL);
    sort_relation_value = attr_find (&sort_relation, NULL);

    if (completeness_value == 2 || completeness_value == 3)
	*complete_flag = 1;
    else
	*complete_flag = 0;
    *reg_id = 0;

    *sort_flag = (sort_relation_value > 0) ? 1 : 0;
    *search_type = "phrase";
    *rank_type = "void";
    if (relation_value == 102)
	*rank_type = "rank";
    
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
    default:
	return -1;
    }
    return 0;
}

int zebra_replace_sub(ZebraMaps zms, unsigned reg_id, const char *ex_list,
		      const char *input_str, int input_len, WRBUF wrbuf);

WRBUF zebra_replace(ZebraMaps zms, unsigned reg_id, const char *ex_list,
		    const char *input_str, int input_len)
{
    struct zebra_map *zm = zebra_map_get (zms, reg_id);

    wrbuf_rewind(zms->wrbuf_1);
    wrbuf_write(zms->wrbuf_1, input_str, input_len);
    if (!zm->replace_tokens)
	return zms->wrbuf_1;
  
#if 0
    logf (LOG_LOG, "in:%.*s:", wrbuf_len(zms->wrbuf_1),
	  wrbuf_buf(zms->wrbuf_1));
#endif
    for (;;)
    {
	if (!zebra_replace_sub(zms, reg_id, ex_list, wrbuf_buf(zms->wrbuf_1),
			       wrbuf_len(zms->wrbuf_1), zms->wrbuf_2))
	    return zms->wrbuf_2;
	if (!zebra_replace_sub(zms, reg_id, ex_list, wrbuf_buf(zms->wrbuf_2),
			       wrbuf_len(zms->wrbuf_2), zms->wrbuf_1))
	    return zms->wrbuf_1;
    }
    return 0;
}

int zebra_replace_sub(ZebraMaps zms, unsigned reg_id, const char *ex_list,
		      const char *input_str, int input_len, WRBUF wrbuf)
{
    int i = -1;
    int no_replaces = 0;
    struct zebra_map *zm = zebra_map_get (zms, reg_id);

    wrbuf_rewind(wrbuf);
    for (i = -1; i <= input_len; )
    {
	struct zm_token *token;
	char replace_string[128];
	int replace_out;
	int replace_in = 0;

	for (token = zm->replace_tokens; !replace_in && token;
	     token = token->next)
	{
	    int j = 0;
	    int replace_done = 0;
	    replace_out = 0;
	    for (;; j++)
	    {
		int c;
		if (!token->token_from[j])
		{
		    replace_in = j;
		    break;
		}
		if (ex_list && strchr (ex_list, token->token_from[j]))
		    break;
		if (i+j < 0 || j+i >= input_len)
		    c = ' ';
		else
		    c = input_str[j+i] & 255;
		if (token->token_from[j] == ZEBRA_REPLACE_ANY)
		{
		    if (c == ' ')
			break;
		    replace_string[replace_out++] = c;
		}
		else
		{
		    if (c != token->token_from[j])
		    {
			break;
		    }
		    if (!replace_done)
		    {
			const char *cp = token->token_to;
			replace_done = 1;
			for (; cp && *cp; cp++)
			    replace_string[replace_out++] = *cp;
		    }
		}
	    }
	}
	if (!replace_in)
	{
	    if (i >= 0 && i < input_len)
		wrbuf_putc(wrbuf, input_str[i]);
	    i++;
	}
	else
	{
	    no_replaces++;
	    if (replace_out)
		wrbuf_write(wrbuf, replace_string, replace_out);
	    i += replace_in;
	}
    }
#if 0
    logf (LOG_LOG, "out:%.*s:", wrbuf_len(wrbuf), wrbuf_buf(wrbuf));
#endif
    return no_replaces;
}
