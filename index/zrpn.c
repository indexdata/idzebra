/*
 * Copyright (C) 1995-1998, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zrpn.c,v $
 * Revision 1.88  1998-10-18 07:54:52  adam
 * Additional info added for diagnostics 114 (Unsupported use attribute) and
 * 121 (Unsupported attribute set).
 *
 * Revision 1.87  1998/09/28 11:19:12  adam
 * Fix for Compiled ASN.1.
 *
 * Revision 1.86  1998/09/22 10:48:20  adam
 * Minor changes in search API.
 *
 * Revision 1.85  1998/09/22 10:03:43  adam
 * Changed result sets to be persistent in the sense that they can
 * be re-searched if needed.
 * Fixed memory leak in rsm_or.
 *
 * Revision 1.84  1998/09/18 12:41:00  adam
 * Fixed bug with numerical relations.
 *
 * Revision 1.83  1998/09/02 13:53:19  adam
 * Extra parameter decode added to search routines to implement
 * persistent queries.
 *
 * Revision 1.82  1998/06/26 11:16:40  quinn
 * Added support (un-optimised) for left and left/right truncation
 *
 * Revision 1.81  1998/06/24 12:16:14  adam
 * Support for relations on text operands. Open range support in
 * DFA module (i.e. [-j], [g-]).
 *
 * Revision 1.80  1998/06/23 15:33:34  adam
 * Added feature to specify sort criteria in query (type 7 specifies
 * sort flags).
 *
 * Revision 1.79  1998/06/22 11:35:09  adam
 * Minor changes.
 *
 * Revision 1.78  1998/06/08 14:43:17  adam
 * Added suport for EXPLAIN Proxy servers - added settings databasePath
 * and explainDatabase to facilitate this. Increased maximum number
 * of databases and attributes in one register.
 *
 * Revision 1.77  1998/05/20 10:12:22  adam
 * Implemented automatic EXPLAIN database maintenance.
 * Modified Zebra to work with ASN.1 compiled version of YAZ.
 *
 * Revision 1.76  1998/04/02 14:35:29  adam
 * First version of Zebra that works with compiled ASN.1.
 *
 * Revision 1.75  1998/03/05 08:45:13  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.74  1998/02/10 12:03:06  adam
 * Implemented Sort.
 *
 * Revision 1.73  1998/01/29 13:40:11  adam
 * Better logging for scan service.
 *
 * Revision 1.72  1998/01/07 13:53:41  adam
 * Queries using simple ranked operands returns right number of hits.
 *
 * Revision 1.71  1997/12/18 10:54:24  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.70  1997/10/31 12:34:43  adam
 * Changed a few log statements.
 *
 * Revision 1.69  1997/10/29 12:05:02  adam
 * Server produces diagnostic "Unsupported Attribute Set" when appropriate.
 *
 * Revision 1.68  1997/10/27 14:33:06  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.67  1997/09/29 09:06:10  adam
 * Removed one static var in order to make this module thread safe.
 *
 * Revision 1.66  1997/09/25 14:58:03  adam
 * Windows NT port.
 *
 * Revision 1.65  1997/09/22 12:39:06  adam
 * Added get_pos method for the ranked result sets.
 *
 * Revision 1.64  1997/09/18 08:59:20  adam
 * Extra generic handle for the character mapping routines.
 *
 * Revision 1.63  1997/09/17 12:19:18  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.62  1997/09/05 15:30:09  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.61  1997/02/10 10:21:14  adam
 * Bug fix: in search terms character (^) wasn't observed.
 *
 * Revision 1.60  1997/01/31 11:10:34  adam
 * Bug fix: Leading and trailing white space weren't removed in scan tokens.
 *
 * Revision 1.59  1997/01/17 11:31:46  adam
 * Bug fix: complete phrase search didn't work.
 *
 * Revision 1.58  1996/12/23 15:30:45  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.57  1996/11/11 13:38:02  adam
 * Added proximity support in search.
 *
 * Revision 1.56  1996/11/08 11:10:32  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.55  1996/11/04 14:07:44  adam
 * Moved truncation code to trunc.c.
 *
 * Revision 1.54  1996/10/29 14:09:52  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.53  1996/06/26 09:21:43  adam
 * Bug fix: local attribute set wasn't obeyed in scan.
 *
 * Revision 1.52  1996/06/17  14:26:20  adam
 * Function gen_regular_rel changed to handle negative numbers.
 *
 * Revision 1.51  1996/06/11 10:54:15  quinn
 * Relevance work
 *
 * Revision 1.50  1996/06/07  08:51:53  adam
 * Bug fix: Character mapping was broken (introducued by last revision).
 *
 * Revision 1.49  1996/06/04  10:18:11  adam
 * Search/scan uses character mapping module.
 *
 * Revision 1.48  1996/05/28  15:15:01  adam
 * Bug fix: Didn't handle unknown database correctly.
 *
 * Revision 1.47  1996/05/15  18:36:28  adam
 * Function trans_term transforms unsearchable characters to blanks.
 *
 * Revision 1.46  1996/05/15  11:57:56  adam
 * Fixed bug introduced by set/field mapping in search operations.
 *
 * Revision 1.45  1996/05/14  11:34:00  adam
 * Scan support in multiple registers/databases.
 *
 * Revision 1.44  1996/05/14  06:16:44  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.43  1996/05/09 09:54:43  adam
 * Server supports maps from one logical attributes to a list of physical
 * attributes.
 * The extraction process doesn't make space consuming 'any' keys.
 *
 * Revision 1.42  1996/05/09  07:28:56  quinn
 * Work towards phrases and multiple registers
 *
 * Revision 1.41  1996/03/20  09:36:43  adam
 * Function dict_lookup_grep got extra parameter, init_pos, which marks
 * from which position in pattern approximate pattern matching should occur.
 * Approximate pattern matching is used in relevance=re-2.
 *
 * Revision 1.40  1996/02/02  13:44:44  adam
 * The public dictionary functions simply use char instead of Dict_char
 * to represent search strings. Dict_char is used internally only.
 *
 * Revision 1.39  1996/01/03  16:22:13  quinn
 * operator->roperator
 *
 * Revision 1.38  1995/12/11  09:12:55  adam
 * The rec_get function returns NULL if record doesn't exist - will
 * happen in the server if the result set records have been deleted since
 * the creation of the set (i.e. the search).
 * The server saves a result temporarily if it is 'volatile', i.e. the
 * set is register dependent.
 *
 * Revision 1.37  1995/12/06  15:05:28  adam
 * More verbose in count_set.
 *
 * Revision 1.36  1995/12/06  12:41:27  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.35  1995/11/27  09:29:00  adam
 * Bug fixes regarding conversion to regular expressions.
 *
 * Revision 1.34  1995/11/16  17:00:56  adam
 * Better logging of rpn query.
 *
 * Revision 1.33  1995/11/01  13:58:28  quinn
 * Moving data1 to yaz/retrieval
 *
 * Revision 1.32  1995/10/27  14:00:11  adam
 * Implemented detection of database availability.
 *
 * Revision 1.31  1995/10/17  18:02:10  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.30  1995/10/16  09:32:38  adam
 * More work on relational op.
 *
 * Revision 1.29  1995/10/13  16:01:49  adam
 * Work on relations.
 *
 * Revision 1.28  1995/10/13  12:26:43  adam
 * Optimization of truncation.
 *
 * Revision 1.27  1995/10/12  17:07:22  adam
 * Truncation works.
 *
 * Revision 1.26  1995/10/12  12:40:54  adam
 * Bug fixes in rpn_prox.
 *
 * Revision 1.25  1995/10/10  13:59:24  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.24  1995/10/09  16:18:37  adam
 * Function dict_lookup_grep got extra client data parameter.
 *
 * Revision 1.23  1995/10/06  16:33:37  adam
 * Use attribute mappings.
 *
 * Revision 1.22  1995/10/06  15:07:39  adam
 * Structure 'local-number' handled.
 *
 * Revision 1.21  1995/10/06  13:52:06  adam
 * Bug fixes. Handler may abort further scanning.
 *
 * Revision 1.20  1995/10/06  11:06:33  adam
 * Scan entries include 'occurrences' now.
 *
 * Revision 1.19  1995/10/06  10:43:56  adam
 * Scan added. 'occurrences' in scan entries not set yet.
 *
 * Revision 1.18  1995/10/04  16:57:20  adam
 * Key input and merge sort in one pass.
 *
 * Revision 1.17  1995/10/04  12:55:17  adam
 * Bug fix in ranked search. Use=Any keys inserted.
 *
 * Revision 1.16  1995/10/02  16:24:40  adam
 * Use attribute actually used in search requests.
 *
 * Revision 1.15  1995/10/02  15:18:52  adam
 * New member in recRetrieveCtrl: diagnostic.
 *
 * Revision 1.14  1995/09/28  12:10:32  adam
 * Bug fixes. Field prefix used in queries.
 *
 * Revision 1.13  1995/09/18  14:17:50  adam
 * Minor changes.
 *
 * Revision 1.12  1995/09/15  14:45:21  adam
 * Retrieve control.
 * Work on truncation.
 *
 * Revision 1.11  1995/09/14  11:53:27  adam
 * First work on regular expressions/truncations.
 *
 * Revision 1.10  1995/09/11  15:23:26  adam
 * More work on relevance search.
 *
 * Revision 1.9  1995/09/11  13:09:35  adam
 * More work on relevance feedback.
 *
 * Revision 1.8  1995/09/08  14:52:27  adam
 * Minor changes. Dictionary is lower case now.
 *
 * Revision 1.7  1995/09/07  13:58:36  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 * Result-set references.
 *
 * Revision 1.6  1995/09/06  16:11:18  adam
 * Option: only one word key per file.
 *
 * Revision 1.5  1995/09/06  10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.4  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 * Revision 1.3  1995/09/04  15:20:22  adam
 * Minor changes.
 *
 * Revision 1.2  1995/09/04  12:33:43  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:40  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */
#include <stdio.h>
#include <assert.h>
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif
#include <ctype.h>

#include "zserver.h"

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
    struct rpn_char_map_info *p = vp;
    return zebra_maps_input (p->zm, p->reg_type, from, len);
}

static void rpn_char_map_prepare (ZebraHandle zh, int reg_type,
				  struct rpn_char_map_info *map_info)
{
    map_info->zm = zh->zebra_maps;
    map_info->reg_type = reg_type;
    dict_grep_cmap (zh->dict, map_info, rpn_char_map_handler);
}

typedef struct {
    int type;
    int major;
    int minor;
    Z_AttributesPlusTerm *zapt;
} AttrType;

static int attr_find (AttrType *src, oid_value *attributeSetP)
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

#define TERM_COUNT        
       
struct grep_info {        
#ifdef TERM_COUNT        
    int *term_no;        
#endif        
    ISAM_P *isam_p_buf;
    int isam_p_size;        
    int isam_p_indx;
    ZebraHandle zh;
    int reg_type;
};        

static void term_untrans  (ZebraHandle zh, int reg_type,
			   char *dst, const char *src)
{
    while (*src)
    {
        const char *cp = zebra_maps_output (zh->zebra_maps, reg_type, &src);
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
        ISAM_P *new_isam_p_buf;
#ifdef TERM_COUNT        
        int *new_term_no;        
#endif
        p->isam_p_size = 2*p->isam_p_size + 100;
        new_isam_p_buf = xmalloc (sizeof(*new_isam_p_buf) *
                                  p->isam_p_size);
        if (p->isam_p_buf)
        {
            memcpy (new_isam_p_buf, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->isam_p_buf));
            xfree (p->isam_p_buf);
        }
        p->isam_p_buf = new_isam_p_buf;

#ifdef TERM_COUNT
        new_term_no = xmalloc (sizeof(*new_term_no) *
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

#if 0
    term_untrans  (p->zh, p->reg_type, term_tmp, name+2);
    logf (LOG_DEBUG, "grep: %s", term_tmp);
#endif
    (p->isam_p_indx)++;
}

static int grep_handle (char *name, const char *info, void *p)
{
    add_isam_p (name, info, p);
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

static int term_100 (ZebraMaps zebra_maps, int reg_type,
		     const char **src, char *dst, int space_split,
		     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    if (!term_pre (zebra_maps, reg_type, src, NULL, NULL))
        return 0;
    s0 = *src;
    while (*s0)
    {
        s1 = s0;
        map = zebra_maps_input (zebra_maps, reg_type, &s0, strlen(s0));
        if (space_split && **map == *CHR_SPACE)
            break;
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

static int term_102 (ZebraMaps zebra_maps, int reg_type, const char **src,
		     char *dst, int space_split, char *dst_term)
{
    return term_103 (zebra_maps, reg_type, src, dst, NULL, space_split,
		     dst_term);
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
        for (i=1; i<pos; i++)
            strcat (dst, "[0-9]?");
    }
    else
    {
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
        if (!term_100 (zh->zebra_maps, reg_type, term_sub, term_component,
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
        if (!term_100 (zh->zebra_maps, reg_type, term_sub, term_component,
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
        if (!term_100 (zh->zebra_maps, reg_type, term_sub, term_component,
		       space_split, term_dst))
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
        if (!term_100 (zh->zebra_maps, reg_type, term_sub, term_component,
		       space_split, term_dst))
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
        if (!term_100 (zh->zebra_maps, reg_type, term_sub, term_component,
		       space_split, term_dst))
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

    rpn_char_map_prepare (zh, reg_type, &rcmi);
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
        if (zebraExplain_curDatabase (zh->zei, basenames[base_no]))
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

            ord = zebraExplain_lookupSU (zh->zei, attp.attset_ordinal,
                                          local_attr->local);
            if (ord < 0)
                continue;
            if (prefix_len)
                term_dict[prefix_len++] = '|';
            else
                term_dict[prefix_len++] = '(';

	    ord_len = key_SU_code (ord, ord_buf);
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
	    r = dict_lookup_grep (zh->dict, term_dict, 0, grep_info, &max_pos,
				  0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep fail, rel=gt: %d", r);
	    break;
	case 1:          /* right truncation */
	    term_dict[j++] = '(';
	    if (!term_100 (zh->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ".*)");
	    dict_lookup_grep (zh->dict, term_dict, 0, grep_info,
			      &max_pos, 0, grep_handle);
	    break;
	case 2:          /* keft truncation */
	    term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
	    if (!term_100 (zh->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    dict_lookup_grep (zh->dict, term_dict, 0, grep_info,
			      &max_pos, 0, grep_handle);
	    break;
	case 3:          /* left&right truncation */
	    term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
	    if (!term_100 (zh->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ".*)");
	    dict_lookup_grep (zh->dict, term_dict, 0, grep_info,
			      &max_pos, 0, grep_handle);
	    break;
	    zh->errCode = 120;
	    return -1;
	case 101:        /* process # in term */
	    term_dict[j++] = '(';
	    if (!term_101 (zh->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    r = dict_lookup_grep (zh->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=#: %d", r);
	    break;
	case 102:        /* Regexp-1 */
	    term_dict[j++] = '(';
	    if (!term_102 (zh->zebra_maps, reg_type,
			   &termp, term_dict + j, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    logf (LOG_DEBUG, "Regexp-1 tolerance=%d", r);
	    r = dict_lookup_grep (zh->dict, term_dict, 0, grep_info,
				  &max_pos, 0, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=regular: %d",
		      r);
	    break;
	case 103:       /* Regexp-2 */
	    r = 1;
	    term_dict[j++] = '(';
	    if (!term_103 (zh->zebra_maps, reg_type,
			   &termp, term_dict + j, &r, space_split, term_dst))
		return 0;
	    strcat (term_dict, ")");
	    logf (LOG_DEBUG, "Regexp-2 tolerance=%d", r);
	    r = dict_lookup_grep (zh->dict, term_dict, r, grep_info,
				  &max_pos, 2, grep_handle);
	    if (r)
		logf (LOG_WARN, "dict_lookup_grep err, trunc=eregular: %d",
		      r);
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
        map = zebra_maps_input (zh->zebra_maps, reg_type, &cp, len);
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

static RSET rpn_proximity (ZebraHandle zh, RSET rset1, RSET rset2,
			   int ordered,
                           int exclusion, int relation, int distance)
{
    int i;
    RSFD rsfd1, rsfd2;
    int  more1, more2;
    struct it_key buf1, buf2;
    RSFD rsfd_result;
    RSET result;
    rset_temp_parms parms;
    int term_index;
    
    rsfd1 = rset_open (rset1, RSETF_READ);
    more1 = rset_read (rset1, rsfd1, &buf1, &term_index);
    
    rsfd2 = rset_open (rset2, RSETF_READ);
    more2 = rset_read (rset2, rsfd2, &buf2, &term_index);

    parms.key_size = sizeof (struct it_key);
    parms.temp_path = res_get (zh->res, "setTmpDir");
    result = rset_create (rset_kind_temp, &parms);
    rsfd_result = rset_open (result, RSETF_WRITE);
   
    logf (LOG_DEBUG, "rpn_proximity  excl=%d ord=%d rel=%d dis=%d",
          exclusion, ordered, relation, distance);
    while (more1 && more2)
    {
        int cmp = key_compare_it (&buf1, &buf2);
        if (cmp < -1)
            more1 = rset_read (rset1, rsfd1, &buf1, &term_index);
        else if (cmp > 1)
            more2 = rset_read (rset2, rsfd2, &buf2, &term_index);
        else
        {
            int sysno = buf1.sysno;
            int seqno[500];
            int n = 0;

            seqno[n++] = buf1.seqno;
            while ((more1 = rset_read (rset1, rsfd1, &buf1, &term_index)) &&
                   sysno == buf1.sysno)
                if (n < 500)
                    seqno[n++] = buf1.seqno;
            do
            {
                for (i = 0; i<n; i++)
                {
                    int diff = buf2.seqno - seqno[i];
                    int excl = exclusion;
                    if (!ordered && diff < 0)
                        diff = -diff;
                    switch (relation)
                    {
                    case 1:      /* < */
                        if (diff < distance)
                            excl = !excl;
                        break;
                    case 2:      /* <= */
                        if (diff <= distance)
                            excl = !excl;
                        break;
                    case 3:      /* == */
                        if (diff == distance)
                            excl = !excl;
                        break;
                    case 4:      /* >= */
                        if (diff >= distance)
                            excl = !excl;
                        break;
                    case 5:      /* > */
                        if (diff > distance)
                            excl = !excl;
                        break;
                    case 6:      /* != */
                        if (diff != distance)
                            excl = !excl;
                        break;
                    }
                    if (excl)
                        rset_write (result, rsfd_result, &buf2);
                }
            } while ((more2 = rset_read (rset2, rsfd2, &buf2, &term_index)) &&
                      sysno == buf2.sysno);
        }
    }
    rset_close (result, rsfd_result);
    rset_close (rset1, rsfd1);
    rset_close (rset2, rsfd2);
    return result;
}

static RSET rpn_prox (ZebraHandle zh, RSET *rset, int rset_no)
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
    
    rsfd = xmalloc (sizeof(*rsfd)*rset_no);
    more = xmalloc (sizeof(*more)*rset_no);
    buf = xmalloc (sizeof(*buf)*rset_no);

    for (i = 0; i<rset_no; i++)
    {
	int j;
	buf[i] = xmalloc (sizeof(**buf));
	rsfd[i] = rset_open (rset[i], RSETF_READ);
        if (!(more[i] = rset_read (rset[i], rsfd[i], buf[i], &term_index)))
	    break;
	for (j = 0; j<rset[i]->no_rset_terms; j++)
	{
	    const char *nflags = rset[i]->rset_terms[j]->flags;
	    char *term = rset[i]->rset_terms[j]->name;
	    int lterm = strlen(term);
	    if (length_prox_term)
		prox_term[length_prox_term++] = ' ';
	    strcpy (prox_term + length_prox_term, term);
	    length_prox_term += lterm;
	    if (min_nn > rset[i]->rset_terms[j]->nn)
		min_nn = rset[i]->rset_terms[j]->nn;
	    flags = nflags;
	}
    }
    if (i != rset_no)
    {
	rset_null_parms parms;

	while (i >= 0)
	{
	    rset_close (rset[i], rsfd[i]);
	    xfree (buf[i]);
	    --i;
	}
	parms.rset_term = rset_term_create (prox_term, -1, flags);
	parms.rset_term->nn = 0;
	result = rset_create (rset_kind_null, &parms);
    }
    else
    {
	rset_temp_parms parms;
	RSFD rsfd_result;

	parms.rset_term = rset_term_create (prox_term, -1, flags);
	parms.rset_term->nn = min_nn;
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
	
	for (i = 0; i<rset_no; i++)
	{
	    rset_close (rset[i], rsfd[i]);
	    xfree (buf[i]);
	}
	rset_close (result, rsfd_result);
    }
    xfree (buf);
    xfree (more);
    xfree (rsfd);
    return result;
}

static RSET rpn_search_APT_phrase (ZebraHandle zh,
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

#ifdef TERM_COUNT
    grep_info.term_no = 0;
#endif
    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;
    grep_info.zh = zh;
    grep_info.reg_type = reg_type;

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
        if (++rset_no >= sizeof(rset)/sizeof(*rset))
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
    else if (rset_no == 1)
        return (rset[0]);
    result = rpn_prox (zh, rset, rset_no);
    for (i = 0; i<rset_no; i++)
        rset_delete (rset[i]);
    return result;
}

static RSET rpn_search_APT_or_list (ZebraHandle zh,
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

#ifdef TERM_COUNT
    grep_info.term_no = 0;
#endif
    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;
    grep_info.zh = zh;
    grep_info.reg_type = reg_type;

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
        if (++rset_no >= sizeof(rset)/sizeof(*rset))
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
        result = rset_create (rset_kind_or, &bool_parms);
    }
    return result;
}

static RSET rpn_search_APT_and_list (ZebraHandle zh,
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

#ifdef TERM_COUNT
    grep_info.term_no = 0;
#endif
    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;
    grep_info.zh = zh;
    grep_info.reg_type = reg_type;

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
        if (++rset_no >= sizeof(rset)/sizeof(*rset))
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

    if (!term_100 (zh->zebra_maps, reg_type, term_sub, term_tmp, 1,
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
    r = dict_lookup_grep (zh->dict, term_dict, 0, grep_info, max_pos,
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

    rpn_char_map_prepare (zh, reg_type, &rcmi);
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
        if (zebraExplain_curDatabase (zh->zei, basenames[base_no]))
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

            ord = zebraExplain_lookupSU (zh->zei, attp.attset_ordinal,
                                          local_attr->local);
            if (ord < 0)
                continue;
            if (prefix_len)
                term_dict[prefix_len++] = '|';
            else
                term_dict[prefix_len++] = '(';

	    ord_len = key_SU_code (ord, ord_buf);
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

#ifdef TERM_COUNT
    grep_info.term_no = 0;
#endif
    grep_info.isam_p_size = 0;
    grep_info.isam_p_buf = NULL;
    grep_info.zh = zh;
    grep_info.reg_type = reg_type;

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
        if (++rset_no >= sizeof(rset)/sizeof(*rset))
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
    
    attr_init (&sort_relation_type, zapt, 7);
    sort_relation_value = attr_find (&sort_relation_type, &attributeSet);

    attr_init (&use_type, zapt, 1);
    use_value = attr_find (&use_type, &attributeSet);

    if (!sort_sequence->specs)
    {
	sort_sequence->num_specs = 10;
	sort_sequence->specs = nmem_malloc (stream, sort_sequence->num_specs *
					    sizeof(*sort_sequence->specs));
	for (i = 0; i<sort_sequence->num_specs; i++)
	    sort_sequence->specs[i] = 0;
    }
    if (zapt->term->which != Z_Term_general)
	i = 0;
    else
	i = atoi_n (zapt->term->u.general->buf, zapt->term->u.general->len);
    if (i >= sort_sequence->num_specs)
	i = 0;

    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_ATTSET;
    oe.value = attributeSet;
    if (!oid_ent_to_oid (&oe, oid))
	return 0;

    sks = nmem_malloc (stream, sizeof(*sks));
    sks->sortElement = nmem_malloc (stream, sizeof(*sks->sortElement));
    sks->sortElement->which = Z_SortElement_generic;
    sk = sks->sortElement->u.generic = nmem_malloc (stream, sizeof(*sk));
    sk->which = Z_SortKey_sortAttributes;
    sk->u.sortAttributes = nmem_malloc (stream, sizeof(*sk->u.sortAttributes));

    sk->u.sortAttributes->id = oid;
    sk->u.sortAttributes->list =
	nmem_malloc (stream, sizeof(*sk->u.sortAttributes->list));
    sk->u.sortAttributes->list->num_attributes = 1;
    sk->u.sortAttributes->list->attributes =
	nmem_malloc (stream, sizeof(*sk->u.sortAttributes->list->attributes));
    ae = *sk->u.sortAttributes->list->attributes =
	nmem_malloc (stream, sizeof(**sk->u.sortAttributes->list->attributes));
    ae->attributeSet = 0;
    ae->attributeType =	nmem_malloc (stream, sizeof(*ae->attributeType));
    *ae->attributeType = 1;
    ae->which = Z_AttributeValue_numeric;
    ae->value.numeric = nmem_malloc (stream, sizeof(*ae->value.numeric));
    *ae->value.numeric = use_value;

    sks->sortRelation = nmem_malloc (stream, sizeof(*sks->sortRelation));
    if (sort_relation_value == 1)
	*sks->sortRelation = Z_SortRelation_ascending;
    else if (sort_relation_value == 2)
	*sks->sortRelation = Z_SortRelation_descending;
    else 
	*sks->sortRelation = Z_SortRelation_ascending;

    sks->caseSensitivity = nmem_malloc (stream, sizeof(*sks->caseSensitivity));
    *sks->caseSensitivity = 0;

#ifdef ASN_COMPILED
    sks->which = Z_SortKeySpec_null;
    sks->u.null = odr_nullval ();
#else
    sks->missingValueAction = 0;
#endif

    sort_sequence->specs[i] = sks;

    parms.rset_term = rset_term_create ("", -1, rank_type);
    return rset_create (rset_kind_null, &parms);
}


static RSET rpn_search_APT (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                            oid_value attributeSet, NMEM stream,
			    Z_SortKeySpecList *sort_sequence,
                            int num_bases, char **basenames)
{
    unsigned reg_id;
    char *search_type = NULL;
    char *rank_type = NULL;
    int complete_flag;
    int sort_flag;
    char termz[IT_MAX_WORD+1];

    zebra_maps_attr (zh->zebra_maps, zapt, &reg_id, &search_type,
		     &rank_type, &complete_flag, &sort_flag);
    
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
                char *val = nmem_malloc (stream, 16);
                zh->errCode = 132;
                zh->errString = val;
                sprintf (val, "%d", *zop->u.prox->u.known);
                return NULL;
            }
#else
            if (*zop->u.prox->proximityUnitCode != Z_ProxUnit_word)
            {
                char *val = nmem_malloc (stream, 16);
                zh->errCode = 132;
                zh->errString = val;
                sprintf (val, "%d", *zop->u.prox->proximityUnitCode);
                return NULL;
            }
#endif
            r = rpn_proximity (zh, bool_parms.rset_l, bool_parms.rset_r,
                               *zop->u.prox->ordered,
                               (!zop->u.prox->exclusion ? 0 :
                                         *zop->u.prox->exclusion),
                               *zop->u.prox->relationType,
                               *zop->u.prox->distance);
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
		r = rset_create (rset_kind_null, NULL);
        }
        else
        {
            zh->errCode = 3;
            return NULL;
        }
    }
    else
    {
        zh->errCode = 3;
        return NULL;
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

    sort_sequence = nmem_malloc (nmem, sizeof(*sort_sequence));
    sort_sequence->num_specs = 10;
    sort_sequence->specs = nmem_malloc (nmem, sort_sequence->num_specs *
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
    ISAM_P isam_p;
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
    struct scan_info *scan_info = client;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp (name, scan_info->prefix, len_prefix))
        return 1;
    if (pos > 0)
        idx = scan_info->after - pos + scan_info->before;
    else
        idx = - pos - 1;
    scan_info->list[idx].term = odr_malloc (scan_info->odr,
                                            strlen(name + len_prefix)+1);
    strcpy (scan_info->list[idx].term, name + len_prefix);
    assert (*info == sizeof(ISAM_P));
    memcpy (&scan_info->list[idx].isam_p, info+1, sizeof(ISAM_P));
    return 0;
}

static void scan_term_untrans (ZebraHandle zh, NMEM stream, int reg_type,
			       char **dst, const char *src)
{
    char term_dst[1024];
    
    term_untrans (zh, reg_type, term_dst, src);
    
    *dst = nmem_malloc (stream, strlen(term_dst)+1);
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
    char *rank_type = NULL;
    int complete_flag;
    int sort_flag;

    if (attributeset == VAL_NONE)
        attributeset = VAL_BIB1;

    zlog_scan (zapt, attributeset);
    logf (LOG_DEBUG, "position = %d, num = %d", pos, num);
        
    attr_init (&use, zapt, 1);
    use_value = attr_find (&use, &attributeset);

    if (zebra_maps_attr (zh->zebra_maps, zapt, &reg_id, &search_type,
			 &rank_type, &complete_flag, &sort_flag))
    {
	zh->errCode = 113;
	return ;
    }

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
        }
        if (zebraExplain_curDatabase (zh->zei, basenames[base_no]))
        {
            zh->errString = basenames[base_no];
	    zh->errCode = 109; /* Database unavailable */
	    return;
        }
        for (local_attr = attp.local_attributes; local_attr && ord_no < 32;
             local_attr = local_attr->next)
        {
            int ord;

            ord = zebraExplain_lookupSU (zh->zei, attp.attset_ordinal,
					 local_attr->local);
            if (ord > 0)
                ords[ord_no++] = ord;
        }
    }
    if (ord_no == 0)
    {
        zh->errCode = 113;
	return;
    }
    /* prepare dictionary scanning */
    before = pos-1;
    after = 1+num-pos;
    scan_info_array = odr_malloc (stream, ord_no * sizeof(*scan_info_array));
    for (i = 0; i < ord_no; i++)
    {
        int j, prefix_len = 0;
        int before_tmp = before, after_tmp = after;
        struct scan_info *scan_info = scan_info_array + i;
	struct rpn_char_map_info rcmi;

	rpn_char_map_prepare (zh, reg_id, &rcmi);

        scan_info->before = before;
        scan_info->after = after;
        scan_info->odr = stream;

        scan_info->list = odr_malloc (stream, (before+after)*
                                      sizeof(*scan_info->list));
        for (j = 0; j<before+after; j++)
            scan_info->list[j].term = NULL;

	prefix_len += key_SU_code (ords[i], termz + prefix_len);
        termz[prefix_len++] = reg_id;
        termz[prefix_len] = 0;
        strcpy (scan_info->prefix, termz);

        trans_scan_term (zh, zapt, termz+prefix_len, reg_id);
                    
        dict_scan (zh->dict, termz, &before_tmp, &after_tmp, scan_info,
                   scan_handle);
    }
    glist = odr_malloc (stream, (before+after)*sizeof(*glist));

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
              
