/* $Id: zrpn.c,v 1.164 2004-12-10 12:37:07 heikki Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

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


#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <ctype.h>

#include "index.h"
#include <zebra_xpath.h>

#include <charmap.h>
#include <rset.h>


static const struct key_control it_ctrl =
{ 
    sizeof(struct it_key),
    2, /* we have sysnos and seqnos in this key, nothing more */
    key_compare_it, 
    key_logdump_txt,   /* FIXME  - clean up these functions */
    key_get_seq,
};


const struct key_control *key_it_ctrl = &it_ctrl;

struct rpn_char_map_info
{
    ZebraMaps zm;
    int reg_type;
};

typedef struct
{
    int type;
    int major;
    int minor;
    Z_AttributesPlusTerm *zapt;
} AttrType;


static int log_level_set=0;
static int log_level_rpn=0;

static const char **rpn_char_map_handler(void *vp, const char **from, int len)
{
    struct rpn_char_map_info *p = (struct rpn_char_map_info *) vp;
    const char **out = zebra_maps_input(p->zm, p->reg_type, from, len, 0);
#if 0
    if (out && *out)
    {
        const char *outp = *out;
        yaz_log(YLOG_LOG, "---");
        while (*outp)
        {
            yaz_log(YLOG_LOG, "%02X", *outp);
            outp++;
        }
    }
#endif
    return out;
}

static void rpn_char_map_prepare(struct zebra_register *reg, int reg_type,
                                  struct rpn_char_map_info *map_info)
{
    map_info->zm = reg->zebra_maps;
    map_info->reg_type = reg_type;
    dict_grep_cmap(reg->dict, map_info, rpn_char_map_handler);
}

static int attr_find_ex(AttrType *src, oid_value *attributeSetP,
                         const char **string_value)
{
    int num_attributes;

    num_attributes = src->zapt->attributes->num_attributes;
    while (src->major < num_attributes)
    {
        Z_AttributeElement *element;

        element = src->zapt->attributes->attributes[src->major];
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
                if (src->minor >= element->value.complex->num_list)
                    break;
                if (element->attributeSet && attributeSetP)
                {
                    oident *attrset;
                    
                    attrset = oid_getentbyoid(element->attributeSet);
                    *attributeSetP = attrset->value;
                }
                if (element->value.complex->list[src->minor]->which ==  
                    Z_StringOrNumeric_numeric)
                {
                    ++(src->minor);
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
                assert(0);
            }
        }
        ++(src->major);
    }
    return -1;
}

static int attr_find(AttrType *src, oid_value *attributeSetP)
{
    return attr_find_ex(src, attributeSetP, 0);
}

static void attr_init(AttrType *src, Z_AttributesPlusTerm *zapt,
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
    ISAMC_P *isam_p_buf;
    int isam_p_size;        
    int isam_p_indx;
    ZebraHandle zh;
    int reg_type;
    ZebraSet termset;
};        

static void term_untrans(ZebraHandle zh, int reg_type,
                           char *dst, const char *src)
{
    int len = 0;
    while (*src)
    {
        const char *cp = zebra_maps_output(zh->reg->zebra_maps,
                                            reg_type, &src);
        if (!cp && len < IT_MAX_WORD-1)
            dst[len++] = *src++;
        else
            while (*cp && len < IT_MAX_WORD-1)
                dst[len++] = *cp++;
    }
    dst[len] = '\0';
}

static void add_isam_p(const char *name, const char *info,
                        struct grep_info *p)
{
    if (!log_level_set)
    {
        log_level_rpn = yaz_log_module_level("rpn");
        log_level_set=1;
    }
    if (p->isam_p_indx == p->isam_p_size)
    {
        ISAMC_P *new_isam_p_buf;
#ifdef TERM_COUNT        
        int *new_term_no;        
#endif
        p->isam_p_size = 2*p->isam_p_size + 100;
        new_isam_p_buf = (ISAMC_P *) xmalloc(sizeof(*new_isam_p_buf) *
                                             p->isam_p_size);
        if (p->isam_p_buf)
        {
            memcpy(new_isam_p_buf, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->isam_p_buf));
            xfree(p->isam_p_buf);
        }
        p->isam_p_buf = new_isam_p_buf;

#ifdef TERM_COUNT
        new_term_no = (int *) xmalloc(sizeof(*new_term_no) *
                                       p->isam_p_size);
        if (p->term_no)
        {
            memcpy(new_term_no, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->term_no));
            xfree(p->term_no);
        }
        p->term_no = new_term_no;
#endif
    }
    assert(*info == sizeof(*p->isam_p_buf));
    memcpy(p->isam_p_buf + p->isam_p_indx, info+1, sizeof(*p->isam_p_buf));

#if 1
    if (p->termset)
    {
        const char *db;
        int set, use;
        char term_tmp[IT_MAX_WORD];
        int su_code = 0;
        int len = key_SU_decode (&su_code, name);
        
        term_untrans  (p->zh, p->reg_type, term_tmp, name+len+1);
        yaz_log(log_level_rpn, "grep: %d %c %s", su_code, name[len], term_tmp);
        zebraExplain_lookup_ord (p->zh->reg->zei,
                                 su_code, &db, &set, &use);
        yaz_log(log_level_rpn, "grep:  set=%d use=%d db=%s", set, use, db);
        
        resultSetAddTerm(p->zh, p->termset, name[len], db,
			 set, use, term_tmp);
    }
#endif
    (p->isam_p_indx)++;
}

static int grep_handle(char *name, const char *info, void *p)
{
    add_isam_p(name, info, (struct grep_info *) p);
    return 0;
}

static int term_pre(ZebraMaps zebra_maps, int reg_type, const char **src,
                     const char *ct1, const char *ct2, int first)
{
    const char *s1, *s0 = *src;
    const char **map;

    /* skip white space */
    while (*s0)
    {
        if (ct1 && strchr(ct1, *s0))
            break;
        if (ct2 && strchr(ct2, *s0))
            break;
        s1 = s0;
        map = zebra_maps_input(zebra_maps, reg_type, &s1, strlen(s1), first);
        if (**map != *CHR_SPACE)
            break;
        s0 = s1;
    }
    *src = s0;
    return *s0;
}

#define REGEX_CHARS " []()|.*+?!"

/* term_100: handle term, where trunc=none(no operators at all) */
static int term_100(ZebraMaps zebra_maps, int reg_type,
                     const char **src, char *dst, int space_split,
                     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    const char *space_start = 0;
    const char *space_end = 0;

    if (!term_pre(zebra_maps, reg_type, src, NULL, NULL, !space_split))
        return 0;
    s0 = *src;
    while (*s0)
    {
        s1 = s0;
        map = zebra_maps_input(zebra_maps, reg_type, &s0, strlen(s0), 0);
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
                    if (strchr(REGEX_CHARS, *space_start))
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
            if (strchr(REGEX_CHARS, *s1))
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
static int term_101(ZebraMaps zebra_maps, int reg_type,
                     const char **src, char *dst, int space_split,
                     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    if (!term_pre(zebra_maps, reg_type, src, "#", "#", !space_split))
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
            map = zebra_maps_input(zebra_maps, reg_type, &s0, strlen(s0), 0);
            if (space_split && **map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (strchr(REGEX_CHARS, *s1))
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
static int term_103(ZebraMaps zebra_maps, int reg_type, const char **src,
                     char *dst, int *errors, int space_split,
                     char *dst_term)
{
    int i = 0;
    int j = 0;
    const char *s0, *s1;
    const char **map;

    if (!term_pre(zebra_maps, reg_type, src, "^\\()[].*+?|", "(", !space_split))
        return 0;
    s0 = *src;
    if (errors && *s0 == '+' && s0[1] && s0[2] == '+' && s0[3] &&
        isdigit(s0[1]))
    {
        *errors = s0[1] - '0';
        s0 += 3;
        if (*errors > 3)
            *errors = 3;
    }
    while (*s0)
    {
        if (strchr("^\\()[].*+?|-", *s0))
        {
            dst_term[j++] = *s0;
            dst[i++] = *s0++;
        }
        else
        {
            s1 = s0;
            map = zebra_maps_input(zebra_maps, reg_type, &s0, strlen(s0), 0);
            if (**map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (strchr(REGEX_CHARS, *s1))
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
    return term_103(zebra_maps, reg_type, src, dst, NULL, space_split,
                     dst_term);
}


/* term_104: handle term, where trunc=Process # and ! */
static int term_104(ZebraMaps zebra_maps, int reg_type,
                     const char **src, char *dst, int space_split,
                     char *dst_term)
{
    const char *s0, *s1;
    const char **map;
    int i = 0;
    int j = 0;

    if (!term_pre(zebra_maps, reg_type, src, "?*#", "?*#", !space_split))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '?')
        {
            dst_term[j++] = *s0++;
            if (*s0 >= '0' && *s0 <= '9')
            {
                int limit = 0;
                while (*s0 >= '0' && *s0 <= '9')
                {
                    limit = limit * 10 + (*s0 - '0');
                    dst_term[j++] = *s0++;
                }
                if (limit > 20)
                    limit = 20;
                while (--limit >= 0)
                {
                    dst[i++] = '.';
                    dst[i++] = '?';
                }
            }
            else
            {
                dst[i++] = '.';
                dst[i++] = '*';
            }
        }
        else if (*s0 == '*')
        {
            dst[i++] = '.';
            dst[i++] = '*';
            dst_term[j++] = *s0++;
        }
        else if (*s0 == '#')
        {
            dst[i++] = '.';
            dst_term[j++] = *s0++;
        }
        {
            s1 = s0;
            map = zebra_maps_input(zebra_maps, reg_type, &s0, strlen(s0), 0);
            if (space_split && **map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (strchr(REGEX_CHARS, *s1))
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

    if (!term_pre(zebra_maps, reg_type, src, "*!", "*!", !space_split))
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
            map = zebra_maps_input(zebra_maps, reg_type, &s0, strlen(s0), 0);
            if (space_split && **map == *CHR_SPACE)
                break;
            while (s1 < s0)
            {
                if (strchr(REGEX_CHARS, *s1))
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
static void gen_regular_rel(char *dst, int val, int islt)
{
    int dst_p;
    int w, d, i;
    int pos = 0;
    char numstr[20];

    yaz_log(YLOG_DEBUG, "gen_regular_rel. val=%d, islt=%d", val, islt);
    if (val >= 0)
    {
        if (islt)
            strcpy(dst, "(-[0-9]+|(");
        else
            strcpy(dst, "((");
    } 
    else
    {
        if (!islt)
        {
            strcpy(dst, "([0-9]+|-(");
            dst_p = strlen(dst);
            islt = 1;
        }
        else
        {
            strcpy(dst, "(-(");
            islt = 0;
        }
        val = -val;
    }
    dst_p = strlen(dst);
    sprintf(numstr, "%d", val);
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
        
        strcpy(dst + dst_p, numstr);
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
        strcat(dst, "0*");
        for (i=1; i<pos; i++)
            strcat(dst, "[0-9]?");
    }
    else
    {
        /* match everything greater than 10^pos */
        for (i = 0; i <= pos; i++)
            strcat(dst, "[0-9]");
        strcat(dst, "[0-9]*");
    }
    strcat(dst, "))");
}

void string_rel_add_char(char **term_p, const char *src, int *indx)
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
static int string_relation(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                            const char **term_sub, char *term_dict,
                            oid_value attributeSet,
                            int reg_type, int space_split, char *term_dst)
{
    AttrType relation;
    int relation_value;
    int i;
    char *term_tmp = term_dict + strlen(term_dict);
    char term_component[2*IT_MAX_WORD+20];

    attr_init(&relation, zapt, 2);
    relation_value = attr_find(&relation, NULL);

    yaz_log(YLOG_DEBUG, "string relation value=%d", relation_value);
    switch (relation_value)
    {
    case 1:
        if (!term_100 (zh->reg->zebra_maps, reg_type,
                       term_sub, term_component,
                       space_split, term_dst))
            return 0;
        yaz_log(log_level_rpn, "Relation <");
        
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

            if ((term_tmp - term_dict) > IT_MAX_WORD)
                break;
        }
        *term_tmp++ = ')';
        *term_tmp = '\0';
        break;
    case 2:
        if (!term_100 (zh->reg->zebra_maps, reg_type,
                       term_sub, term_component,
                       space_split, term_dst))
            return 0;
        yaz_log(log_level_rpn, "Relation <=");

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

            if ((term_tmp - term_dict) > IT_MAX_WORD)
                break;
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
        yaz_log(log_level_rpn, "Relation >");

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

            if ((term_tmp - term_dict) > IT_MAX_WORD)
                break;
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
        yaz_log(log_level_rpn, "Relation >=");

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

            if ((term_tmp - term_dict) > IT_MAX_WORD)
                break;
        }
        *term_tmp++ = ')';
        *term_tmp = '\0';
        break;
    case 3:
    default:
        yaz_log(log_level_rpn, "Relation =");
        if (!term_100 (zh->reg->zebra_maps, reg_type, term_sub,
                       term_component, space_split, term_dst))
            return 0;
        strcat(term_tmp, "(");
        strcat(term_tmp, term_component);
        strcat(term_tmp, ")");
    }
    return 1;
}

static int string_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                        const char **term_sub, 
                        oid_value attributeSet, NMEM stream,
                        struct grep_info *grep_info,
                        int reg_type, int complete_flag,
                        int num_bases, char **basenames,
                        char *term_dst, int xpath_use);

static RSET term_trunc(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                        const char **term_sub, 
                        oid_value attributeSet, NMEM stream,
                        struct grep_info *grep_info,
                        int reg_type, int complete_flag,
                        int num_bases, char **basenames,
                        char *term_dst,
                        const char *rank_type, int xpath_use,
                        NMEM rset_nmem)
{
    int r;
    grep_info->isam_p_indx = 0;
    r = string_term(zh, zapt, term_sub, attributeSet, stream, grep_info,
                     reg_type, complete_flag, num_bases, basenames,
                     term_dst, xpath_use);
    if (r < 1)
        return 0;
    yaz_log(log_level_rpn, "term: %s", term_dst);
    return rset_trunc(zh, grep_info->isam_p_buf,
                       grep_info->isam_p_indx, term_dst,
                       strlen(term_dst), rank_type, 1 /* preserve pos */,
                       zapt->term->which, rset_nmem,
                       key_it_ctrl,key_it_ctrl->scope);
}
static char *nmem_strdup_i(NMEM nmem, int v)
{
    char val_str[64];
    sprintf (val_str, "%d", v);
    return nmem_strdup(nmem, val_str);
}

static int string_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
		       const char **term_sub, 
		       oid_value attributeSet, NMEM stream,
		       struct grep_info *grep_info,
		       int reg_type, int complete_flag,
		       int num_bases, char **basenames,
		       char *term_dst, int xpath_use)
{
    char term_dict[2*IT_MAX_WORD+4000];
    int j, r, base_no;
    AttrType truncation;
    int truncation_value;
    AttrType use;
    int use_value;
    const char *use_string = 0;
    oid_value curAttributeSet = attributeSet;
    const char *termp;
    struct rpn_char_map_info rcmi;
    int space_split = complete_flag ? 0 : 1;

    int bases_ok = 0;     /* no of databases with OK attribute */
    int errCode = 0;      /* err code (if any is not OK) */
    char *errString = 0;  /* addinfo */

    rpn_char_map_prepare (zh->reg, reg_type, &rcmi);
    attr_init (&use, zapt, 1);
    use_value = attr_find_ex (&use, &curAttributeSet, &use_string);
    yaz_log(log_level_rpn, "string_term, use value %d", use_value);
    attr_init (&truncation, zapt, 5);
    truncation_value = attr_find (&truncation, NULL);
    yaz_log(log_level_rpn, "truncation value %d", truncation_value);

    if (use_value == -1)    /* no attribute - assumy "any" */
        use_value = 1016;
    for (base_no = 0; base_no < num_bases; base_no++)
    {
	int attr_ok = 0;
	int regex_range = 0;
	int init_pos = 0;
        attent attp;
        data1_local_attribute id_xpath_attr;
        data1_local_attribute *local_attr;
        int max_pos, prefix_len = 0;

        termp = *term_sub;

        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
            zh->errCode = 109; /* Database unavailable */
            zh->errString = basenames[base_no];
            return -1;
        }
        if (xpath_use > 0 && use_value == -2) 
        {
            use_value = xpath_use;
            attp.local_attributes = &id_xpath_attr;
            attp.attset_ordinal = VAL_IDXPATH;
            id_xpath_attr.next = 0;
            id_xpath_attr.local = use_value;
        }
        else if (curAttributeSet == VAL_IDXPATH)
        {
            attp.local_attributes = &id_xpath_attr;
            attp.attset_ordinal = VAL_IDXPATH;
            id_xpath_attr.next = 0;
            id_xpath_attr.local = use_value;
        }
        else
        {
            if ((r=att_getentbyatt (zh, &attp, curAttributeSet, use_value,
                                            use_string)))
            {
                yaz_log(YLOG_DEBUG, "att_getentbyatt fail. set=%d use=%d r=%d",
                      curAttributeSet, use_value, r);
                if (r == -1)
                {
                    /* set was found, but value wasn't defined */
                    errCode = 114;
                    if (use_string)
                        errString = nmem_strdup(stream, use_string);
                    else
                        errString = nmem_strdup_i (stream, use_value);
                }
                else
                {
                    int oid[OID_SIZE];
                    struct oident oident;
                    
                    oident.proto = PROTO_Z3950;
                    oident.oclass = CLASS_ATTSET;
                    oident.value = curAttributeSet;
                    oid_ent_to_oid (&oident, oid);
                    
                    errCode = 121;
                    errString = nmem_strdup (stream, oident.desc);
                }
                continue;
            }
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
#if 1
            bases_ok++;
#else
            errCode = 114;
            errString = nmem_strdup_i(stream, use_value);
            continue;
#endif
        }
	else
	{
	    bases_ok++; /* this has OK attributes */
	    attr_ok = 1;
	}

        term_dict[prefix_len++] = ')';
        term_dict[prefix_len++] = 1;
        term_dict[prefix_len++] = reg_type;
        yaz_log(log_level_rpn, "reg_type = %d", term_dict[prefix_len-1]);
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
	    break;
        case 1:          /* right truncation */
            term_dict[j++] = '(';
            if (!term_100(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
                return 0;
            strcat(term_dict, ".*)");
            break;
        case 2:          /* keft truncation */
            term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
            if (!term_100(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
                return 0;
            strcat(term_dict, ")");
            break;
        case 3:          /* left&right truncation */
            term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
            if (!term_100(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
                return 0;
            strcat(term_dict, ".*)");
            break;
        case 101:        /* process # in term */
            term_dict[j++] = '(';
            if (!term_101(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
                return 0;
            strcat(term_dict, ")");
            break;
        case 102:        /* Regexp-1 */
            term_dict[j++] = '(';
            if (!term_102(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
                return 0;
            strcat(term_dict, ")");
            break;
        case 103:       /* Regexp-2 */
            r = 1;
            term_dict[j++] = '(';
	    init_pos = 2;
            if (!term_103 (zh->reg->zebra_maps, reg_type,
                           &termp, term_dict + j, &regex_range,
			   space_split, term_dst))
                return 0;
            strcat(term_dict, ")");
        case 104:        /* process # and ! in term */
            term_dict[j++] = '(';
            if (!term_104 (zh->reg->zebra_maps, reg_type,
                           &termp, term_dict + j, space_split, term_dst))
                return 0;
            strcat(term_dict, ")");
            break;
        case 105:        /* process * and ! in term */
            term_dict[j++] = '(';
            if (!term_105 (zh->reg->zebra_maps, reg_type,
                           &termp, term_dict + j, space_split, term_dst, 1))
                return 0;
            strcat(term_dict, ")");
            break;
        case 106:        /* process * and ! in term */
            term_dict[j++] = '(';
            if (!term_105 (zh->reg->zebra_maps, reg_type,
                           &termp, term_dict + j, space_split, term_dst, 0))
                return 0;
            strcat(term_dict, ")");
            break;
	default:
	    zh->errCode = 120;
	    zh->errString = nmem_strdup_i(stream, truncation_value);
	    return -1;
        }
	if (attr_ok)
	{
	    yaz_log(log_level_rpn, "dict_lookup_grep: %s", term_dict+prefix_len);
	    r = dict_lookup_grep(zh->reg->dict, term_dict, regex_range,
				 grep_info, &max_pos, init_pos,
				 grep_handle);
	    if (r)
		yaz_log(YLOG_WARN, "dict_lookup_grep fail %d", r);
	}
    }
    if (!bases_ok)
    {
        zh->errCode = errCode;
        zh->errString = errString;
        return -1;
    }
    *term_sub = termp;
    yaz_log(YLOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 1;
}


/* convert APT search term to UTF8 */
static int zapt_term_to_utf8 (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                              char *termz)
{
    size_t sizez;
    Z_Term *term = zapt->term;

    switch (term->which)
    {
    case Z_Term_general:
        if (zh->iconv_to_utf8 != 0)
        {
            char *inbuf = term->u.general->buf;
            size_t inleft = term->u.general->len;
            char *outbuf = termz;
            size_t outleft = IT_MAX_WORD-1;
            size_t ret;

            ret = yaz_iconv(zh->iconv_to_utf8, &inbuf, &inleft,
                        &outbuf, &outleft);
            if (ret == (size_t)(-1))
            {
                ret = yaz_iconv(zh->iconv_to_utf8, 0, 0, 0, 0);
                zh->errCode = 125;
                return -1;
            }
            *outbuf = 0;
        }
        else
        {
            sizez = term->u.general->len;
            if (sizez > IT_MAX_WORD-1)
                sizez = IT_MAX_WORD-1;
            memcpy (termz, term->u.general->buf, sizez);
            termz[sizez] = '\0';
        }
        break;
    case Z_Term_characterString:
        sizez = strlen(term->u.characterString);
        if (sizez > IT_MAX_WORD-1)
            sizez = IT_MAX_WORD-1;
        memcpy (termz, term->u.characterString, sizez);
        termz[sizez] = '\0';
        break;
    default:
        zh->errCode = 124;
        return -1;
    }
    return 0;
}

/* convert APT SCAN term to internal cmap */
static int trans_scan_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                            char *termz, int reg_type)
{
    char termz0[IT_MAX_WORD];

    if (zapt_term_to_utf8(zh, zapt, termz0))
        return -1;    /* error */
    else
    {
        const char **map;
        const char *cp = (const char *) termz0;
        const char *cp_end = cp + strlen(cp);
        const char *src;
        int i = 0;
        const char *space_map = NULL;
        int len;
            
        while ((len = (cp_end - cp)) > 0)
        {
            map = zebra_maps_input (zh->reg->zebra_maps, reg_type, &cp, len, 0);
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
    return 0;
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
        char *buf = (char*) nmem_malloc(stream, wrbuf_len(wrbuf)+1);
        memcpy (buf, wrbuf_buf(wrbuf), wrbuf_len(wrbuf));
        buf[wrbuf_len(wrbuf)] = '\0';
        return buf;
    }
}

static void grep_info_delete (struct grep_info *grep_info)
{
#ifdef TERM_COUNT
    xfree(grep_info->term_no);
#endif
    xfree (grep_info->isam_p_buf);
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

    if (!zapt)
        return 0;
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
        yaz_log(log_level_rpn, "creating termset set %s", termset_name);
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
                                   const char *rank_type, int xpath_use,
                                   int num_bases, char **basenames, 
                                   NMEM rset_nmem)
{
    char term_dst[IT_MAX_WORD+1];
    RSET rset[60], result;
    int rset_no = 0;
    struct grep_info grep_info;
    char *termz = normalize_term(zh, zapt, termz_org, stream, reg_type);
    const char *termp = termz;

    *term_dst = 0;
    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
        return 0;
    while (1)
    { 
        yaz_log(log_level_rpn, "APT_phrase termp=%s", termp);
        rset[rset_no] = term_trunc(zh, zapt, &termp, attributeSet,
                                    stream, &grep_info,
                                    reg_type, complete_flag,
                                    num_bases, basenames,
                                    term_dst, rank_type,
                                    xpath_use,rset_nmem);
        if (!rset[rset_no])
            break;
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
    grep_info_delete (&grep_info);
    if (rset_no == 0)
        return rsnull_create (rset_nmem,key_it_ctrl); 
    else if (rset_no == 1)
        return (rset[0]);
    else
        result = rsprox_create( rset_nmem, key_it_ctrl, key_it_ctrl->scope,
                       rset_no, rset,
                       1 /* ordered */, 0 /* exclusion */,
                       3 /* relation */, 1 /* distance */);
    return result;
}

static RSET rpn_search_APT_or_list (ZebraHandle zh,
                                    Z_AttributesPlusTerm *zapt,
                                    const char *termz_org,
                                    oid_value attributeSet,
                                    NMEM stream,
                                    int reg_type, int complete_flag,
                                    const char *rank_type,
                                    int xpath_use,
                                    int num_bases, char **basenames,
                                    NMEM rset_nmem)
{
    char term_dst[IT_MAX_WORD+1];
    RSET rset[60];
    int rset_no = 0;
    struct grep_info grep_info;
    char *termz = normalize_term(zh, zapt, termz_org, stream, reg_type);
    const char *termp = termz;

    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
        return 0;
    while (1)
    { 
        yaz_log(log_level_rpn, "APT_or_list termp=%s", termp);
        rset[rset_no] = term_trunc(zh, zapt, &termp, attributeSet,
                                    stream, &grep_info,
                                    reg_type, complete_flag,
                                    num_bases, basenames,
                                    term_dst, rank_type,
                                    xpath_use,rset_nmem);
        if (!rset[rset_no])
            break;
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
    grep_info_delete (&grep_info);
    if (rset_no == 0)
        return rsnull_create (rset_nmem,key_it_ctrl);  
    return rsmultior_create(rset_nmem, key_it_ctrl,key_it_ctrl->scope,
                            rset_no, rset);
}

static RSET rpn_search_APT_and_list (ZebraHandle zh,
                                     Z_AttributesPlusTerm *zapt,
                                     const char *termz_org,
                                     oid_value attributeSet,
                                     NMEM stream,
                                     int reg_type, int complete_flag,
                                     const char *rank_type, 
                                     int xpath_use,
                                     int num_bases, char **basenames,
                                     NMEM rset_nmem)
{
    char term_dst[IT_MAX_WORD+1];
    RSET rset[60]; /* FIXME - bug 160 - should be dynamic somehow */
    int rset_no = 0;
    struct grep_info grep_info;
    char *termz = normalize_term(zh, zapt, termz_org, stream, reg_type);
    const char *termp = termz;

    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
        return 0;
    while (1)
    { 
        yaz_log(log_level_rpn, "APT_and_list termp=%s", termp);
        rset[rset_no] = term_trunc(zh, zapt, &termp, attributeSet,
                                    stream, &grep_info,
                                    reg_type, complete_flag,
                                    num_bases, basenames,
                                    term_dst, rank_type,
                                    xpath_use, rset_nmem);
        if (!rset[rset_no])
            break;
        assert (rset[rset_no]);
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
    grep_info_delete (&grep_info);
    if (rset_no == 0)
        return rsnull_create (rset_nmem,key_it_ctrl); 

    return rsmultiand_create( rset_nmem, key_it_ctrl, key_it_ctrl->scope,
                              rset_no, rset);
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

    yaz_log(log_level_rpn, "numeric relation value=%d", relation_value);

    if (!term_100 (zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
                   term_dst))
        return 0;
    term_value = atoi (term_tmp);
    switch (relation_value)
    {
    case 1:
        yaz_log(log_level_rpn, "Relation <");
        gen_regular_rel (term_tmp, term_value-1, 1);
        break;
    case 2:
        yaz_log(log_level_rpn, "Relation <=");
        gen_regular_rel (term_tmp, term_value, 1);
        break;
    case 4:
        yaz_log(log_level_rpn, "Relation >=");
        gen_regular_rel (term_tmp, term_value, 0);
        break;
    case 5:
        yaz_log(log_level_rpn, "Relation >");
        gen_regular_rel (term_tmp, term_value+1, 0);
        break;
    case 3:
    default:
        yaz_log(log_level_rpn, "Relation =");
        sprintf (term_tmp, "(0*%d)", term_value);
    }
    yaz_log(log_level_rpn, "dict_lookup_grep: %s", term_tmp);
    r = dict_lookup_grep(zh->reg->dict, term_dict, 0, grep_info, max_pos,
                          0, grep_handle);
    if (r)
        yaz_log(YLOG_WARN, "dict_lookup_grep fail, rel=gt: %d", r);
    yaz_log(log_level_rpn, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static int numeric_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                         const char **term_sub, 
                         oid_value attributeSet, struct grep_info *grep_info,
                         int reg_type, int complete_flag,
                         int num_bases, char **basenames,
                         char *term_dst, int xpath_use, NMEM stream)
{
    char term_dict[2*IT_MAX_WORD+2];
    int r, base_no;
    AttrType use;
    int use_value;
    const char *use_string = 0;
    oid_value curAttributeSet = attributeSet;
    const char *termp;
    struct rpn_char_map_info rcmi;

    int bases_ok = 0;     /* no of databases with OK attribute */
    int errCode = 0;      /* err code (if any is not OK) */
    char *errString = 0;  /* addinfo */

    rpn_char_map_prepare (zh->reg, reg_type, &rcmi);
    attr_init (&use, zapt, 1);
    use_value = attr_find_ex (&use, &curAttributeSet, &use_string);

    if (use_value == -1)
        use_value = 1016;

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        attent attp;
        data1_local_attribute id_xpath_attr;
        data1_local_attribute *local_attr;
        int max_pos, prefix_len = 0;

        termp = *term_sub;
        if (use_value == -2)  /* string attribute (assume IDXPATH/any) */
        {
            use_value = xpath_use;
            attp.local_attributes = &id_xpath_attr;
            attp.attset_ordinal = VAL_IDXPATH;
            id_xpath_attr.next = 0;
            id_xpath_attr.local = use_value;
        }
        else if (curAttributeSet == VAL_IDXPATH)
        {
            attp.local_attributes = &id_xpath_attr;
            attp.attset_ordinal = VAL_IDXPATH;
            id_xpath_attr.next = 0;
            id_xpath_attr.local = use_value;
        }
        else
        {
            if ((r=att_getentbyatt (zh, &attp, curAttributeSet, use_value,
                                            use_string)))
            {
                yaz_log(YLOG_DEBUG, "att_getentbyatt fail. set=%d use=%d r=%d",
                      curAttributeSet, use_value, r);
                if (r == -1)
                {
                    errString = nmem_strdup_i(stream, use_value);
                    errCode = 114;
                }
                else
                    errCode = 121;
                continue;
            }
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
            errCode = 114;
            errString = nmem_strdup_i(stream, use_value);
            continue;
        }
        bases_ok++;
        term_dict[prefix_len++] = ')';        
        term_dict[prefix_len++] = 1;
        term_dict[prefix_len++] = reg_type;
        yaz_log(YLOG_DEBUG, "reg_type = %d", term_dict[prefix_len-1]);
        term_dict[prefix_len] = '\0';
        if (!numeric_relation (zh, zapt, &termp, term_dict,
                               attributeSet, grep_info, &max_pos, reg_type,
                               term_dst))
            return 0;
    }
    if (!bases_ok)
    {
        zh->errCode = errCode;
        zh->errString = errString;
        return -1;
    }
    *term_sub = termp;
    yaz_log(YLOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static RSET rpn_search_APT_numeric (ZebraHandle zh,
                                    Z_AttributesPlusTerm *zapt,
                                    const char *termz,
                                    oid_value attributeSet,
                                    NMEM stream,
                                    int reg_type, int complete_flag,
                                    const char *rank_type, int xpath_use,
                                    int num_bases, char **basenames,
                                    NMEM rset_nmem)
{
    char term_dst[IT_MAX_WORD+1];
    const char *termp = termz;
    RSET rset[60]; /* FIXME - hard-coded magic number */
    int  r, rset_no = 0;
    struct grep_info grep_info;

    yaz_log(log_level_rpn, "APT_numeric t='%s'",termz);
    if (grep_info_prepare (zh, zapt, &grep_info, reg_type, stream))
        return 0;
    while (1)
    { 
        yaz_log(YLOG_DEBUG, "APT_numeric termp=%s", termp);
        grep_info.isam_p_indx = 0;
        r = numeric_term(zh, zapt, &termp, attributeSet, &grep_info,
                          reg_type, complete_flag, num_bases, basenames,
                          term_dst, xpath_use,
                          stream);
        if (r < 1)
            break;
        yaz_log(YLOG_DEBUG, "term: %s", term_dst);
        rset[rset_no] = rset_trunc(zh, grep_info.isam_p_buf,
                                    grep_info.isam_p_indx, term_dst,
                                    strlen(term_dst), rank_type,
                                    0 /* preserve position */,
                                    zapt->term->which, rset_nmem, 
                                    key_it_ctrl,key_it_ctrl->scope);
        assert (rset[rset_no]);
        if (++rset_no >= (int) (sizeof(rset)/sizeof(*rset)))
            break;
    }
    grep_info_delete (&grep_info);
    if (rset_no == 0)
        return rsnull_create (rset_nmem,key_it_ctrl);
    if (rset_no == 1)
        return rset[0];
    return rsmultiand_create(rset_nmem,key_it_ctrl,key_it_ctrl->scope,
               rset_no, rset);
}

static RSET rpn_search_APT_local (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                                  const char *termz,
                                  oid_value attributeSet,
                                  NMEM stream,
                                  const char *rank_type, NMEM rset_nmem)
{
    RSET result;
    RSFD rsfd;
    struct it_key key;
    int sys;
    result = rstemp_create( rset_nmem,key_it_ctrl,key_it_ctrl->scope,
                     res_get (zh->res, "setTmpDir"),0 );
    rsfd = rset_open (result, RSETF_WRITE);

    sys = atoi(termz);
    if (sys <= 0)
        sys = 1;
    key.mem[0] = sys;
    key.mem[1] = 1;
    key.len = 2;
    rset_write (rsfd, &key);
    rset_close (rsfd);
    return result;
}

static RSET rpn_sort_spec (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                           oid_value attributeSet, NMEM stream,
                           Z_SortKeySpecList *sort_sequence,
                           const char *rank_type)
{
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
            nmem_malloc(stream, sort_sequence->num_specs *
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

    sks = (Z_SortKeySpec *) nmem_malloc(stream, sizeof(*sks));
    sks->sortElement = (Z_SortElement *)
        nmem_malloc(stream, sizeof(*sks->sortElement));
    sks->sortElement->which = Z_SortElement_generic;
    sk = sks->sortElement->u.generic = (Z_SortKey *)
        nmem_malloc(stream, sizeof(*sk));
    sk->which = Z_SortKey_sortAttributes;
    sk->u.sortAttributes = (Z_SortAttributes *)
        nmem_malloc(stream, sizeof(*sk->u.sortAttributes));

    sk->u.sortAttributes->id = oid;
    sk->u.sortAttributes->list = (Z_AttributeList *)
        nmem_malloc(stream, sizeof(*sk->u.sortAttributes->list));
    sk->u.sortAttributes->list->num_attributes = 1;
    sk->u.sortAttributes->list->attributes = (Z_AttributeElement **)
        nmem_malloc(stream, sizeof(*sk->u.sortAttributes->list->attributes));
    ae = *sk->u.sortAttributes->list->attributes = (Z_AttributeElement *)
        nmem_malloc(stream, sizeof(**sk->u.sortAttributes->list->attributes));
    ae->attributeSet = 0;
    ae->attributeType = (int *)
        nmem_malloc(stream, sizeof(*ae->attributeType));
    *ae->attributeType = 1;
    ae->which = Z_AttributeValue_numeric;
    ae->value.numeric = (int *)
        nmem_malloc(stream, sizeof(*ae->value.numeric));
    *ae->value.numeric = use_value;

    sks->sortRelation = (int *)
        nmem_malloc(stream, sizeof(*sks->sortRelation));
    if (sort_relation_value == 1)
        *sks->sortRelation = Z_SortKeySpec_ascending;
    else if (sort_relation_value == 2)
        *sks->sortRelation = Z_SortKeySpec_descending;
    else 
        *sks->sortRelation = Z_SortKeySpec_ascending;

    sks->caseSensitivity = (int *)
        nmem_malloc(stream, sizeof(*sks->caseSensitivity));
    *sks->caseSensitivity = 0;

    sks->which = Z_SortKeySpec_null;
    sks->u.null = odr_nullval ();
    sort_sequence->specs[i] = sks;
    return rsnull_create (NULL,key_it_ctrl);
        /* FIXME - nmem?? */
}


static int parse_xpath(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                       oid_value attributeSet,
                       struct xpath_location_step *xpath, int max, NMEM mem)
{
    oid_value curAttributeSet = attributeSet;
    AttrType use;
    const char *use_string = 0;
    
    attr_init (&use, zapt, 1);
    attr_find_ex (&use, &curAttributeSet, &use_string);

    if (!use_string || *use_string != '/')
        return -1;

    return zebra_parse_xpath_str(use_string, xpath, max, mem);
}
 
               

static RSET xpath_trunc(ZebraHandle zh, NMEM stream,
                        int reg_type, const char *term, int use,
                        oid_value curAttributeSet, NMEM rset_nmem)
{
    RSET rset;
    struct grep_info grep_info;
    char term_dict[2048];
    char ord_buf[32];
    int prefix_len = 0;
    int ord = zebraExplain_lookupSU (zh->reg->zei, curAttributeSet, use);
    int ord_len, i, r, max_pos;
    int term_type = Z_Term_characterString;
    const char *flags = "void";

    if (grep_info_prepare (zh, 0 /* zapt */, &grep_info, '0', stream))
        return rsnull_create (rset_nmem,key_it_ctrl);

    if (ord < 0)
        return rsnull_create (rset_nmem,key_it_ctrl);
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
    term_dict[prefix_len++] = ')';
    term_dict[prefix_len++] = 1;
    term_dict[prefix_len++] = reg_type;
    
    strcpy(term_dict+prefix_len, term);
    
    grep_info.isam_p_indx = 0;
    r = dict_lookup_grep(zh->reg->dict, term_dict, 0,
                          &grep_info, &max_pos, 0, grep_handle);
    yaz_log (YLOG_LOG, "%s %d positions", term,
             grep_info.isam_p_indx);
    rset = rset_trunc(zh, grep_info.isam_p_buf,
                       grep_info.isam_p_indx, term, strlen(term),
                       flags, 1, term_type,rset_nmem,
                       key_it_ctrl, key_it_ctrl->scope);
    grep_info_delete (&grep_info);
    return rset;
}

static RSET rpn_search_xpath (ZebraHandle zh,
                              oid_value attributeSet,
                              int num_bases, char **basenames,
                              NMEM stream, const char *rank_type, RSET rset,
                              int xpath_len, struct xpath_location_step *xpath,
                              NMEM rset_nmem)
{
    oid_value curAttributeSet = attributeSet;
    int base_no;
    int i;

    if (xpath_len < 0)
        return rset;

    yaz_log (YLOG_DEBUG, "xpath len=%d", xpath_len);
    for (i = 0; i<xpath_len; i++)
    {
        yaz_log (log_level_rpn, "XPATH %d %s", i, xpath[i].part);

    }

    curAttributeSet = VAL_IDXPATH;

    /*
      //a    ->    a/.*
      //a/b  ->    b/a/.*
      /a     ->    a/
      /a/b   ->    b/a/

      /      ->    none

   a[@attr=value]/b[@other=othervalue]

 /e/@a val      range(e/,range(@a,freetext(w,1015,val),@a),e/)
 /a/b val       range(b/a/,freetext(w,1016,val),b/a/)
 /a/b/@c val    range(b/a/,range(@c,freetext(w,1016,val),@c),b/a/)
 /a/b[@c=y] val range(b/a/,freetext(w,1016,val),b/a/,@c=y)
 /a[@c=y]/b val range(a/,range(b/a/,freetext(w,1016,val),b/a/),a/,@c=y)
 /a[@c=x]/b[@c=y] range(a/,range(b/a/,freetext(w,1016,val),b/a/,@c=y),a/,@c=x)
      
    */

    dict_grep_cmap (zh->reg->dict, 0, 0);

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        int level = xpath_len;
        int first_path = 1;
        
        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
            zh->errCode = 109; /* Database unavailable */
            zh->errString = basenames[base_no];
            return rset;
        }
        while (--level >= 0)
        {
            char xpath_rev[128];
            int i, len;
            RSET rset_start_tag = 0, rset_end_tag = 0, rset_attr = 0;

            *xpath_rev = 0;
            len = 0;
            for (i = level; i >= 1; --i)
            {
                const char *cp = xpath[i].part;
                if (*cp)
                {
                    for (;*cp; cp++)
                        if (*cp == '*')
                        {
                            memcpy (xpath_rev + len, "[^/]*", 5);
                            len += 5;
                        }
                        else if (*cp == ' ')
                        {

                            xpath_rev[len++] = 1;
                            xpath_rev[len++] = ' ';
                        }

                        else
                            xpath_rev[len++] = *cp;
                    xpath_rev[len++] = '/';
                }
                else if (i == 1)  /* // case */
                {
                    xpath_rev[len++] = '.';
                    xpath_rev[len++] = '*';
                }
            }
            xpath_rev[len] = 0;

            if (xpath[level].predicate &&
                xpath[level].predicate->which == XPATH_PREDICATE_RELATION &&
                xpath[level].predicate->u.relation.name[0])
            {
                WRBUF wbuf = wrbuf_alloc();
                wrbuf_puts(wbuf, xpath[level].predicate->u.relation.name+1);
                if (xpath[level].predicate->u.relation.value)
                {
                    const char *cp = xpath[level].predicate->u.relation.value;
                    wrbuf_putc(wbuf, '=');
                    
                    while (*cp)
                    {
                        if (strchr(REGEX_CHARS, *cp))
                            wrbuf_putc(wbuf, '\\');
                        wrbuf_putc(wbuf, *cp);
                        cp++;
                    }
                }
                wrbuf_puts(wbuf, "");
                rset_attr = xpath_trunc(
                    zh, stream, '0', wrbuf_buf(wbuf), 3, 
                    curAttributeSet,rset_nmem);
                wrbuf_free(wbuf, 1);
            } 
            else 
            {
                if (!first_path)
                    continue;
            }
            yaz_log (log_level_rpn, "xpath_rev (%d) = %s", level, xpath_rev);
            if (strlen(xpath_rev))
            {
                rset_start_tag = xpath_trunc(zh, stream, '0', 
                        xpath_rev, 1, curAttributeSet, rset_nmem);
            
                rset_end_tag = xpath_trunc(zh, stream, '0', 
                        xpath_rev, 2, curAttributeSet, rset_nmem);

                /*
                parms.key_size = sizeof(struct it_key);
                parms.cmp = key_compare_it;
                parms.rset_l = rset_start_tag;
                parms.rset_m = rset;
                parms.rset_r = rset_end_tag;
                parms.rset_attr = rset_attr;
                parms.printer = key_print_it;
                rset = rset_create (rset_kind_between, &parms);
                */
                rset=rsbetween_create( rset_nmem,key_it_ctrl,key_it_ctrl->scope,
                        rset_start_tag, rset, rset_end_tag, rset_attr);
            }
            first_path = 0;
        }
    }

    return rset;
}



static RSET rpn_search_APT (ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                            oid_value attributeSet, NMEM stream,
                            Z_SortKeySpecList *sort_sequence,
                            int num_bases, char **basenames, 
                            NMEM rset_nmem)
{
    unsigned reg_id;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    char termz[IT_MAX_WORD+1];
    RSET rset = 0;
    int xpath_len;
    int xpath_use = 0;
    struct xpath_location_step xpath[10];

    if (!log_level_set)
    {
        log_level_rpn = yaz_log_module_level("rpn");
        log_level_set=1;
    }
    zebra_maps_attr (zh->reg->zebra_maps, zapt, &reg_id, &search_type,
                     rank_type, &complete_flag, &sort_flag);
    
    yaz_log(YLOG_DEBUG, "reg_id=%c", reg_id);
    yaz_log(YLOG_DEBUG, "complete_flag=%d", complete_flag);
    yaz_log(YLOG_DEBUG, "search_type=%s", search_type);
    yaz_log(YLOG_DEBUG, "rank_type=%s", rank_type);

    if (zapt_term_to_utf8(zh, zapt, termz))
        return 0;

    if (sort_flag)
        return rpn_sort_spec (zh, zapt, attributeSet, stream, sort_sequence,
                              rank_type);
    xpath_len = parse_xpath(zh, zapt, attributeSet, xpath, 10, stream);
    if (xpath_len >= 0)
    {
        xpath_use = 1016;
        if (xpath[xpath_len-1].part[0] == '@')
            xpath_use = 1015;
    }

    if (!strcmp (search_type, "phrase"))
    {
        rset = rpn_search_APT_phrase (zh, zapt, termz, attributeSet, stream,
                                      reg_id, complete_flag, rank_type,
                                      xpath_use,
                                      num_bases, basenames, rset_nmem);
    }
    else if (!strcmp (search_type, "and-list"))
    {
        rset = rpn_search_APT_and_list (zh, zapt, termz, attributeSet, stream,
                                        reg_id, complete_flag, rank_type,
                                        xpath_use,
                                        num_bases, basenames, rset_nmem);
    }
    else if (!strcmp (search_type, "or-list"))
    {
        rset = rpn_search_APT_or_list (zh, zapt, termz, attributeSet, stream,
                                       reg_id, complete_flag, rank_type,
                                       xpath_use,
                                       num_bases, basenames, rset_nmem);
    }
    else if (!strcmp (search_type, "local"))
    {
        rset = rpn_search_APT_local (zh, zapt, termz, attributeSet, stream,
                                     rank_type, rset_nmem);
    }
    else if (!strcmp (search_type, "numeric"))
    {
        rset = rpn_search_APT_numeric (zh, zapt, termz, attributeSet, stream,
                                       reg_id, complete_flag, rank_type,
                                       xpath_use,
                                       num_bases, basenames, rset_nmem);
    }
    else if (!strcmp (search_type, "always"))
    {
        rset = 0;
    }
    else
        zh->errCode = 118;
    return rpn_search_xpath (zh, attributeSet, num_bases, basenames,
                             stream, rank_type, rset, 
                             xpath_len, xpath, rset_nmem);
}

static RSET rpn_search_structure (ZebraHandle zh, Z_RPNStructure *zs,
                                  oid_value attributeSet, 
                                  NMEM stream, NMEM rset_nmem,
                                  Z_SortKeySpecList *sort_sequence,
                                  int num_bases, char **basenames)
{
    RSET r = NULL;
    if (zs->which == Z_RPNStructure_complex)
    {
        Z_Operator *zop = zs->u.complex->roperator;
        RSET rsets[2]; /* l and r argument */

        rsets[0]=rpn_search_structure (zh, zs->u.complex->s1,
                                       attributeSet, stream, rset_nmem,
                                       sort_sequence,
                                       num_bases, basenames);
        if (rsets[0] == NULL)
            return NULL;
        rsets[1]=rpn_search_structure (zh, zs->u.complex->s2,
                                       attributeSet, stream, rset_nmem,
                                       sort_sequence,
                                       num_bases, basenames);
        if (rsets[1] == NULL)
        {
            rset_delete (rsets[0]);
            return NULL;
        }

        switch (zop->which)
        {
        case Z_Operator_and:
            r=rsmultiand_create(rset_nmem, key_it_ctrl, key_it_ctrl->scope,
                        2, rsets);
            break;
        case Z_Operator_or:
            r=rsmultior_create(rset_nmem, key_it_ctrl, key_it_ctrl->scope,
                        2, rsets);
            break;
        case Z_Operator_and_not:
            r = rsbool_create_not(rset_nmem,key_it_ctrl, key_it_ctrl->scope,
                    rsets[0],rsets[1]);
            break;
        case Z_Operator_prox:
            if (zop->u.prox->which != Z_ProximityOperator_known)
            {
                zh->errCode = 132;
                return NULL;
            }
            if (*zop->u.prox->u.known != Z_ProxUnit_word)
            {
                char *val = (char *) nmem_malloc(stream, 16);
                zh->errCode = 132;
                zh->errString = val;
                sprintf (val, "%d", *zop->u.prox->u.known);
                return NULL;
            }
            else
            {
                /* new / old prox */
                r=rsprox_create(rset_nmem,key_it_ctrl,key_it_ctrl->scope,
                         2, rsets, 
                         *zop->u.prox->ordered,
                         (!zop->u.prox->exclusion ? 
                              0 : *zop->u.prox->exclusion),
                         *zop->u.prox->relationType,
                         *zop->u.prox->distance );
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
            yaz_log(YLOG_DEBUG, "rpn_search_APT");
            r = rpn_search_APT (zh, zs->u.simple->u.attributesPlusTerm,
                                attributeSet, stream, sort_sequence,
                                num_bases, basenames,rset_nmem);
        }
        else if (zs->u.simple->which == Z_Operand_resultSetId)
        {
            yaz_log(YLOG_DEBUG, "rpn_search_ref");
            r = resultSetRef (zh, zs->u.simple->u.resultSetId);
            if (!r)
            {
                r = rsnull_create (rset_nmem,key_it_ctrl);
                zh->errCode = 30;
                zh->errString =
                    nmem_strdup (stream, zs->u.simple->u.resultSetId);
                return 0;
            }
            else
                rset_dup(r);
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


RSET rpn_search(ZebraHandle zh, NMEM nmem, NMEM rset_nmem,
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
        nmem_malloc(nmem, sizeof(*sort_sequence));
    sort_sequence->num_specs = 10; /* FIXME - Hard-coded number */
    sort_sequence->specs = (Z_SortKeySpec **)
        nmem_malloc(nmem, sort_sequence->num_specs *
                     sizeof(*sort_sequence->specs));
    for (i = 0; i<sort_sequence->num_specs; i++)
        sort_sequence->specs[i] = 0;
    
    attrset = oid_getentbyoid (rpn->attributeSetId);
    attributeSet = attrset->value;
    rset = rpn_search_structure (zh, rpn->RPNStructure, attributeSet,
                                 nmem, rset_nmem,
                                 sort_sequence, num_bases, basenames);
    if (!rset)
        return 0;

    if (zh->errCode)
        yaz_log(YLOG_DEBUG, "search error: %d", zh->errCode);
    
    for (i = 0; sort_sequence->specs[i]; i++)
        ;
    sort_sequence->num_specs = i;
    if (!i)
        resultSetRank (zh, sset, rset, rset_nmem);
    else
    {
        yaz_log(YLOG_DEBUG, "resultSetSortSingle in rpn_search");
        resultSetSortSingle (zh, nmem, sset, rset,
                             sort_sequence, &sort_status);
        if (zh->errCode)
        {
            yaz_log(YLOG_DEBUG, "resultSetSortSingle status = %d", zh->errCode);
        }
    }
    return rset;
}

struct scan_info_entry {
    char *term;
    ISAMC_P isam_p;
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
        odr_malloc(scan_info->odr, strlen(name + len_prefix)+1);
    strcpy(scan_info->list[idx].term, name + len_prefix);
    assert (*info == sizeof(ISAMC_P));
    memcpy (&scan_info->list[idx].isam_p, info+1, sizeof(ISAMC_P));
    return 0;
}

static void scan_term_untrans (ZebraHandle zh, NMEM stream, int reg_type,
                               char **dst, const char *src)
{
    char term_src[IT_MAX_WORD];
    char term_dst[IT_MAX_WORD];
    
    term_untrans (zh, reg_type, term_src, src);

    if (zh->iconv_from_utf8 != 0)
    {
        int len;
        char *inbuf = term_src;
        size_t inleft = strlen(term_src);
        char *outbuf = term_dst;
        size_t outleft = sizeof(term_dst)-1;
        size_t ret;
        
        ret = yaz_iconv (zh->iconv_from_utf8, &inbuf, &inleft,
                         &outbuf, &outleft);
        if (ret == (size_t)(-1))
            len = 0;
        else
            len = outbuf - term_dst;
        *dst = nmem_malloc(stream, len + 1);
        if (len > 0)
            memcpy (*dst, term_dst, len);
        (*dst)[len] = '\0';
    }
    else
        *dst = nmem_strdup(stream, term_src);
}

static void count_set (RSET r, int *count)
{
    zint psysno = 0;
    int kno = 0;
    struct it_key key;
    RSFD rfd;

    yaz_log(YLOG_DEBUG, "count_set");

    *count = 0;
    rfd = rset_open (r, RSETF_READ);
    while (rset_read (rfd, &key,0 /* never mind terms */))
    {
        if (key.mem[0] != psysno)
        {
            psysno = key.mem[0];
            (*count)++;
        }
        kno++;
    }
    rset_close (rfd);
    yaz_log(YLOG_DEBUG, "%d keys, %d records", kno, *count);
}

void rpn_scan(ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
               oid_value attributeset,
               int num_bases, char **basenames,
               int *position, int *num_entries, ZebraScanEntry **list,
               int *is_partial, RSET limit_set, int return_zero)
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
    const char *use_string = 0;
    struct scan_info *scan_info_array;
    ZebraScanEntry *glist;
    int ords[32], ord_no = 0;
    int ptr[32];

    int bases_ok = 0;     /* no of databases with OK attribute */
    int errCode = 0;      /* err code (if any is not OK) */
    char *errString = 0;  /* addinfo */

    unsigned reg_id;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    NMEM rset_nmem=NULL; 

    *list = 0;

    if (attributeset == VAL_NONE)
        attributeset = VAL_BIB1;

    if (!limit_set)
    {
        AttrType termset;
        int termset_value_numeric;
        const char *termset_value_string;
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
            
            limit_set = resultSetRef (zh, termset_name);
        }
    }
        
    yaz_log (YLOG_DEBUG, "position = %d, num = %d set=%d",
             pos, num, attributeset);
        
    attr_init (&use, zapt, 1);
    use_value = attr_find_ex (&use, &attributeset, &use_string);

    if (zebra_maps_attr (zh->reg->zebra_maps, zapt, &reg_id, &search_type,
                         rank_type, &complete_flag, &sort_flag))
    {
        *num_entries = 0;
        zh->errCode = 113;
        return ;
    }
    yaz_log (YLOG_DEBUG, "use_value = %d", use_value);

    if (use_value == -1)
        use_value = 1016;
    for (base_no = 0; base_no < num_bases && ord_no < 32; base_no++)
    {
        int r;
        attent attp;
        data1_local_attribute *local_attr;

        if ((r=att_getentbyatt (zh, &attp, attributeset, use_value,
                                use_string)))
        {
            yaz_log(YLOG_DEBUG, "att_getentbyatt fail. set=%d use=%d",
                  attributeset, use_value);
            if (r == -1)
            {
                char val_str[32];
                sprintf (val_str, "%d", use_value);
                errCode = 114;
                errString = odr_strdup (stream, val_str);
            }   
            else
                errCode = 121;
            continue;
        }
        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
            zh->errString = basenames[base_no];
            zh->errCode = 109; /* Database unavailable */
            *num_entries = 0;
            return;
        }
        bases_ok++;
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
    if (!bases_ok && errCode)
    {
        zh->errCode = errCode;
        zh->errString = errString;
        *num_entries = 0;
    }
    if (ord_no == 0)
    {
        *num_entries = 0;
        return;
    }
    /* prepare dictionary scanning */
    before = pos-1;
    after = 1+num-pos;
    scan_info_array = (struct scan_info *)
        odr_malloc(stream, ord_no * sizeof(*scan_info_array));
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
            odr_malloc(stream, (before+after) * sizeof(*scan_info->list));
        for (j = 0; j<before+after; j++)
            scan_info->list[j].term = NULL;

        prefix_len += key_SU_encode (ords[i], termz + prefix_len);
        termz[prefix_len++] = reg_id;
        termz[prefix_len] = 0;
        strcpy(scan_info->prefix, termz);

        if (trans_scan_term(zh, zapt, termz+prefix_len, reg_id))
            return ;
                    
        dict_scan(zh->reg->dict, termz, &before_tmp, &after_tmp,
		  scan_info, scan_handle);
    }
    glist = (ZebraScanEntry *)
        odr_malloc(stream, (before+after)*sizeof(*glist));

    rset_nmem = nmem_create();

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
        rset = rset_trunc(zh, &scan_info_array[j0].list[ptr[j0]].isam_p, 1,
                           glist[i+before].term, strlen(glist[i+before].term),
                           NULL, 0, zapt->term->which, rset_nmem, 
                           key_it_ctrl,key_it_ctrl->scope);
        ptr[j0]++;
        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before+after &&
                (tst=scan_info_array[j].list[ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
                RSET rset2;

                rset2 =
                   rset_trunc(zh, &scan_info_array[j].list[ptr[j]].isam_p, 1,
                               glist[i+before].term,
                               strlen(glist[i+before].term), NULL, 0,
                               zapt->term->which,rset_nmem,
                               key_it_ctrl, key_it_ctrl->scope);
                rset = rsbool_create_or(rset_nmem,key_it_ctrl,
                               key_it_ctrl->scope, rset, rset2);
                /* FIXME - Use a proper multi-or */

                ptr[j]++;
            }
        }
        if (limit_set)
            rset = rsbool_create_and(rset_nmem,key_it_ctrl,key_it_ctrl->scope,
                            rset, rset_dup(limit_set));
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
                NULL, 0, zapt->term->which,rset_nmem,
                key_it_ctrl,key_it_ctrl->scope);

        ptr[j0]++;

        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before &&
                (tst=scan_info_array[j].list[before-1-ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
                RSET rset2;

                rset2 = rset_trunc(
		    zh,
		    &scan_info_array[j].list[before-1-ptr[j]].isam_p, 1,
		    glist[before-1-i].term,
		    strlen(glist[before-1-i].term), NULL, 0,
		    zapt->term->which, rset_nmem,
		    key_it_ctrl, key_it_ctrl->scope);
                rset = rsbool_create_and(rset_nmem,key_it_ctrl,
					 key_it_ctrl->scope, rset, rset2);
                /* FIXME - multi-and ?? */
                ptr[j]++;
            }
        }
        if (limit_set)
            rset = rsbool_create_and(rset_nmem,key_it_ctrl,key_it_ctrl->scope,
                            rset, rset_dup(limit_set));
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

    nmem_destroy(rset_nmem);
    *list = glist + i;               /* list is set to first 'real' entry */
    
    yaz_log(YLOG_DEBUG, "position = %d, num_entries = %d",
          *position, *num_entries);
    if (zh->errCode)
        yaz_log(YLOG_DEBUG, "scan error: %d", zh->errCode);
}
              
