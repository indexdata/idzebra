/*
 * Copyright (C) 1995-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: zrpn.c,v 1.112 2002-04-04 14:14:13 adam Exp $
 */
#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <ctype.h>

#include "index.h"

#include <charmap.h>
#include <rstemp.h>
#include <rsnull.h>
#include <rsbool.h>

struct rpn_char_map_info {
    ZebraMaps zm;
    int reg_type;
};

static const char **rpn_char_map_handler (void *vp, const char **from, int len)
{
    struct rpn_char_map_info *p = (struct rpn_char_map_info *) vp;
    return zebra_maps_input (p->zm, p->reg_type, from, len);
}

static void rpn_char_map_prepare (struct zebra_register *reg, int reg_type,
				  struct rpn_char_map_info *map_info)
{
    map_info->zm = reg->zebra_maps;
    map_info->reg_type = reg_type;
    dict_grep_cmap (reg->dict, map_info, rpn_char_map_handler);
}

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributesPlusTerm *zapt;
} AttrType;

static int attr_find_ex (AttrType *src, oid_value *attributeSetP,
			 const char **string_value)
{
    int num_attributes;

#ifdef ASN_COMPILED
    num_attributes = src->zapt->attributes->num_attributes;
#else
    num_attributes = src->zapt->num_attributes;
#endif
    while (src->major < num_attributes)
    {
        Z_AttributeElement *element;

#ifdef ASN_COMPILED
        element = src->zapt->attributes->attributes[src->major];
#else
        element = src->zapt->attributeList[src->major];
#endif
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
                if (src->minor >= element->value.complex->num_list)
		    break;
                if (element->value.complex->list[src->minor]->which ==  
                    Z_StringOrNumeric_numeric)
		{
		    ++(src->minor);
		    if (element->attributeSet && attributeSetP)
		    {
			oident *attrset;
			
			attrset = oid_getentbyoid (element->attributeSet);
			*attributeSetP = attrset->value;
		    }
		    return
			*element->value.complex->list[src->minor-1]->u.numeric;
		}
		else if (element->value.complex->list[src->minor]->which ==  
			 Z_StringOrNumeric_string)
		{
		    if (!string_value)
			break;
		    ++(src->minor);
		    *string_value = 
			element->value.complex->list[src->minor-1]->u.string;
		    return -2;
		}
		else
		    break;
            default:
                assert (0);
            }
        }
        ++(src->major);
    }
    return -1;
}

static int attr_find (AttrType *src, oid_value *attributeSetP)
{
    return attr_find_ex (src, attributeSetP, 0);
}

static void attr_init (AttrType *src, Z_AttributesPlusTerm *zapt,
                       int type)
{
    src->zapt = zapt;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

#define TERM_COUNT        
       
struct grep_info {        
#ifdef TERM_COUNT        
    int *term_no;        
#endif        
    ISAMS_P *isam_p_buf;
    int isam_p_size;        
    int isam_p_indx;
    ZebraHandle zh;
    int reg_type;
    ZebraSet termset;
};        

static void term_untrans  (ZebraHandle zh, int reg_type,
			   char *dst, const char *src)
{
    while (*src)
    {
        const char *cp = zebra_maps_output (zh->reg->zebra_maps,
					    reg_type, &src);
	if (!cp)
	    *dst++ = *src++;
	else
	    while (*cp)
		*dst++ = *cp++;
    }
    *dst = '\0';
}

static void add_isam_p (const char *name, const char *info,
			struct grep_info *p)
{
    if (p->isam_p_indx == p->isam_p_size)
    {
        ISAMS_P *new_isam_p_buf;
#ifdef TERM_COUNT        
        int *new_term_no;        
#endif
        p->isam_p_size = 2*p->isam_p_size + 100;
        new_isam_p_buf = (ISAMS_P *) xmalloc (sizeof(*new_isam_p_buf) *
					     p->isam_p_size);
        if (p->isam_p_buf)
        {
            memcpy (new_isam_p_buf, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->isam_p_buf));
            xfree (p->isam_p_buf);
        }
        p->isam_p_buf = new_isam_p_buf;

#ifdef TERM_COUNT
        new_term_no = (int *) xmalloc (sizeof(*new_term_no) *
				       p->isam_p_size);
        if (p->term_no)
        {
            memcpy (new_term_no, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->term_no));
            xfree (p->term_no);
        }
        p->term_no = new_term_no;
#endif
    }
    assert (*info == sizeof(*p->isam_p_buf));
    memcpy (p->isam_p_buf + p->isam_p_indx, info+1, sizeof(*p->isam_p_buf));

#if 1
    if (p->termset)
    {
	const char *db;
	int set, use;
	char term_tmp[512];
	int su_code = 0;
	int len = key_SU_decode (&su_code, name);
	
	term_untrans  (p->zh, p->reg_type, term_tmp, name+len+1);
	logf (LOG_LOG, "grep: %d %c %s", su_code, name[len], term_tmp);
	zebraExplain_lookup_ord (p->zh->reg->zei,
				 su_code, &db, &set, &use);
	logf (LOG_LOG, "grep:  set=%d use=%d db=%s", set, use, db);
	
	resultSetAddTerm (p->zh, p->termset, name[len], db,
			  set, use, term_tmp);
    }
#endif
    (p->isam_p_indx)++;
}

static int grep_handle (char *name, const char *info, void *p)
{
    add_isam_p (name, info, (struct grep_info *) p);
    return 0;
}

static int term_pre (ZebraMaps zebra_maps, int reg_type, const char **src,
		     const char *ct1, const char *ct2)
{
    const char *s1, *s0 = *src;
    const char **map;

    /* skip white space */
    while (*s0)
    {
        if (ct1 && strchr (ct1, *s0))
            break;
        if (ct2 && strchr (ct2, *s0))
            break;
        s1 = s0;
        map = zebra_maps_input (zebra_maps, reg_type, &s1, strlen(s1));
        if (**map != *CHR_SPACE)
            break;
        s0 = s1;
    }
    *src = s0;
    return *s0;
}

/* term_100: handle term, where trunc=none (no operators at all) */
static int term_100 (ZebraMaps zebra_maps, int reg_type,
		     const char **src, char *dst, int space_split,
		     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    const char *space_start = 0;
    const char *space_end = 0;

    if (!term_pre (zebra_maps, reg_type, src, NULL, NULL))
        return 0;
    s0 = *src;
    while (*s0)
    {
        s1 = s0;
        map = zebra_maps_input (zebra_maps, reg_type, &s0, strlen(s0));
	if (space_split)
	{
	    if (**map == *CHR_SPACE)
		break;
	}
	else  /* complete subfield only. */
	{
	    if (**map == *CHR_SPACE)
	    {   /* save space mapping for later  .. */
		space_start = s1;
		space_end = s0;
		continue;
	    }
	    else if (space_start)
	    {   /* reload last space */
		while (space_start < space_end)
		{
		    if (!isalnum (*space_start) && *space_start != '-')
			dst[i++] = '\\';
		    dst_term[j++] = *space_start;
		    dst[i++] = *space_start++;
		}
		/* and reset */
		space_start = space_end = 0;
	    }
	}
	/* add non-space char */
        while (s1 < s0)
        {
            if (!isalnum (*s1) && *s1 != '-')
                dst[i++] = '\\';
	    dst_term[j++] = *s1;
            dst[i++] = *s1++;
        }
    }
    dst[i] = '\0';
    dst_term[j] = '\0';
    *src = s0;
    return i;
}

/* term_101: handle term, where trunc=Process # */
static int term_101 (ZebraMaps zebra_maps, int reg_type,
		     const char **src, char *dst, int space_split,
		     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    if (!term_pre (zebra_maps, reg_type, src, "#", "#"))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '#')
        {
            dst[i++] = '.';
            dst[i++] = '*';
	    dst_term[j++] = *s0++;
        }
        else
        {
            s1 = s0;
            map = zebra_maps_input (zebra_maps, reg_type, &s0, strlen(s0));
            if (space_split && **map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (!isalnum (*s1))
                    dst[i++] = '\\';
		dst_term[j++] = *s1;
                dst[i++] = *s1++;
            }
        }
    }
    dst[i] = '\0';
    dst_term[j++] = '\0';
    *src = s0;
    return i;
}

/* term_103: handle term, where trunc=re-2 (regular expressions) */
static int term_103 (ZebraMaps zebra_maps, int reg_type, const char **src,
		     char *dst, int *errors, int space_split,
		     char *dst_term)
{
    int i = 0;
    int j = 0;
    const char *s0, *s1;
    const char **map;

    if (!term_pre (zebra_maps, reg_type, src, "^\\()[].*+?|", "("))
        return 0;
    s0 = *src;
    if (errors && *s0 == '+' && s0[1] && s0[2] == '+' && s0[3] &&
        isdigit (s0[1]))
    {
        *errors = s0[1] - '0';
        s0 += 3;
        if (*errors > 3)
            *errors = 3;
    }
    while (*s0)
    {
        if (strchr ("^\\()[].*+?|-", *s0))
	{
	    dst_term[j++] = *s0;
            dst[i++] = *s0++;
	}
        else
        {
            s1 = s0;
            map = zebra_maps_input (zebra_maps, reg_type, &s0, strlen(s0));
            if (**map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (!isalnum (*s1))
                    dst[i++] = '\\';
		dst_term[j++] = *s1;
                dst[i++] = *s1++;
            }
        }
    }
    dst[i] = '\0';
    dst_term[j] = '\0';
    *src = s0;
    return i;
}

/* term_103: handle term, where trunc=re-1 (regular expressions) */
static int term_102 (ZebraMaps zebra_maps, int reg_type, const char **src,
		     char *dst, int space_split, char *dst_term)
{
    return term_103 (zebra_maps, reg_type, src, dst, NULL, space_split,
		     dst_term);
}


/* term_104: handle term, where trunc=Process # and ! */
static int term_104 (ZebraMaps zebra_maps, int reg_type,
		     const char **src, char *dst, int space_split,
		     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    if (!term_pre (zebra_maps, reg_type, src, "#!", "#!"))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '#')
        {
            dst[i++] = '.';
            dst[i++] = '*';
	    dst_term[j++] = *s0++;
        }
        else if (*s0 == '!')
	{
            dst[i++] = '.';
	    dst_term[j++] = *s0++;
	}
        {
            s1 = s0;
            map = zebra_maps_input (zebra_maps, reg_type, &s0, strlen(s0));
            if (space_split && **map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (!isalnum (*s1))
                    dst[i++] = '\\';
		dst_term[j++] = *s1;
                dst[i++] = *s1++;
            }
        }
    }
    dst[i] = '\0';
    dst_term[j++] = '\0';
    *src = s0;
    return i;
}

/* term_105/106: handle term, where trunc=Process * and ! and right trunc */
static int term_105 (ZebraMaps zebra_maps, int reg_type,
		     const char **src, char *dst, int space_split,
		     char *dst_term, int right_truncate)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    if (!term_pre (zebra_maps, reg_type, src, "*!", "*!"))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '*')
        {
            dst[i++] = '.';
            dst[i++] = '*';
	    dst_term[j++] = *s0++;
        }
        else if (*s0 == '!')
	{
            dst[i++] = '.';
	    dst_term[j++] = *s0++;
	}
        {
            s1 = s0;
            map = zebra_maps_input (zebra_maps, reg_type, &s0, strlen(s0));
            if (space_split && **map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (!isalnum (*s1))
                    dst[i++] = '\\';
		dst_term[j++] = *s1;
                dst[i++] = *s1++;
            }
        }
    }
    if (right_truncate)
    {
        dst[i++] = '.';
        dst[i++] = '*';
    }
    dst[i] = '\0';
    
    dst_term[j++] = '\0';
    *src = s0;
    return i;
}


/* gen_regular_rel - generate regular expression from relation
 *  val:     border value (inclusive)
 *  islt:    1 if <=; 0 if >=.
 */
static void gen_regular_rel (char *dst, int val, int islt)
{
    int dst_p;
    int w, d, i;
    int pos = 0;
    char numstr[20];

    logf (LOG_DEBUG, "gen_regular_rel. val=%d, islt=%d", val, islt);
    if (val >= 0)
    {
        if (islt)
            strcpy (dst, "(-[0-9]+|(");
        else
            strcpy (dst, "((");
    } 
    else
    {
        if (!islt)
        {
            strcpy (dst, "([0-9]+|-(");
            dst_p = strlen (dst);
            islt = 1;
        }
        else
        {
            strcpy (dst, "(-(");
            islt = 0;
        }
        val = -val;
    }
    dst_p = strlen (dst);
    sprintf (numstr, "%d", val);
    for (w = strlen(numstr); --w >= 0; pos++)
    {
        d = numstr[w];
        if (pos > 0)
        {
            if (islt)
            {
                if (d == '0')
                    continue;
                d--;
            } 
            else
            {
                if (d == '9')
                    continue;
                d++;
            }
        }
        
        strcpy (dst + dst_p, numstr);
        dst_p = strlen(dst) - pos - 1;

        if (islt)
        {
            if (d != '0')
            {
                dst[dst_p++] = '[';
                dst[dst_p++] = '0';
                dst[dst_p++] = '-';
                dst[dst_p++] = d;
                dst[dst_p++] = ']';
            }
            else
                dst[dst_p++] = d;
        }
        else
        {
            if (d != '9')
            { 
                dst[dst_p++] = '[';
                dst[dst_p++] = d;
                dst[dst_p++] = '-';
                dst[dst_p++] = '9';
                dst[dst_p++] = ']';
            }
            else
                dst[dst_p++] = d;
        }
        for (i = 0; i<pos; i++)
        {
            dst[dst_p++] = '[';
            dst[dst_p++] = '0';
            dst[dst_p++] = '-';
            dst[dst_p++] = '9';
            dst[dst_p++] = ']';
        }
        dst[dst_p++] = '|';
    }
    dst[dst_p] = '\0';
    if (islt)
    {
	/* match everything less than 10^(pos-1) */
	strcat (dst, "0*");
	for (i=1; i<pos; i++)
	    strcat (dst, "[0-9]?");
    }
    else
    {
	/* match everything greater than 10^pos */
        for (i = 0; i <= pos; i++)
            strcat (dst, "[0-9]");
        strcat (dst, "[0-9]*");
    }
    strcat (dst, "))");
}

void string_rel_add_char (char **term_p, const char *src, int *indx)
{
    if (src[*indx] == '\\')
	*(*term_p)++ = src[(*indx)++];
    *(*term_p)++ = src[(*indx)++];
}

/*
 *   >  abc     ([b-].*|a[c-].*|ab[d-].*|abc.+)
 *              ([^-a].*|a[^-b].*ab[^-c].*|abc.+)
 *   >= abc     ([b-].*|a[c-].*|ab[c-].*)
 *              ([^-a].*|a[^-b].*|ab[c-].*)
 *   <  abc     ([-0].*|a[-a].*|ab[-b].*)
 *              ([^a-].*|a[^b-].*|ab[^c-].*)
 *   <= abc     ([-0].*|a[-a].*|ab[-b].*|abc)
 *              ([^a-].*|a[^b-].*|ab[^c-].*|abc)
 */
static int string_relation (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			    const char **term_sub, char *term_dict,
			    oid_value attributeSet,
			    int reg_type, int space_split, char *term_dst)
{
    AttrType relation;
    int relation_value;
    int i;
    char *term_tmp = term_dict + strlen(term_dict);
    char term_component[256];

    attr_init (&relation, zapt, 2);
    relation_value = attr_find (&relation, NULL);

    logf (LOG_DEBUG, "string relation value=%d", relation_value);
    switch (relation_value)
    {
    case 1:
        if (!term_100 (zh->reg->zebra_maps, reg_type,
		       term_sub, term_component,
		       space_split, term_dst))
            return 0;
        logf (LOG_DEBUG, "Relation <");
	
	*term_tmp++ = '(';
	for (i = 0; term_component[i]; )
	{
	    int j = 0;

	    if (i)
		*term_tmp++ = '|';
	    while (j < i)
		string_rel_add_char (&term_tmp, term_component, &j);

	    *term_tmp++ = '[';

	    *term_tmp++ = '^';
	    string_rel_add_char (&term_tmp, term_component, &i);
	    *term_tmp++ = '-';

	    *term_tmp++ = ']';
	    *term_tmp++ = '.';
	    *term_tmp++ = '*';
	}
	*term_tmp++ = ')';
	*term_tmp = '\0';
        break;
    case 2:
        if (!term_100 (zh->reg->zebra_maps, reg_type,
		       term_sub, term_component,
		       space_split, term_dst))
            return 0;
        logf (LOG_DEBUG, "Relation <=");

	*term_tmp++ = '(';
	for (i = 0; term_component[i]; )
	{
	    int j = 0;

	    while (j < i)
		string_rel_add_char (&term_tmp, term_component, &j);
	    *term_tmp++ = '[';

	    *term_tmp++ = '^';
	    string_rel_add_char (&term_tmp, term_component, &i);
	    *term_tmp++ = '-';

	    *term_tmp++ = ']';
	    *term_tmp++ = '.';
	    *term_tmp++ = '*';

	    *term_tmp++ = '|';
	}
	for (i = 0; term_component[i]; )
	    string_rel_add_char (&term_tmp, term_component, &i);
	*term_tmp++ = ')';
	*term_tmp = '\0';
        break;
    case 5:
        if (!term_100 (zh->reg->zebra_maps, reg_type,
		       term_sub, term_component, space_split, term_dst))
            return 0;
        logf (LOG_DEBUG, "Relation >");

	*term_tmp++ = '(';
	for (i = 0; term_component[i];)
	{
	    int j = 0;

	    while (j < i)
		string_rel_add_char (&term_tmp, term_component, &j);
	    *term_tmp++ = '[';
	    
	    *term_tmp++ = '^';
	    *term_tmp++ = '-';
	    string_rel_add_char (&term_tmp, term_component, &i);

	    *term_tmp++ = ']';
	    *term_tmp++ = '.';
	    *term_tmp++ = '*';

	    *term_tmp++ = '|';
	}
	for (i = 0; term_component[i];)
	    string_rel_add_char (&term_tmp, term_component, &i);
	*term_tmp++ = '.';
	*term_tmp++ = '+';
	*term_tmp++ = ')';
	*term_tmp = '\0';
        break;
    case 4:
        if (!term_100 (zh->reg->zebra_maps, reg_type, term_sub,
		       term_component, space_split, term_dst))
            return 0;
        logf (LOG_DEBUG, "Relation >=");

	*term_tmp++ = '(';
	for (i = 0; term_component[i];)
	{
	    int j = 0;

	    if (i)
		*term_tmp++ = '|';
	    while (j < i)
		string_rel_add_char (&term_tmp, term_component, &j);
	    *term_tmp++ = '[';

	    if (term_component[i+1])
	    {
		*term_tmp++ = '^';
		*term_tmp++ = '-';
		string_rel_add_char (&term_tmp, term_component, &i);
	    }
	    else
	    {
		string_rel_add_char (&term_tmp, term_component, &i);
		*term_tmp++ = '-';
	    }
	    *term_tmp++ = ']';
	    *term_tmp++ = '.';
	    *term_tmp++ = '*';
	}
	*term_tmp++ = ')';
	*term_tmp = '\0';
        break;
    case 3:
    default:
        logf (LOG_DEBUG, "Relation =");
        if (!term_100 (zh->reg->zebra_maps, reg_type, term_sub,
		       term_component, space_split, term_dst))
            return 0;
	strcat (term_tmp, "(");
	strcat (term_tmp, term_component);
	strcat (term_tmp, ")");
    }
    return 1;
}

static int string_term (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			const char **term_sub, 
			oid_value attributeSet, NMEM stream,
			struct grep_info *grep_info,
			int reg_type, int complete_flag,
			int num_bases, char **basenames,
			char *term_dst)
{
    char term_dict[2*IT_MAX_WORD+4000];
    int j, r, base_no;
    AttrType truncation;
    int truncation_value;
    AttrType use;
    int use_value;
    oid_value curAttributeSet = attributeSet;
    const char *termp;
    struct rpn_char_map_info rcmi;
    int space_split = complete_flag ? 0 : 1;

    rpn_char_map_prepare (zh->reg, reg_type, &rcmi);
    attr_init (&use, zapt, 1);
    use_value = attr_find (&use, &curAttributeSet);
    logf (LOG_DEBUG, "string_term, use value %d", use_value);
    attr_init (&truncation, zapt, 5);
    truncation_value = attr_find (&truncation, NULL);
    logf (LOG_DEBUG, "truncation value %d", truncation_value);

    if (use_value == -1)
        use_value = 1016;

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        attent attp;
        data1_local_attribute *local_attr;
        int max_pos, prefix_len = 0;

        termp = *term_sub;
        if ((r=att_getentbyatt (zh, &attp, curAttributeSet, use_value)))
        {
            logf (LOG_DEBUG, "att_getentbyatt fail. set=%d use=%d r=%d",
                  curAttributeSet, use_value, r);
	    if (r == -1)
	    {
		char val_str[32];
		sprintf (val_str, "%d", use_value);
		zh->errCode = 114;
		zh->errString = nmem_strdup (stream, val_str);
	    }
	    else
	    {
		int oid[OID_SIZE];
		struct oident oident;

		oident.proto = PROTO_Z3950;
		oident.oclass = CLASS_ATTSET;
		oident.value = curAttributeSet;
		oid_ent_to_oid (&oident, oid);

		zh->errCode = 121;
		zh->errString = nmem_strdup (stream, oident.desc);
	    }
            return -1;
        }
        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
            zh->errCode = 109; /* Database unavailable */
            zh->errString = basenames[base_no];
            return -1;
        }
        for (local_attr = attp.local_attributes; local_attr;
             local_attr = local_attr->next)
        {
            int ord;
	    char ord_buf[32];
	    int i, ord_len;

            ord = zebraExplain_lookupSU (zh->reg->zei, attp.attset_ordinal,
                                          local_attr->local);
            if (ord < 0)
                continue;
            if (prefix_len)
                term_dict[prefix_len++] = '|';
            else
                term_dict[prefix_len++] = '(';

	    ord_len = key_SU_encode (ord, ord_buf);
	    for (i = 0; i<ord_len; i++)
	    {
		term_dict[prefix_len++] = 1;
		term_dict[prefix_len++] = ord_buf[i];
	    }
        }
        if (!prefix_len)
        {
	    char val_str[32];
	    sprintf (val_str, "%d", use_value);
	    zh->errCode = 114;
	    zh->errString = nmem_strdup (stream, val_str);
            return -1;
        }
        term_dict[prefix_len++] = ')';        
        term_dict[prefix_len++] = 1;
        term_dict[prefix_len++] = reg_type;
	logf (LOG_DEBUG, "reg_type = %d", term_dict[prefix_len-1]);
        term_dict[prefix_len] = '\0';
	j = prefix_len;
	switch (truncation_value)
	{
	case -1:         /* not specified */
	case 100:        /* do not truncate */
	    if (!string_relation (zh, zapt, &termp, term_dict,
				  attributeSet,
				  reg_type, space_split, term_dst))
		return 0;
	    logf (LOG_DEBUG, "dict_lookup_grep: %s", term_dict+prefix_len);
	    r = dict_lookup_grep (zh->reg->dict, term_dict, 0,
				  grep_info, &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep fail, rel=gt: %d", r);
	    break;
	case 1:          /* right truncation */
	    term_dict[j++] = '(';
	    if (!term_100 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ".*)");
	    dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
			      &max_pos, 0, grep_handle);
	    break;
	case 2:          /* keft truncation */
	    term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
	    if (!term_100 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
			      &max_pos, 0, grep_handle);
	    break;
	case 3:          /* left&right truncation */
	    term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
	    if (!term_100 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ".*)");
	    dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
			      &max_pos, 0, grep_handle);
	    break;
	    zh->errCode = 120;
	    return -1;
	case 101:        /* process # in term */
	    term_dict[j++] = '(';
	    if (!term_101 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    r = dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=#: %d", r);
	    break;
	case 102:        /* Regexp-1 */
	    term_dict[j++] = '(';
	    if (!term_102 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    logf (LOG_DEBUG, "Regexp-1 tolerance=%d", r);
	    r = dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=regular: %d",
		      r);
	    break;
	case 103:       /* Regexp-2 */
	    r = 1;
	    term_dict[j++] = '(';
	    if (!term_103 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, &r, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    logf (LOG_DEBUG, "Regexp-2 tolerance=%d", r);
	    r = dict_lookup_grep (zh->reg->dict, term_dict, r, grep_info,
				  &max_pos, 2, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=eregular: %d",
		      r);
	    break;
	case 104:        /* process # and ! in term */
	    term_dict[j++] = '(';
	    if (!term_104 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    r = dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=#/!: %d", r);
	    break;
	case 105:        /* process * and ! in term */
	    term_dict[j++] = '(';
	    if (!term_105 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst, 1))
		return 0;
	    strcat (term_dict, ")");
	    r = dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=*/!: %d", r);
	    break;
	case 106:        /* process * and ! in term */
	    term_dict[j++] = '(';
	    if (!term_105 (zh->reg->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst, 0))
		return 0;
	    strcat (term_dict, ")");
	    r = dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=*/!: %d", r);
	    break;
        }
    }
    *term_sub = termp;
    logf (LOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static void trans_term (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                        char *termz)
{
    size_t sizez;
    Z_Term *term = zapt->term;

    sizez = term->u.general->len;
    if (sizez > IT_MAX_WORD-1)
        sizez = IT_MAX_WORD-1;
    memcpy (termz, term->u.general->buf, sizez);
    termz[sizez] = '\0';
}

static void trans_scan_term (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                             char *termz, int reg_type)
{
    Z_Term *term = zapt->term;
    const char **map;
    const char *cp = (const char *) term->u.general->buf;
    const char *cp_end = cp + term->u.general->len;
    const char *src;
    int i = 0;
    const char *space_map = NULL;
    int len;
    
    while ((len = (cp_end - cp)) > 0)
    {
        map = zebra_maps_input (zh->reg->zebra_maps, reg_type, &cp, len);
        if (**map == *CHR_SPACE)
            space_map = *map;
        else
        {
            if (i && space_map)
                for (src = space_map; *src; src++)
                    termz[i++] = *src;
            space_map = NULL;
            for (src = *map; *src; src++)
                termz[i++] = *src;
        }
    }
    termz[i] = '\0';
}

static RSET rpn_prox (ZebraHandle zh, RSET *rset, int rset_no,
		      int ordered, int exclusion, int relation, int distance)
{
    int i;
    RSFD *rsfd;
    int  *more;
    struct it_key **buf;
    RSET result;
    char prox_term[1024];
    int length_prox_term = 0;
    int min_nn = 10000000;
    int term_index;
    const char *flags = NULL;
    
    rsfd = (RSFD *) xmalloc (sizeof(*rsfd)*rset_no);
    more = (int *) xmalloc (sizeof(*more)*rset_no);
    buf = (struct it_key **) xmalloc (sizeof(*buf)*rset_no);

    *prox_term = '\0';
    for (i = 0; i<rset_no; i++)
    {
	int j;
	for (j = 0; j<rset[i]->no_rset_terms; j++)
	{
	    const char *nflags = rset[i]->rset_terms[j]->flags;
	    char *term = rset[i]->rset_terms[j]->name;
	    int lterm = strlen(term);
	    if (lterm + length_prox_term < sizeof(prox_term)-1)
	    {
		if (length_prox_term)
		    prox_term[length_prox_term++] = ' ';
		strcpy (prox_term + length_prox_term, term);
		length_prox_term += lterm;
	    }
	    if (min_nn > rset[i]->rset_terms[j]->nn)
		min_nn = rset[i]->rset_terms[j]->nn;
	    flags = nflags;
	}
    }
    for (i = 0; i<rset_no; i++)
    {
	buf[i] = 0;
	rsfd[i] = 0;
    }
    for (i = 0; i<rset_no; i++)
    {
	buf[i] = (struct it_key *) xmalloc (sizeof(**buf));
	rsfd[i] = rset_open (rset[i], RSETF_READ);
        if (!(more[i] = rset_read (rset[i], rsfd[i], buf[i], &term_index)))
	    break;
    }
    if (i != rset_no)
    {
	/* at least one is empty ... return null set */
	rset_null_parms parms;
	
	parms.rset_term = rset_term_create (prox_term, length_prox_term,
					    flags);
	parms.rset_term->nn = 0;
	result = rset_create (rset_kind_null, &parms);
    }
    else if (ordered && relation == 3 && exclusion == 0 && distance == 1)
    {
	/* special proximity case = phrase search ... */
	rset_temp_parms parms;
	RSFD rsfd_result;

	parms.rset_term = rset_term_create (prox_term, length_prox_term,
					    flags);
	parms.rset_term->nn = min_nn;
        parms.cmp = key_compare_it;
	parms.key_size = sizeof (struct it_key);
	parms.temp_path = res_get (zh->res, "setTmpDir");
	result = rset_create (rset_kind_temp, &parms);
	rsfd_result = rset_open (result, RSETF_WRITE);
	
	while (*more)
	{
	    for (i = 1; i<rset_no; i++)
	    {
		int cmp;
		
		if (!more[i])
		{
		    *more = 0;
		    break;
		}
		cmp = key_compare_it (buf[i], buf[i-1]);
		if (cmp > 1)
		{
		    more[i-1] = rset_read (rset[i-1], rsfd[i-1],
					   buf[i-1], &term_index);
		    break;
		}
		else if (cmp == 1)
		{
		    if (buf[i-1]->seqno+1 != buf[i]->seqno)
		    {
			more[i-1] = rset_read (rset[i-1], rsfd[i-1],
					       buf[i-1], &term_index);
			break;
		    }
		}
		else
		{
		    more[i] = rset_read (rset[i], rsfd[i], buf[i],
					 &term_index);
		    break;
		}
	    }
	    if (i == rset_no)
	    {
		rset_write (result, rsfd_result, buf[0]);
		more[0] = rset_read (*rset, *rsfd, *buf, &term_index);
	    }
	}
	rset_close (result, rsfd_result);
    }
    else if (rset_no == 2)
    {
	/* generic proximity case (two input sets only) ... */
	rset_temp_parms parms;
	RSFD rsfd_result;

	logf (LOG_LOG, "generic prox, dist = %d, relation = %d, ordered =%d, exclusion=%d",
	      distance, relation, ordered, exclusion);
	parms.rset_term = rset_term_create (prox_term, length_prox_term,
					    flags);
	parms.rset_term->nn = min_nn;
        parms.cmp = key_compare_it;
	parms.key_size = sizeof (struct it_key);
	parms.temp_path = res_get (zh->res, "setTmpDir");
	result = rset_create (rset_kind_temp, &parms);
	rsfd_result = rset_open (result, RSETF_WRITE);

	while (more[0] && more[1]) 
	{
	    int cmp = key_compare_it (buf[0], buf[1]);
	    if (cmp < -1)
		more[0] = rset_read (rset[0], rsfd[0], buf[0], &term_index);
	    else if (cmp > 1)
		more[1] = rset_read (rset[1], rsfd[1], buf[1], &term_index);
	    else
	    {
		int sysno = buf[0]->sysno;
		int seqno[500];
		int n = 0;
		
		seqno[n++] = buf[0]->seqno;
		while ((more[0] = rset_read (rset[0], rsfd[0], buf[0],
					     &term_index)) &&
		       sysno == buf[0]->sysno)
		    if (n < 500)
			seqno[n++] = buf[0]->seqno;
		do
		{
		    for (i = 0; i<n; i++)
		    {
			int diff = buf[1]->seqno - seqno[i];
			int excl = exclusion;
			if (!ordered && diff < 0)
			    diff = -diff;
			switch (relation)
			{
			case 1:      /* < */
			    if (diff < distance && diff >= 0)
				excl = !excl;
			    break;
			case 2:      /* <= */
			    if (diff <= distance && diff >= 0)
				excl = !excl;
			    break;
			case 3:      /* == */
			    if (diff == distance && diff >= 0)
				excl = !excl;
			    break;
			case 4:      /* >= */
			    if (diff >= distance && diff >= 0)
				excl = !excl;
			    break;
			case 5:      /* > */
			    if (diff > distance && diff >= 0)
				excl = !excl;
			    break;
			case 6:      /* != */
			    if (diff != distance && diff >= 0)
				excl = !excl;
			    break;
			}
			if (excl)
			{
			    rset_write (result, rsfd_result, buf[1]);
			    break;
			}
		    }
		} while ((more[1] = rset_read (rset[1], rsfd[1], buf[1],
					       &term_index)) &&
			 sysno == buf[1]->sysno);
	    }
	}
	rset_close (result, rsfd_result);
    }
    else
    {
	rset_null_parms parms;
	
	parms.rset_term = rset_term_create (prox_term, length_prox_term,
					    flags);
	parms.rset_term->nn = 0;
	result = rset_create (rset_kind_null, &parms);
    }
    for (i = 0; i<rset_no; i++)
    {
	if (rsfd[i])
	    rset_close (rset[i], rsfd[i]);
	xfree (buf[i]);
    }
    xfree (buf);
    xfree (more);
    xfree (rsfd);
    return result;
}


char *normalize_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
		     const char *termz, NMEM stream, unsigned reg_id)
{
    WRBUF wrbuf = 0;
    AttrType truncation;
    int truncation_value;
    char *ex_list = 0;

    attr_init (&truncation, zapt, 5);
    truncation_value = attr_find (&truncation, NULL);

    switch (truncation_value)
    {
    default:
	ex_list = "";
	break;
    case 101:
	ex_list = "#";
	break;
    case 102:
    case 103:
	ex_list = 0;
	break;
    case 104:
	ex_list = "!#";
	break;
    case 105:
	ex_list = "!*";
	break;
    }
    if (ex_list)
	wrbuf = zebra_replace(zh->reg->zebra_maps, reg_id, ex_list,
			      termz, strlen(termz));
    if (!wrbuf)
	return nmem_strdup(stream, termz);
    else
    {
	char *buf = (char*) nmem_malloc (stream, wrbuf_len(wrbuf)+1);
	memcpy (buf, wrbuf_buf(wrbuf), wrbuf_len(wrbuf));
	buf[wrbuf_len(wrbuf)] = '\0';
	return buf;
    }
}

static int grep_info_prepare (ZebraHandle zh,
			      Z_AttributesPlusTerm *zapt,
			      struct grep_info *grep_info,
			      int reg_type,
			      NMEM stream)
{
    AttrType termset;
    int termset_value_numeric;
    const char *termset_value_string;

#ifdef TERM_COUNT
    grep_info->term_no = 0;
#endif
    grep_info->isam_p_size = 0;
    grep_info->isam_p_buf = NULL;
    grep_info->zh = zh;
    grep_info->reg_type = reg_type;
    grep_info->termset = 0;

    attr_init (&termset, zapt, 8);
    termset_value_numeric =
	attr_find_ex (&termset, NULL, &termset_value_string);
    if (termset_value_numeric != -1)
    {
	char resname[32];
	const char *termset_name = 0;
	if (termset_value_numeric != -2)
	{
    
	    sprintf (resname, "%d", termset_value_numeric);
	    termset_name = resname;
	}
	else
	    termset_name = termset_value_string;
	logf (LOG_LOG, "creating termset set %s", termset_name);
	grep_info->termset = resultSetAdd (zh, termset_name, 1);
	if (!grep_info->termset)
	{
	    zh->errCode = 128;
	    zh->errString = nmem_strdup (stream, termset_name);
	    return -1;
	}
    }
    return 0;
}
			       

static RSET rpn_search_APT_phrase (ZebraHandle zh,
                                   Z_AttributesPlusTerm *zapt,
				   const char *termz_org,
                                   oid_value attributeSet,
				   NMEM stream,
				   int reg_type, int complete_flag,
				   const char *rank_type,
				   int num_bases, char **basenames)
{
    char term_dst[IT_MAX_WORD+1];
    RSET rset[60], result;
    int i, r, rset_no = 0;
    struct grep_info grep_info;
    char *termz = normalize_term(zh, zapt, termz_org, stream, reg_type);
    const char *termp = termz;

    *term_dst = 0;
    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
	return 0;
    while (1)
    { 
	logf (LOG_DEBUG, "APT_phrase termp=%s", termp);
	grep_info.isam_p_indx = 0;
        r = string_term (zh, zapt, &termp, attributeSet, stream, &grep_info,
			reg_type, complete_flag, num_bases, basenames,
			term_dst);
        if (r < 1)
            break;
	logf (LOG_DEBUG, "term: %s", term_dst);
        rset[rset_no] = rset_trunc (zh, grep_info.isam_p_buf,
                                    grep_info.isam_p_indx, term_dst,
				    strlen(term_dst), rank_type);
        assert (rset[rset_no]);
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
#ifdef TERM_COUNT
    xfree(grep_info.term_no);
#endif
    xfree (grep_info.isam_p_buf);
    if (rset_no == 0)
    {
	rset_null_parms parms;
	
	parms.rset_term = rset_term_create (termz, -1, rank_type);
        return rset_create (rset_kind_null, &parms);
    }
    else if (rset_no == 1)
        return (rset[0]);
    result = rpn_prox (zh, rset, rset_no, 1, 0, 3, 1);
    for (i = 0; i<rset_no; i++)
        rset_delete (rset[i]);
    return result;
}

static RSET rpn_search_APT_or_list (ZebraHandle zh,
                                    Z_AttributesPlusTerm *zapt,
				    const char *termz_org,
                                    oid_value attributeSet,
				    NMEM stream,
				    int reg_type, int complete_flag,
				    const char *rank_type,
				    int num_bases, char **basenames)
{
    char term_dst[IT_MAX_WORD+1];
    RSET rset[60], result;
    int i, r, rset_no = 0;
    struct grep_info grep_info;
    char *termz = normalize_term(zh, zapt, termz_org, stream, reg_type);
    const char *termp = termz;

    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
	return 0;
    while (1)
    { 
	logf (LOG_DEBUG, "APT_or_list termp=%s", termp);
	grep_info.isam_p_indx = 0;
        r = string_term (zh, zapt, &termp, attributeSet, stream, &grep_info,
			reg_type, complete_flag, num_bases, basenames,
			term_dst);
        if (r < 1)
            break;
	logf (LOG_DEBUG, "term: %s", term_dst);
        rset[rset_no] = rset_trunc (zh, grep_info.isam_p_buf,
                                    grep_info.isam_p_indx, term_dst,
				    strlen(term_dst), rank_type);
        assert (rset[rset_no]);
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
#ifdef TERM_COUNT
    xfree(grep_info.term_no);
#endif
    xfree (grep_info.isam_p_buf);
    if (rset_no == 0)
    {
	rset_null_parms parms;
	
	parms.rset_term = rset_term_create (termz, -1, rank_type);
        return rset_create (rset_kind_null, &parms);
    }
    result = rset[0];
    for (i = 1; i<rset_no; i++)
    {
        rset_bool_parms bool_parms;

        bool_parms.rset_l = result;
        bool_parms.rset_r = rset[i];
        bool_parms.key_size = sizeof(struct it_key);
	bool_parms.cmp = key_compare_it;
        result = rset_create (rset_kind_or, &bool_parms);
    }
    return result;
}

static RSET rpn_search_APT_and_list (ZebraHandle zh,
                                     Z_AttributesPlusTerm *zapt,
				     const char *termz_org,
                                     oid_value attributeSet,
				     NMEM stream,
				     int reg_type, int complete_flag,
				     const char *rank_type,
				     int num_bases, char **basenames)
{
    char term_dst[IT_MAX_WORD+1];
    RSET rset[60], result;
    int i, r, rset_no = 0;
    struct grep_info grep_info;
    char *termz = normalize_term(zh, zapt, termz_org, stream, reg_type);
    const char *termp = termz;

    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
	return 0;
    while (1)
    { 
	logf (LOG_DEBUG, "APT_and_list termp=%s", termp);
	grep_info.isam_p_indx = 0;
        r = string_term (zh, zapt, &termp, attributeSet, stream, &grep_info,
			reg_type, complete_flag, num_bases, basenames,
			term_dst);
        if (r < 1)
            break;
	logf (LOG_DEBUG, "term: %s", term_dst);
        rset[rset_no] = rset_trunc (zh, grep_info.isam_p_buf,
                                    grep_info.isam_p_indx, term_dst,
				    strlen(term_dst), rank_type);
        assert (rset[rset_no]);
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
#ifdef TERM_COUNT
    xfree(grep_info.term_no);
#endif
    xfree (grep_info.isam_p_buf);
    if (rset_no == 0)
    {
	rset_null_parms parms;
	
	parms.rset_term = rset_term_create (termz, -1, rank_type);
        return rset_create (rset_kind_null, &parms);
    }
    result = rset[0];
    for (i = 1; i<rset_no; i++)
    {
        rset_bool_parms bool_parms;

        bool_parms.rset_l = result;
        bool_parms.rset_r = rset[i];
        bool_parms.key_size = sizeof(struct it_key);
	bool_parms.cmp = key_compare_it;
        result = rset_create (rset_kind_and, &bool_parms);
    }
    return result;
}

static int numeric_relation (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			     const char **term_sub,
			     char *term_dict,
			     oid_value attributeSet,
			     struct grep_info *grep_info,
			     int *max_pos,
			     int reg_type,
			     char *term_dst)
{
    AttrType relation;
    int relation_value;
    int term_value;
    int r;
    char *term_tmp = term_dict + strlen(term_dict);

    attr_init (&relation, zapt, 2);
    relation_value = attr_find (&relation, NULL);

    logf (LOG_DEBUG, "numeric relation value=%d", relation_value);

    if (!term_100 (zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
		   term_dst))
	return 0;
    term_value = atoi (term_tmp);
    switch (relation_value)
    {
    case 1:
        logf (LOG_DEBUG, "Relation <");
        gen_regular_rel (term_tmp, term_value-1, 1);
        break;
    case 2:
        logf (LOG_DEBUG, "Relation <=");
        gen_regular_rel (term_tmp, term_value, 1);
        break;
    case 4:
        logf (LOG_DEBUG, "Relation >=");
        gen_regular_rel (term_tmp, term_value, 0);
        break;
    case 5:
        logf (LOG_DEBUG, "Relation >");
        gen_regular_rel (term_tmp, term_value+1, 0);
        break;
    case 3:
    default:
	logf (LOG_DEBUG, "Relation =");
	sprintf (term_tmp, "(0*%d)", term_value);
    }
    logf (LOG_DEBUG, "dict_lookup_grep: %s", term_tmp);
    r = dict_lookup_grep (zh->reg->dict, term_dict, 0, grep_info, max_pos,
                          0, grep_handle);
    if (r)
        logf (LOG_WARN, "dict_lookup_grep fail, rel=gt: %d", r);
    logf (LOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static int numeric_term (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			 const char **term_sub, 
			 oid_value attributeSet, struct grep_info *grep_info,
			 int reg_type, int complete_flag,
			 int num_bases, char **basenames,
			 char *term_dst)
{
    char term_dict[2*IT_MAX_WORD+2];
    int r, base_no;
    AttrType use;
    int use_value;
    oid_value curAttributeSet = attributeSet;
    const char *termp;
    struct rpn_char_map_info rcmi;

    rpn_char_map_prepare (zh->reg, reg_type, &rcmi);
    attr_init (&use, zapt, 1);
    use_value = attr_find (&use, &curAttributeSet);
    logf (LOG_DEBUG, "numeric_term, use value %d", use_value);

    if (use_value == -1)
        use_value = 1016;

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        attent attp;
        data1_local_attribute *local_attr;
        int max_pos, prefix_len = 0;

        termp = *term_sub;
        if ((r=att_getentbyatt (zh, &attp, curAttributeSet, use_value)))
        {
            logf (LOG_DEBUG, "att_getentbyatt fail. set=%d use=%d r=%d",
                  curAttributeSet, use_value, r);
	    if (r == -1)
		zh->errCode = 114;
	    else
		zh->errCode = 121;
            return -1;
        }
        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
            zh->errCode = 109; /* Database unavailable */
            zh->errString = basenames[base_no];
            return -1;
        }
        for (local_attr = attp.local_attributes; local_attr;
             local_attr = local_attr->next)
        {
            int ord;
	    char ord_buf[32];
	    int i, ord_len;

            ord = zebraExplain_lookupSU (zh->reg->zei, attp.attset_ordinal,
                                          local_attr->local);
            if (ord < 0)
                continue;
            if (prefix_len)
                term_dict[prefix_len++] = '|';
            else
                term_dict[prefix_len++] = '(';

	    ord_len = key_SU_encode (ord, ord_buf);
	    for (i = 0; i<ord_len; i++)
	    {
		term_dict[prefix_len++] = 1;
		term_dict[prefix_len++] = ord_buf[i];
	    }
        }
        if (!prefix_len)
        {
            zh->errCode = 114;
            return -1;
        }
        term_dict[prefix_len++] = ')';        
        term_dict[prefix_len++] = 1;
        term_dict[prefix_len++] = reg_type;
	logf (LOG_DEBUG, "reg_type = %d", term_dict[prefix_len-1]);
        term_dict[prefix_len] = '\0';
        if (!numeric_relation (zh, zapt, &termp, term_dict,
			       attributeSet, grep_info, &max_pos, reg_type,
			       term_dst))
	    return 0;
    }
    *term_sub = termp;
    logf (LOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static RSET rpn_search_APT_numeric (ZebraHandle zh,
				    Z_AttributesPlusTerm *zapt,
				    const char *termz,
				    oid_value attributeSet,
				    NMEM stream,
				    int reg_type, int complete_flag,
				    const char *rank_type,
				    int num_bases, char **basenames)
{
    char term_dst[IT_MAX_WORD+1];
    const char *termp = termz;
    RSET rset[60], result;
    int i, r, rset_no = 0;
    struct grep_info grep_info;

    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
	return 0;
    while (1)
    { 
	logf (LOG_DEBUG, "APT_numeric termp=%s", termp);
	grep_info.isam_p_indx = 0;
        r = numeric_term (zh, zapt, &termp, attributeSet, &grep_info,
			  reg_type, complete_flag, num_bases, basenames,
			  term_dst);
        if (r < 1)
            break;
	logf (LOG_DEBUG, "term: %s", term_dst);
        rset[rset_no] = rset_trunc (zh, grep_info.isam_p_buf,
                                    grep_info.isam_p_indx, term_dst,
				    strlen(term_dst), rank_type);
        assert (rset[rset_no]);
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
#ifdef TERM_COUNT
    xfree(grep_info.term_no);
#endif
    xfree (grep_info.isam_p_buf);
    if (rset_no == 0)
    {
	rset_null_parms parms;
	
	parms.rset_term = rset_term_create (term_dst, -1, rank_type);
        return rset_create (rset_kind_null, &parms);
    }
    result = rset[0];
    for (i = 1; i<rset_no; i++)
    {
        rset_bool_parms bool_parms;

        bool_parms.rset_l = result;
        bool_parms.rset_r = rset[i];
        bool_parms.key_size = sizeof(struct it_key);
	bool_parms.cmp = key_compare_it;
        result = rset_create (rset_kind_and, &bool_parms);
    }
    return result;
}

static RSET rpn_search_APT_local (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				  const char *termz,
                                  oid_value attributeSet,
				  NMEM stream,
				  const char *rank_type)
{
    RSET result;
    RSFD rsfd;
    struct it_key key;
    rset_temp_parms parms;

    parms.rset_term = rset_term_create (termz, -1, rank_type);
    parms.cmp = key_compare_it;
    parms.key_size = sizeof (struct it_key);
    parms.temp_path = res_get (zh->res, "setTmpDir");
    result = rset_create (rset_kind_temp, &parms);
    rsfd = rset_open (result, RSETF_WRITE);

    key.sysno = atoi (termz);
    key.seqno = 1;
    if (key.sysno <= 0)
        key.sysno = 1;
    rset_write (result, rsfd, &key);
    rset_close (result, rsfd);
    return result;
}

static RSET rpn_sort_spec (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			   oid_value attributeSet, NMEM stream,
			   Z_SortKeySpecList *sort_sequence,
			   const char *rank_type)
{
    rset_null_parms parms;    
    int i;
    int sort_relation_value;
    AttrType sort_relation_type;
    int use_value;
    AttrType use_type;
    Z_SortKeySpec *sks;
    Z_SortKey *sk;
    Z_AttributeElement *ae;
    int oid[OID_SIZE];
    oident oe;
    char termz[20];
    
    attr_init (&sort_relation_type, zapt, 7);
    sort_relation_value = attr_find (&sort_relation_type, &attributeSet);

    attr_init (&use_type, zapt, 1);
    use_value = attr_find (&use_type, &attributeSet);

    if (!sort_sequence->specs)
    {
	sort_sequence->num_specs = 10;
	sort_sequence->specs = (Z_SortKeySpec **)
	    nmem_malloc (stream, sort_sequence->num_specs *
			 sizeof(*sort_sequence->specs));
	for (i = 0; i<sort_sequence->num_specs; i++)
	    sort_sequence->specs[i] = 0;
    }
    if (zapt->term->which != Z_Term_general)
	i = 0;
    else
	i = atoi_n ((char *) zapt->term->u.general->buf,
		    zapt->term->u.general->len);
    if (i >= sort_sequence->num_specs)
	i = 0;
    sprintf (termz, "%d", i);

    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_ATTSET;
    oe.value = attributeSet;
    if (!oid_ent_to_oid (&oe, oid))
	return 0;

    sks = (Z_SortKeySpec *) nmem_malloc (stream, sizeof(*sks));
    sks->sortElement = (Z_SortElement *)
	nmem_malloc (stream, sizeof(*sks->sortElement));
    sks->sortElement->which = Z_SortElement_generic;
    sk = sks->sortElement->u.generic = (Z_SortKey *)
	nmem_malloc (stream, sizeof(*sk));
    sk->which = Z_SortKey_sortAttributes;
    sk->u.sortAttributes = (Z_SortAttributes *)
	nmem_malloc (stream, sizeof(*sk->u.sortAttributes));

    sk->u.sortAttributes->id = oid;
    sk->u.sortAttributes->list = (Z_AttributeList *)
	nmem_malloc (stream, sizeof(*sk->u.sortAttributes->list));
    sk->u.sortAttributes->list->num_attributes = 1;
    sk->u.sortAttributes->list->attributes = (Z_AttributeElement **)
	nmem_malloc (stream, sizeof(*sk->u.sortAttributes->list->attributes));
    ae = *sk->u.sortAttributes->list->attributes = (Z_AttributeElement *)
	nmem_malloc (stream, sizeof(**sk->u.sortAttributes->list->attributes));
    ae->attributeSet = 0;
    ae->attributeType =	(int *)
	nmem_malloc (stream, sizeof(*ae->attributeType));
    *ae->attributeType = 1;
    ae->which = Z_AttributeValue_numeric;
    ae->value.numeric = (int *)
	nmem_malloc (stream, sizeof(*ae->value.numeric));
    *ae->value.numeric = use_value;

    sks->sortRelation = (int *)
	nmem_malloc (stream, sizeof(*sks->sortRelation));
    if (sort_relation_value == 1)
	*sks->sortRelation = Z_SortRelation_ascending;
    else if (sort_relation_value == 2)
	*sks->sortRelation = Z_SortRelation_descending;
    else 
	*sks->sortRelation = Z_SortRelation_ascending;

    sks->caseSensitivity = (int *)
	nmem_malloc (stream, sizeof(*sks->caseSensitivity));
    *sks->caseSensitivity = 0;

#ifdef ASN_COMPILED
    sks->which = Z_SortKeySpec_null;
    sks->u.null = odr_nullval ();
#else
    sks->missingValueAction = 0;
#endif

    sort_sequence->specs[i] = sks;

    parms.rset_term = rset_term_create (termz, -1, rank_type);
    return rset_create (rset_kind_null, &parms);
}


static RSET rpn_search_APT (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                            oid_value attributeSet, NMEM stream,
			    Z_SortKeySpecList *sort_sequence,
                            int num_bases, char **basenames)
{
    unsigned reg_id;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    char termz[IT_MAX_WORD+1];

    zebra_maps_attr (zh->reg->zebra_maps, zapt, &reg_id, &search_type,
		     rank_type, &complete_flag, &sort_flag);
    
    logf (LOG_DEBUG, "reg_id=%c", reg_id);
    logf (LOG_DEBUG, "complete_flag=%d", complete_flag);
    logf (LOG_DEBUG, "search_type=%s", search_type);
    logf (LOG_DEBUG, "rank_type=%s", rank_type);

    if (zapt->term->which != Z_Term_general)
    {
        zh->errCode = 124;
        return NULL;
    }
    trans_term (zh, zapt, termz);

    if (sort_flag)
	return rpn_sort_spec (zh, zapt, attributeSet, stream, sort_sequence,
			      rank_type);

    if (!strcmp (search_type, "phrase"))
    {
	return rpn_search_APT_phrase (zh, zapt, termz, attributeSet, stream,
				      reg_id, complete_flag, rank_type,
				      num_bases, basenames);
    }
    else if (!strcmp (search_type, "and-list"))
    {
	return rpn_search_APT_and_list (zh, zapt, termz, attributeSet, stream,
			                reg_id, complete_flag, rank_type,
				        num_bases, basenames);
    }
    else if (!strcmp (search_type, "or-list"))
    {
	return rpn_search_APT_or_list (zh, zapt, termz, attributeSet, stream,
			               reg_id, complete_flag, rank_type,
				       num_bases, basenames);
    }
    else if (!strcmp (search_type, "local"))
    {
        return rpn_search_APT_local (zh, zapt, termz, attributeSet, stream,
				     rank_type);
    }
    else if (!strcmp (search_type, "numeric"))
    {
	return rpn_search_APT_numeric (zh, zapt, termz, attributeSet, stream,
			               reg_id, complete_flag, rank_type,
				       num_bases, basenames);
    }
    zh->errCode = 118;
    return NULL;
}

static RSET rpn_search_structure (ZebraHandle zh, Z_RPNStructure *zs,
                                  oid_value attributeSet, NMEM stream,
				  Z_SortKeySpecList *sort_sequence,
                                  int num_bases, char **basenames)
{
    RSET r = NULL;
    if (zs->which == Z_RPNStructure_complex)
    {
        Z_Operator *zop = zs->u.complex->roperator;
        rset_bool_parms bool_parms;

        bool_parms.rset_l = rpn_search_structure (zh, zs->u.complex->s1,
                                                  attributeSet, stream,
						  sort_sequence,
                                                  num_bases, basenames);
        if (bool_parms.rset_l == NULL)
            return NULL;
        bool_parms.rset_r = rpn_search_structure (zh, zs->u.complex->s2,
                                                  attributeSet, stream,
						  sort_sequence,
                                                  num_bases, basenames);
        if (bool_parms.rset_r == NULL)
        {
            rset_delete (bool_parms.rset_l);
            return NULL;
        }
        bool_parms.key_size = sizeof(struct it_key);
        bool_parms.cmp = key_compare_it;

        switch (zop->which)
        {
        case Z_Operator_and:
            r = rset_create (rset_kind_and, &bool_parms);
            break;
        case Z_Operator_or:
            r = rset_create (rset_kind_or, &bool_parms);
            break;
        case Z_Operator_and_not:
            r = rset_create (rset_kind_not, &bool_parms);
            break;
        case Z_Operator_prox:
#ifdef ASN_COMPILED
            if (zop->u.prox->which != Z_ProximityOperator_known)
            {
                zh->errCode = 132;
                return NULL;
            }
#else
            if (zop->u.prox->which != Z_ProxCode_known)
            {
                zh->errCode = 132;
                return NULL;
            }
#endif

#ifdef ASN_COMPILED
            if (*zop->u.prox->u.known != Z_ProxUnit_word)
            {
                char *val = (char *) nmem_malloc (stream, 16);
                zh->errCode = 132;
                zh->errString = val;
                sprintf (val, "%d", *zop->u.prox->u.known);
                return NULL;
            }
#else
            if (*zop->u.prox->proximityUnitCode != Z_ProxUnit_word)
            {
                char *val = (char *) nmem_malloc (stream, 16);
                zh->errCode = 132;
                zh->errString = val;
                sprintf (val, "%d", *zop->u.prox->proximityUnitCode);
                return NULL;
            }
#endif
	    else
	    {
		RSET rsets[2];

		rsets[0] = bool_parms.rset_l;
		rsets[1] = bool_parms.rset_r;
		
		r = rpn_prox (zh, rsets, 2, 
			      *zop->u.prox->ordered,
			      (!zop->u.prox->exclusion ? 0 :
			       *zop->u.prox->exclusion),
			      *zop->u.prox->relationType,
			      *zop->u.prox->distance);
		rset_delete (rsets[0]);
		rset_delete (rsets[1]);
	    }
            break;
        default:
            zh->errCode = 110;
            return NULL;
        }
    }
    else if (zs->which == Z_RPNStructure_simple)
    {
        if (zs->u.simple->which == Z_Operand_APT)
        {
            logf (LOG_DEBUG, "rpn_search_APT");
            r = rpn_search_APT (zh, zs->u.simple->u.attributesPlusTerm,
                                attributeSet, stream, sort_sequence,
				num_bases, basenames);
        }
        else if (zs->u.simple->which == Z_Operand_resultSetId)
        {
            logf (LOG_DEBUG, "rpn_search_ref");
            r = resultSetRef (zh, zs->u.simple->u.resultSetId);
	    if (!r)
	    {
		r = rset_create (rset_kind_null, NULL);
		zh->errCode = 30;
		zh->errString =
		    nmem_strdup (stream, zs->u.simple->u.resultSetId);
		return 0;
	    }
        }
        else
        {
            zh->errCode = 3;
            return 0;
        }
    }
    else
    {
        zh->errCode = 3;
        return 0;
    }
    return r;
}


RSET rpn_search (ZebraHandle zh, NMEM nmem,
		 Z_RPNQuery *rpn, int num_bases, char **basenames, 
		 const char *setname,
		 ZebraSet sset)
{
    RSET rset;
    oident *attrset;
    oid_value attributeSet;
    Z_SortKeySpecList *sort_sequence;
    int sort_status, i;

    zh->errCode = 0;
    zh->errString = NULL;
    zh->hits = 0;

    sort_sequence = (Z_SortKeySpecList *)
	nmem_malloc (nmem, sizeof(*sort_sequence));
    sort_sequence->num_specs = 10;
    sort_sequence->specs = (Z_SortKeySpec **)
	nmem_malloc (nmem, sort_sequence->num_specs *
		     sizeof(*sort_sequence->specs));
    for (i = 0; i<sort_sequence->num_specs; i++)
	sort_sequence->specs[i] = 0;
    
    attrset = oid_getentbyoid (rpn->attributeSetId);
    attributeSet = attrset->value;
    rset = rpn_search_structure (zh, rpn->RPNStructure, attributeSet,
				 nmem, sort_sequence, num_bases, basenames);
    if (!rset)
	return 0;

    if (zh->errCode)
        logf (LOG_DEBUG, "search error: %d", zh->errCode);
    
    for (i = 0; sort_sequence->specs[i]; i++)
	;
    sort_sequence->num_specs = i;
    if (!i)
	resultSetRank (zh, sset, rset);
    else
    {
	logf (LOG_DEBUG, "resultSetSortSingle in rpn_search");
	resultSetSortSingle (zh, nmem, sset, rset,
			     sort_sequence, &sort_status);
	if (zh->errCode)
	{
	    logf (LOG_DEBUG, "resultSetSortSingle status = %d", zh->errCode);
	}
    }
    return rset;
}

struct scan_info_entry {
    char *term;
    ISAMS_P isam_p;
};

struct scan_info {
    struct scan_info_entry *list;
    ODR odr;
    int before, after;
    char prefix[20];
};

static int scan_handle (char *name, const char *info, int pos, void *client)
{
    int len_prefix, idx;
    struct scan_info *scan_info = (struct scan_info *) client;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp (name, scan_info->prefix, len_prefix))
        return 1;
    if (pos > 0)        idx = scan_info->after - pos + scan_info->before;
    else
        idx = - pos - 1;
    scan_info->list[idx].term = (char *)
	odr_malloc (scan_info->odr, strlen(name + len_prefix)+1);
    strcpy (scan_info->list[idx].term, name + len_prefix);
    assert (*info == sizeof(ISAMS_P));
    memcpy (&scan_info->list[idx].isam_p, info+1, sizeof(ISAMS_P));
    return 0;
}

static void scan_term_untrans (ZebraHandle zh, NMEM stream, int reg_type,
			       char **dst, const char *src)
{
    char term_dst[1024];
    
    term_untrans (zh, reg_type, term_dst, src);
    
    *dst = (char *) nmem_malloc (stream, strlen(term_dst)+1);
    strcpy (*dst, term_dst);
}

static void count_set (RSET r, int *count)
{
    int psysno = 0;
    int kno = 0;
    struct it_key key;
    RSFD rfd;
    int term_index;

    logf (LOG_DEBUG, "count_set");

    *count = 0;
    rfd = rset_open (r, RSETF_READ);
    while (rset_read (r, rfd, &key, &term_index))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            (*count)++;
        }
        kno++;
    }
    rset_close (r, rfd);
    logf (LOG_DEBUG, "%d keys, %d records", kno, *count);
}

void rpn_scan (ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
	       oid_value attributeset,
	       int num_bases, char **basenames,
	       int *position, int *num_entries, ZebraScanEntry **list,
	       int *is_partial)
{
    int i;
    int pos = *position;
    int num = *num_entries;
    int before;
    int after;
    int base_no;
    char termz[IT_MAX_WORD+20];
    AttrType use;
    int use_value;
    struct scan_info *scan_info_array;
    ZebraScanEntry *glist;
    int ords[32], ord_no = 0;
    int ptr[32];

    unsigned reg_id;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    *list = 0;

    if (attributeset == VAL_NONE)
        attributeset = VAL_BIB1;

    yaz_log (LOG_DEBUG, "position = %d, num = %d set=%d",
             pos, num, attributeset);
        
    attr_init (&use, zapt, 1);
    use_value = attr_find (&use, &attributeset);

    if (zebra_maps_attr (zh->reg->zebra_maps, zapt, &reg_id, &search_type,
			 rank_type, &complete_flag, &sort_flag))
    {
	*num_entries = 0;
	zh->errCode = 113;
	return ;
    }
    yaz_log (LOG_DEBUG, "use_value = %d", use_value);

    if (use_value == -1)
        use_value = 1016;
    for (base_no = 0; base_no < num_bases && ord_no < 32; base_no++)
    {
	int r;
        attent attp;
        data1_local_attribute *local_attr;

        if ((r=att_getentbyatt (zh, &attp, attributeset, use_value)))
        {
            logf (LOG_DEBUG, "att_getentbyatt fail. set=%d use=%d",
                  attributeset, use_value);
	    if (r == -1)
		zh->errCode = 114;
	    else
		zh->errCode = 121;
	    *num_entries = 0;
	    return;
        }
        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
            zh->errString = basenames[base_no];
	    zh->errCode = 109; /* Database unavailable */
	    *num_entries = 0;
	    return;
        }
        for (local_attr = attp.local_attributes; local_attr && ord_no < 32;
             local_attr = local_attr->next)
        {
            int ord;

            ord = zebraExplain_lookupSU (zh->reg->zei, attp.attset_ordinal,
					 local_attr->local);
            if (ord > 0)
                ords[ord_no++] = ord;
        }
    }
    if (ord_no == 0)
    {
	*num_entries = 0;
        zh->errCode = 113;
	return;
    }
    /* prepare dictionary scanning */
    before = pos-1;
    after = 1+num-pos;
    scan_info_array = (struct scan_info *)
	odr_malloc (stream, ord_no * sizeof(*scan_info_array));
    for (i = 0; i < ord_no; i++)
    {
        int j, prefix_len = 0;
        int before_tmp = before, after_tmp = after;
        struct scan_info *scan_info = scan_info_array + i;
	struct rpn_char_map_info rcmi;

	rpn_char_map_prepare (zh->reg, reg_id, &rcmi);

        scan_info->before = before;
        scan_info->after = after;
        scan_info->odr = stream;

        scan_info->list = (struct scan_info_entry *)
	    odr_malloc (stream, (before+after) * sizeof(*scan_info->list));
        for (j = 0; j<before+after; j++)
            scan_info->list[j].term = NULL;

	prefix_len += key_SU_encode (ords[i], termz + prefix_len);
        termz[prefix_len++] = reg_id;
        termz[prefix_len] = 0;
        strcpy (scan_info->prefix, termz);

        trans_scan_term (zh, zapt, termz+prefix_len, reg_id);
                    
        dict_scan (zh->reg->dict, termz, &before_tmp, &after_tmp,
		   scan_info, scan_handle);
    }
    glist = (ZebraScanEntry *)
	odr_malloc (stream, (before+after)*sizeof(*glist));

    /* consider terms after main term */
    for (i = 0; i < ord_no; i++)
        ptr[i] = before;
    
    *is_partial = 0;
    for (i = 0; i<after; i++)
    {
        int j, j0 = -1;
        const char *mterm = NULL;
        const char *tst;
        RSET rset;
        
        for (j = 0; j < ord_no; j++)
        {
            if (ptr[j] < before+after &&
                (tst=scan_info_array[j].list[ptr[j]].term) &&
                (!mterm || strcmp (tst, mterm) < 0))
            {
                j0 = j;
                mterm = tst;
            }
        }
        if (j0 == -1)
            break;
        scan_term_untrans (zh, stream->mem, reg_id,
			   &glist[i+before].term, mterm);
        rset = rset_trunc (zh, &scan_info_array[j0].list[ptr[j0]].isam_p, 1,
			   glist[i+before].term, strlen(glist[i+before].term),
			   NULL);

        ptr[j0]++;
        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before+after &&
                (tst=scan_info_array[j].list[ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
                rset_bool_parms bool_parms;
                RSET rset2;

                rset2 =
                   rset_trunc (zh, &scan_info_array[j].list[ptr[j]].isam_p, 1,
			       glist[i+before].term,
			       strlen(glist[i+before].term), NULL);

                bool_parms.key_size = sizeof(struct it_key);
                bool_parms.cmp = key_compare_it;
                bool_parms.rset_l = rset;
                bool_parms.rset_r = rset2;
              
                rset = rset_create (rset_kind_or, &bool_parms);

                ptr[j]++;
            }
        }
        count_set (rset, &glist[i+before].occurrences);
        rset_delete (rset);
    }
    if (i < after)
    {
        *num_entries -= (after-i);
        *is_partial = 1;
    }

    /* consider terms before main term */
    for (i = 0; i<ord_no; i++)
        ptr[i] = 0;

    for (i = 0; i<before; i++)
    {
        int j, j0 = -1;
        const char *mterm = NULL;
        const char *tst;
        RSET rset;
        
        for (j = 0; j <ord_no; j++)
        {
            if (ptr[j] < before &&
                (tst=scan_info_array[j].list[before-1-ptr[j]].term) &&
                (!mterm || strcmp (tst, mterm) > 0))
            {
                j0 = j;
                mterm = tst;
            }
        }
        if (j0 == -1)
            break;

        scan_term_untrans (zh, stream->mem, reg_id,
			   &glist[before-1-i].term, mterm);

        rset = rset_trunc
               (zh, &scan_info_array[j0].list[before-1-ptr[j0]].isam_p, 1,
		glist[before-1-i].term, strlen(glist[before-1-i].term),
		NULL);

        ptr[j0]++;

        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before &&
                (tst=scan_info_array[j].list[before-1-ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
                rset_bool_parms bool_parms;
                RSET rset2;

                rset2 = rset_trunc (zh,
                         &scan_info_array[j].list[before-1-ptr[j]].isam_p, 1,
				    glist[before-1-i].term,
				    strlen(glist[before-1-i].term), NULL);

                bool_parms.key_size = sizeof(struct it_key);
                bool_parms.cmp = key_compare_it;
                bool_parms.rset_l = rset;
                bool_parms.rset_r = rset2;
              
                rset = rset_create (rset_kind_or, &bool_parms);

                ptr[j]++;
            }
        }
        count_set (rset, &glist[before-1-i].occurrences);
        rset_delete (rset);
    }
    i = before-i;
    if (i)
    {
        *is_partial = 1;
        *position -= i;
        *num_entries -= i;
    }
    *list = glist + i;               /* list is set to first 'real' entry */
    
    logf (LOG_DEBUG, "position = %d, num_entries = %d",
          *position, *num_entries);
    if (zh->errCode)
        logf (LOG_DEBUG, "scan error: %d", zh->errCode);
}
              
