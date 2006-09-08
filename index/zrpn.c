/* $Id: zrpn.c,v 1.229 2006-09-08 18:24:53 adam Exp $
   Copyright (C) 1995-2006
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include <yaz/diagbib1.h>
#include "index.h"
#include <zebra_xpath.h>
#include <attrfind.h>
#include <charmap.h>
#include <rset.h>

struct rpn_char_map_info
{
    ZebraMaps zm;
    int reg_type;
};

static int log_level_set = 0;
static int log_level_rpn = 0;

#define TERMSET_DISABLE 1

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
    ZebraSet termset;
};        

void zebra_term_untrans(ZebraHandle zh, int reg_type,
			char *dst, const char *src)
{
    int len = 0;
    while (*src)
    {
        const char *cp = zebra_maps_output(zh->reg->zebra_maps,
					   reg_type, &src);
	if (!cp)
	{
	    if (len < IT_MAX_WORD-1)
		dst[len++] = *src;
	    src++;
	}
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
        log_level_set = 1;
    }
    if (p->isam_p_indx == p->isam_p_size)
    {
        ISAM_P *new_isam_p_buf;
#ifdef TERM_COUNT        
        int *new_term_no;        
#endif
        p->isam_p_size = 2*p->isam_p_size + 100;
        new_isam_p_buf = (ISAM_P *) xmalloc(sizeof(*new_isam_p_buf) *
					    p->isam_p_size);
        if (p->isam_p_buf)
        {
            memcpy(new_isam_p_buf, p->isam_p_buf,
                    p->isam_p_indx * sizeof(*p->isam_p_buf));
            xfree(p->isam_p_buf);
        }
        p->isam_p_buf = new_isam_p_buf;

#ifdef TERM_COUNT
        new_term_no = (int *) xmalloc(sizeof(*new_term_no) * p->isam_p_size);
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

    if (p->termset)
    {
        const char *db;
        char term_tmp[IT_MAX_WORD];
        int ord = 0;
        const char *index_name;
        int len = key_SU_decode (&ord, (const unsigned char *) name);
        
        zebra_term_untrans  (p->zh, p->reg_type, term_tmp, name+len);
        yaz_log(log_level_rpn, "grep: %d %c %s", ord, name[len], term_tmp);
        zebraExplain_lookup_ord(p->zh->reg->zei,
                                ord, 0 /* index_type */, &db, &index_name);
        yaz_log(log_level_rpn, "grep:  db=%s index=%s", db, index_name);
        
        resultSetAddTerm(p->zh, p->termset, name[len], db,
			 index_name, term_tmp);
    }
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


static void esc_str(char *out_buf, size_t out_size,
		    const char *in_buf, int in_size)
{
    int k;

    assert(out_buf);
    assert(in_buf);
    assert(out_size > 20);
    *out_buf = '\0';
    for (k = 0; k<in_size; k++)
    {
	int c = in_buf[k] & 0xff;
	int pc;
	if (c < 32 || c > 126)
	    pc = '?';
	else
	    pc = c;
	sprintf(out_buf +strlen(out_buf), "%02X:%c  ", c, pc);
	if (strlen(out_buf) > out_size-20)
	{
	    strcat(out_buf, "..");
	    break;
	}
    }
}

#define REGEX_CHARS " []()|.*+?!"

/* term_100: handle term, where trunc = none(no operators at all) */
static int term_100(ZebraMaps zebra_maps, int reg_type,
		    const char **src, char *dst, int space_split,
		    char *dst_term)
{
    const char *s0;
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
        const char *s1 = s0;
	int q_map_match = 0;
        map = zebra_maps_search(zebra_maps, reg_type, &s0, strlen(s0), 
				&q_map_match);
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
	memcpy(dst_term+j, s1, s0 - s1);
	j += (s0 - s1);
	if (!q_map_match)
	{
	    while (s1 < s0)
	    {
		if (strchr(REGEX_CHARS, *s1))
		    dst[i++] = '\\';
		dst[i++] = *s1++;
	    }
	}
	else
	{
	    char tmpbuf[80];
	    esc_str(tmpbuf, sizeof(tmpbuf), map[0], strlen(map[0]));
	    
	    strcpy(dst + i, map[0]);
	    i += strlen(map[0]);
	}
    }
    dst[i] = '\0';
    dst_term[j] = '\0';
    *src = s0;
    return i;
}

/* term_101: handle term, where trunc = Process # */
static int term_101(ZebraMaps zebra_maps, int reg_type,
		    const char **src, char *dst, int space_split,
		    char *dst_term)
{
    const char *s0;
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
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zebra_maps, reg_type, &s0, strlen(s0), 
				    &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

	    /* add non-space char */
	    memcpy(dst_term+j, s1, s0 - s1);
	    j += (s0 - s1);
	    if (!q_map_match)
	    {
		while (s1 < s0)
		{
		    if (strchr(REGEX_CHARS, *s1))
			dst[i++] = '\\';
		    dst[i++] = *s1++;
		}
	    }
	    else
	    {
		char tmpbuf[80];
		esc_str(tmpbuf, sizeof(tmpbuf), map[0], strlen(map[0]));
		
		strcpy(dst + i, map[0]);
		i += strlen(map[0]);
	    }
        }
    }
    dst[i] = '\0';
    dst_term[j++] = '\0';
    *src = s0;
    return i;
}

/* term_103: handle term, where trunc = re-2 (regular expressions) */
static int term_103(ZebraMaps zebra_maps, int reg_type, const char **src,
		    char *dst, int *errors, int space_split,
		    char *dst_term)
{
    int i = 0;
    int j = 0;
    const char *s0;
    const char **map;

    if (!term_pre(zebra_maps, reg_type, src, "^\\()[].*+?|", "(", !space_split))
        return 0;
    s0 = *src;
    if (errors && *s0 == '+' && s0[1] && s0[2] == '+' && s0[3] &&
        isdigit(((const unsigned char *)s0)[1]))
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
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zebra_maps, reg_type, &s0, strlen(s0), 
				    &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

	    /* add non-space char */
	    memcpy(dst_term+j, s1, s0 - s1);
	    j += (s0 - s1);
	    if (!q_map_match)
	    {
		while (s1 < s0)
		{
		    if (strchr(REGEX_CHARS, *s1))
			dst[i++] = '\\';
		    dst[i++] = *s1++;
		}
	    }
	    else
	    {
		char tmpbuf[80];
		esc_str(tmpbuf, sizeof(tmpbuf), map[0], strlen(map[0]));
		
		strcpy(dst + i, map[0]);
		i += strlen(map[0]);
	    }
        }
    }
    dst[i] = '\0';
    dst_term[j] = '\0';
    *src = s0;
    
    return i;
}

/* term_103: handle term, where trunc = re-1 (regular expressions) */
static int term_102(ZebraMaps zebra_maps, int reg_type, const char **src,
		    char *dst, int space_split, char *dst_term)
{
    return term_103(zebra_maps, reg_type, src, dst, NULL, space_split,
		    dst_term);
}


/* term_104: handle term, where trunc = Process # and ! */
static int term_104(ZebraMaps zebra_maps, int reg_type,
		    const char **src, char *dst, int space_split,
		    char *dst_term)
{
    const char *s0;
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
	else
        {
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zebra_maps, reg_type, &s0, strlen(s0), 
				    &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

	    /* add non-space char */
	    memcpy(dst_term+j, s1, s0 - s1);
	    j += (s0 - s1);
	    if (!q_map_match)
	    {
		while (s1 < s0)
		{
		    if (strchr(REGEX_CHARS, *s1))
			dst[i++] = '\\';
		    dst[i++] = *s1++;
		}
	    }
	    else
	    {
		char tmpbuf[80];
		esc_str(tmpbuf, sizeof(tmpbuf), map[0], strlen(map[0]));
		
		strcpy(dst + i, map[0]);
		i += strlen(map[0]);
	    }
        }
    }
    dst[i] = '\0';
    dst_term[j++] = '\0';
    *src = s0;
    return i;
}

/* term_105/106: handle term, where trunc = Process * and ! and right trunc */
static int term_105(ZebraMaps zebra_maps, int reg_type,
		    const char **src, char *dst, int space_split,
		    char *dst_term, int right_truncate)
{
    const char *s0;
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
	else
        {
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zebra_maps, reg_type, &s0, strlen(s0), 
				    &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

	    /* add non-space char */
	    memcpy(dst_term+j, s1, s0 - s1);
	    j += (s0 - s1);
	    if (!q_map_match)
	    {
		while (s1 < s0)
		{
		    if (strchr(REGEX_CHARS, *s1))
			dst[i++] = '\\';
		    dst[i++] = *s1++;
		}
	    }
	    else
	    {
		char tmpbuf[80];
		esc_str(tmpbuf, sizeof(tmpbuf), map[0], strlen(map[0]));
		
		strcpy(dst + i, map[0]);
		i += strlen(map[0]);
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
        for (i = 1; i<pos; i++)
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
			   int reg_type, int space_split, char *term_dst,
			   int *error_code)
{
    AttrType relation;
    int relation_value;
    int i;
    char *term_tmp = term_dict + strlen(term_dict);
    char term_component[2*IT_MAX_WORD+20];

    attr_init_APT(&relation, zapt, 2);
    relation_value = attr_find(&relation, NULL);

    *error_code = 0;
    yaz_log(YLOG_DEBUG, "string relation value=%d", relation_value);
    switch (relation_value)
    {
    case 1:
        if (!term_100(zh->reg->zebra_maps, reg_type,
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
                string_rel_add_char(&term_tmp, term_component, &j);

            *term_tmp++ = '[';

            *term_tmp++ = '^';
            string_rel_add_char(&term_tmp, term_component, &i);
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
        if (!term_100(zh->reg->zebra_maps, reg_type,
		      term_sub, term_component,
		      space_split, term_dst))
            return 0;
        yaz_log(log_level_rpn, "Relation <=");

        *term_tmp++ = '(';
        for (i = 0; term_component[i]; )
        {
            int j = 0;

            while (j < i)
                string_rel_add_char(&term_tmp, term_component, &j);
            *term_tmp++ = '[';

            *term_tmp++ = '^';
            string_rel_add_char(&term_tmp, term_component, &i);
            *term_tmp++ = '-';

            *term_tmp++ = ']';
            *term_tmp++ = '.';
            *term_tmp++ = '*';

            *term_tmp++ = '|';

            if ((term_tmp - term_dict) > IT_MAX_WORD)
                break;
        }
        for (i = 0; term_component[i]; )
            string_rel_add_char(&term_tmp, term_component, &i);
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
                string_rel_add_char(&term_tmp, term_component, &j);
            *term_tmp++ = '[';
            
            *term_tmp++ = '^';
            *term_tmp++ = '-';
            string_rel_add_char(&term_tmp, term_component, &i);

            *term_tmp++ = ']';
            *term_tmp++ = '.';
            *term_tmp++ = '*';

            *term_tmp++ = '|';

            if ((term_tmp - term_dict) > IT_MAX_WORD)
                break;
        }
        for (i = 0; term_component[i];)
            string_rel_add_char(&term_tmp, term_component, &i);
        *term_tmp++ = '.';
        *term_tmp++ = '+';
        *term_tmp++ = ')';
        *term_tmp = '\0';
        break;
    case 4:
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub,
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
                string_rel_add_char(&term_tmp, term_component, &j);
            *term_tmp++ = '[';

            if (term_component[i+1])
            {
                *term_tmp++ = '^';
                *term_tmp++ = '-';
                string_rel_add_char(&term_tmp, term_component, &i);
            }
            else
            {
                string_rel_add_char(&term_tmp, term_component, &i);
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
    case 102:
    case -1:
        if (!**term_sub)
            return 1;
        yaz_log(log_level_rpn, "Relation =");
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub,
                      term_component, space_split, term_dst))
            return 0;
        strcat(term_tmp, "(");
        strcat(term_tmp, term_component);
        strcat(term_tmp, ")");
	break;
    case 103:
        yaz_log(log_level_rpn, "Relation always matches");
        /* skip to end of term (we don't care what it is) */
        while (**term_sub != '\0')
            (*term_sub)++;
        break;
    default:
	*error_code = YAZ_BIB1_UNSUPP_RELATION_ATTRIBUTE;
	return 0;
    }
    return 1;
}

static ZEBRA_RES string_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			     const char **term_sub, 
			     oid_value attributeSet, NMEM stream,
			     struct grep_info *grep_info,
			     int reg_type, int complete_flag,
			     int num_bases, char **basenames,
			     char *term_dst,
                             const char *xpath_use,
			     struct ord_list **ol);

static ZEBRA_RES term_limits_APT(ZebraHandle zh,
				 Z_AttributesPlusTerm *zapt,
				 zint *hits_limit_value,
				 const char **term_ref_id_str,
				 NMEM nmem)
{
    AttrType term_ref_id_attr;
    AttrType hits_limit_attr;
    int term_ref_id_int;
 
    attr_init_APT(&hits_limit_attr, zapt, 11);
    *hits_limit_value  = attr_find(&hits_limit_attr, NULL);

    attr_init_APT(&term_ref_id_attr, zapt, 10);
    term_ref_id_int = attr_find_ex(&term_ref_id_attr, NULL, term_ref_id_str);
    if (term_ref_id_int >= 0)
    {
	char *res = nmem_malloc(nmem, 20);
	sprintf(res, "%d", term_ref_id_int);
	*term_ref_id_str = res;
    }

    /* no limit given ? */
    if (*hits_limit_value == -1)
    {
	if (*term_ref_id_str)
	{
	    /* use global if term_ref is present */
	    *hits_limit_value = zh->approx_limit;
	}
	else
	{
	    /* no counting if term_ref is not present */
	    *hits_limit_value = 0;
	}
    }
    else if (*hits_limit_value == 0)
    {
	/* 0 is the same as global limit */
	*hits_limit_value = zh->approx_limit;
    }
    yaz_log(YLOG_DEBUG, "term_limits_APT ref_id=%s limit=" ZINT_FORMAT,
	    *term_ref_id_str ? *term_ref_id_str : "none",
	    *hits_limit_value);
    return ZEBRA_OK;
}

static ZEBRA_RES term_trunc(ZebraHandle zh,
			    Z_AttributesPlusTerm *zapt,
			    const char **term_sub, 
			    oid_value attributeSet, NMEM stream,
			    struct grep_info *grep_info,
			    int reg_type, int complete_flag,
			    int num_bases, char **basenames,
			    char *term_dst,
			    const char *rank_type, 
                            const char *xpath_use,
			    NMEM rset_nmem,
			    RSET *rset,
			    struct rset_key_control *kc)
{
    ZEBRA_RES res;
    struct ord_list *ol;
    zint hits_limit_value;
    const char *term_ref_id_str = 0;
    *rset = 0;

    term_limits_APT(zh, zapt, &hits_limit_value, &term_ref_id_str, stream);
    grep_info->isam_p_indx = 0;
    res = string_term(zh, zapt, term_sub, attributeSet, stream, grep_info,
		      reg_type, complete_flag, num_bases, basenames,
		      term_dst, xpath_use, &ol);
    if (res != ZEBRA_OK)
        return res;
    if (!*term_sub)  /* no more terms ? */
	return res;
    yaz_log(log_level_rpn, "term: %s", term_dst);
    *rset = rset_trunc(zh, grep_info->isam_p_buf,
		       grep_info->isam_p_indx, term_dst,
		       strlen(term_dst), rank_type, 1 /* preserve pos */,
		       zapt->term->which, rset_nmem,
		       kc, kc->scope, ol, reg_type, hits_limit_value,
		       term_ref_id_str);
    if (!*rset)
	return ZEBRA_FAIL;
    return ZEBRA_OK;
}

static ZEBRA_RES string_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			     const char **term_sub, 
			     oid_value attributeSet, NMEM stream,
			     struct grep_info *grep_info,
			     int reg_type, int complete_flag,
			     int num_bases, char **basenames,
			     char *term_dst,
                             const char *xpath_use,
			     struct ord_list **ol)
{
    char term_dict[2*IT_MAX_WORD+4000];
    int j, r, base_no;
    AttrType truncation;
    int truncation_value;
    const char *termp;
    struct rpn_char_map_info rcmi;
    int space_split = complete_flag ? 0 : 1;

    int bases_ok = 0;     /* no of databases with OK attribute */

    *ol = ord_list_create(stream);

    rpn_char_map_prepare (zh->reg, reg_type, &rcmi);
    attr_init_APT(&truncation, zapt, 5);
    truncation_value = attr_find(&truncation, NULL);
    yaz_log(log_level_rpn, "truncation value %d", truncation_value);

    for (base_no = 0; base_no < num_bases; base_no++)
    {
	int ord = -1;
	int regex_range = 0;
        int max_pos, prefix_len = 0;
	int relation_error;
        char ord_buf[32];
        int ord_len, i;

        termp = *term_sub; /* start of term for each database */

        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[base_no]);
            return ZEBRA_FAIL;
        }
        
        if (zebra_apt_get_ord(zh, zapt, reg_type, xpath_use,
                              attributeSet, &ord) != ZEBRA_OK)
            continue;

	bases_ok++;

        *ol = ord_list_append(stream, *ol, ord);
        ord_len = key_SU_encode (ord, ord_buf);
        
        term_dict[prefix_len++] = '(';
        for (i = 0; i<ord_len; i++)
        {
            term_dict[prefix_len++] = 1;  /* our internal regexp escape char */
            term_dict[prefix_len++] = ord_buf[i];
        }
        term_dict[prefix_len++] = ')';
        term_dict[prefix_len] = '\0';
        j = prefix_len;
        switch (truncation_value)
        {
        case -1:         /* not specified */
        case 100:        /* do not truncate */
            if (!string_relation(zh, zapt, &termp, term_dict,
                                 attributeSet,
                                 reg_type, space_split, term_dst,
				 &relation_error))
	    {
		if (relation_error)
		{
		    zebra_setError(zh, relation_error, 0);
		    return ZEBRA_FAIL;
		}
		*term_sub = 0;
		return ZEBRA_OK;
	    }
	    break;
        case 1:          /* right truncation */
            term_dict[j++] = '(';
            if (!term_100(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
	    {
		*term_sub = 0;
		return ZEBRA_OK;
	    }
            strcat(term_dict, ".*)");
            break;
        case 2:          /* keft truncation */
            term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
            if (!term_100(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
	    {
		*term_sub = 0;
		return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
            break;
        case 3:          /* left&right truncation */
            term_dict[j++] = '('; term_dict[j++] = '.'; term_dict[j++] = '*';
            if (!term_100(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
	    {
		*term_sub = 0;
		return ZEBRA_OK;
	    }
            strcat(term_dict, ".*)");
            break;
        case 101:        /* process # in term */
            term_dict[j++] = '(';
            if (!term_101(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
	    {
		*term_sub = 0;
                return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
            break;
        case 102:        /* Regexp-1 */
            term_dict[j++] = '(';
            if (!term_102(zh->reg->zebra_maps, reg_type,
			  &termp, term_dict + j, space_split, term_dst))
	    {
		*term_sub = 0;
                return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
            break;
        case 103:       /* Regexp-2 */
            regex_range = 1;
            term_dict[j++] = '(';
            if (!term_103(zh->reg->zebra_maps, reg_type,
                          &termp, term_dict + j, &regex_range,
			  space_split, term_dst))
	    {
		*term_sub = 0;
                return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
	    break;
        case 104:        /* process # and ! in term */
            term_dict[j++] = '(';
            if (!term_104(zh->reg->zebra_maps, reg_type,
                          &termp, term_dict + j, space_split, term_dst))
	    {
		*term_sub = 0;
                return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
            break;
        case 105:        /* process * and ! in term */
            term_dict[j++] = '(';
            if (!term_105(zh->reg->zebra_maps, reg_type,
                          &termp, term_dict + j, space_split, term_dst, 1))
	    {
		*term_sub = 0;
                return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
            break;
        case 106:        /* process * and ! in term */
            term_dict[j++] = '(';
            if (!term_105(zh->reg->zebra_maps, reg_type,
                          &termp, term_dict + j, space_split, term_dst, 0))
	    {
		*term_sub = 0;
                return ZEBRA_OK;
	    }
            strcat(term_dict, ")");
            break;
	default:
	    zebra_setError_zint(zh,
				YAZ_BIB1_UNSUPP_TRUNCATION_ATTRIBUTE,
				truncation_value);
	    return ZEBRA_FAIL;
        }
	if (1)
	{
	    char buf[80];
	    const char *input = term_dict + prefix_len;
	    esc_str(buf, sizeof(buf), input, strlen(input));
	}
        yaz_log(log_level_rpn, "dict_lookup_grep: %s", term_dict+prefix_len);
        r = dict_lookup_grep(zh->reg->dict, term_dict, regex_range,
                             grep_info, &max_pos, 
                             ord_len /* number of "exact" chars */,
                             grep_handle);
        if (r)
            yaz_log(YLOG_WARN, "dict_lookup_grep fail %d", r);
    }
    if (!bases_ok)
        return ZEBRA_FAIL;
    *term_sub = termp;
    yaz_log(YLOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return ZEBRA_OK;
}


/* convert APT search term to UTF8 */
static ZEBRA_RES zapt_term_to_utf8(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				   char *termz)
{
    size_t sizez;
    Z_Term *term = zapt->term;

    switch (term->which)
    {
    case Z_Term_general:
        if (zh->iconv_to_utf8 != 0)
        {
            char *inbuf = (char *) term->u.general->buf;
            size_t inleft = term->u.general->len;
            char *outbuf = termz;
            size_t outleft = IT_MAX_WORD-1;
            size_t ret;

            ret = yaz_iconv(zh->iconv_to_utf8, &inbuf, &inleft,
                        &outbuf, &outleft);
            if (ret == (size_t)(-1))
            {
                ret = yaz_iconv(zh->iconv_to_utf8, 0, 0, 0, 0);
		zebra_setError(
		    zh, 
		    YAZ_BIB1_QUERY_TERM_INCLUDES_CHARS_THAT_DO_NOT_TRANSLATE_INTO_,
		    0);
                return ZEBRA_FAIL;
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
	zebra_setError(zh, YAZ_BIB1_UNSUPP_CODED_VALUE_FOR_TERM, 0);
	return ZEBRA_FAIL;
    }
    return ZEBRA_OK;
}

/* convert APT SCAN term to internal cmap */
static ZEBRA_RES trans_scan_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				 char *termz, int reg_type)
{
    char termz0[IT_MAX_WORD];

    if (zapt_term_to_utf8(zh, zapt, termz0) == ZEBRA_FAIL)
        return ZEBRA_FAIL;    /* error */
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
            map = zebra_maps_input(zh->reg->zebra_maps, reg_type, &cp, len, 0);
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
    return ZEBRA_OK;
}

static void grep_info_delete(struct grep_info *grep_info)
{
#ifdef TERM_COUNT
    xfree(grep_info->term_no);
#endif
    xfree(grep_info->isam_p_buf);
}

static ZEBRA_RES grep_info_prepare(ZebraHandle zh,
				   Z_AttributesPlusTerm *zapt,
				   struct grep_info *grep_info,
				   int reg_type)
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
        return ZEBRA_OK;
    attr_init_APT(&termset, zapt, 8);
    termset_value_numeric =
        attr_find_ex(&termset, NULL, &termset_value_string);
    if (termset_value_numeric != -1)
    {
#if TERMSET_DISABLE
        zebra_setError(zh, YAZ_BIB1_UNSUPP_SEARCH, "termset");
        return ZEBRA_FAIL;
#else
        char resname[32];
        const char *termset_name = 0;
        if (termset_value_numeric != -2)
        {
    
            sprintf(resname, "%d", termset_value_numeric);
            termset_name = resname;
        }
        else
            termset_name = termset_value_string;
        yaz_log(log_level_rpn, "creating termset set %s", termset_name);
        grep_info->termset = resultSetAdd(zh, termset_name, 1);
        if (!grep_info->termset)
        {
	    zebra_setError(zh, YAZ_BIB1_ILLEGAL_RESULT_SET_NAME, termset_name);
            return ZEBRA_FAIL;
        }
#endif
    }
    return ZEBRA_OK;
}
                               
/**
  \brief Create result set(s) for list of terms
  \param zh Zebra Handle
  \param zapt Attributes Plust Term (RPN leaf)
  \param termz term as used in query but converted to UTF-8
  \param attributeSet default attribute set
  \param stream memory for result
  \param reg_type register type ('w', 'p',..)
  \param complete_flag whether it's phrases or not
  \param rank_type term flags for ranking
  \param xpath_use use attribute for X-Path (-1 for no X-path)
  \param num_bases number of databases
  \param basenames array of databases
  \param rset_nmem memory for result sets
  \param result_sets output result set for each term in list (output)
  \param num_result_sets number of output result sets
  \param kc rset key control to be used for created result sets
*/
static ZEBRA_RES term_list_trunc(ZebraHandle zh,
				 Z_AttributesPlusTerm *zapt,
				 const char *termz,
				 oid_value attributeSet,
				 NMEM stream,
				 int reg_type, int complete_flag,
				 const char *rank_type,
                                 const char *xpath_use,
				 int num_bases, char **basenames, 
				 NMEM rset_nmem,
				 RSET **result_sets, int *num_result_sets,
				 struct rset_key_control *kc)
{
    char term_dst[IT_MAX_WORD+1];
    struct grep_info grep_info;
    const char *termp = termz;
    int alloc_sets = 0;

    *num_result_sets = 0;
    *term_dst = 0;
    if (grep_info_prepare(zh, zapt, &grep_info, reg_type) == ZEBRA_FAIL)
        return ZEBRA_FAIL;
    while(1)
    { 
	ZEBRA_RES res;

	if (alloc_sets == *num_result_sets)
	{
	    int add = 10;
	    RSET *rnew = (RSET *) nmem_malloc(stream, (alloc_sets+add) * 
					      sizeof(*rnew));
	    if (alloc_sets)
		memcpy(rnew, *result_sets, alloc_sets * sizeof(*rnew));
	    alloc_sets = alloc_sets + add;
	    *result_sets = rnew;
	}
        res = term_trunc(zh, zapt, &termp, attributeSet,
			 stream, &grep_info,
			 reg_type, complete_flag,
			 num_bases, basenames,
			 term_dst, rank_type,
			 xpath_use, rset_nmem,
			 &(*result_sets)[*num_result_sets],
			 kc);
	if (res != ZEBRA_OK)
	{
	    int i;
	    for (i = 0; i < *num_result_sets; i++)
		rset_delete((*result_sets)[i]);
	    grep_info_delete (&grep_info);
	    return res;
	}
	if ((*result_sets)[*num_result_sets] == 0)
	    break;
	(*num_result_sets)++;

        if (!*termp)
            break;
    }
    grep_info_delete(&grep_info);
    return ZEBRA_OK;
}

static ZEBRA_RES rpn_search_APT_position(ZebraHandle zh,
                                         Z_AttributesPlusTerm *zapt,
                                         oid_value attributeSet,
                                         int reg_type,
                                         int num_bases, char **basenames,
                                         NMEM rset_nmem,
                                         RSET *rset,
                                         struct rset_key_control *kc)
{
    RSET *f_set;
    int base_no;
    int position_value;
    int num_sets = 0;
    AttrType position;

    attr_init_APT(&position, zapt, 3);
    position_value = attr_find(&position, NULL);
    switch(position_value)
    {
    case 3:
    case -1:
        return ZEBRA_OK;
    case 1:
    case 2:
        break;
    default:
        zebra_setError_zint(zh, YAZ_BIB1_UNSUPP_POSITION_ATTRIBUTE,
                            position_value);
        return ZEBRA_FAIL;
    }

    if (!zebra_maps_is_first_in_field(zh->reg->zebra_maps, reg_type))
    {
        zebra_setError_zint(zh, YAZ_BIB1_UNSUPP_POSITION_ATTRIBUTE,
                            position_value);
        return ZEBRA_FAIL;
    }

    if (!zh->reg->isamb && !zh->reg->isamc)
    {
        zebra_setError_zint(zh, YAZ_BIB1_UNSUPP_POSITION_ATTRIBUTE,
                            position_value);
        return ZEBRA_FAIL;
    }
    f_set = xmalloc(sizeof(RSET) * num_bases);
    for (base_no = 0; base_no < num_bases; base_no++)
    {
	int ord = -1;
        char ord_buf[32];
        char term_dict[100];
        int ord_len;
        char *val;
        ISAM_P isam_p;

        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[base_no]);
            return ZEBRA_FAIL;
        }
        
        if (zebra_apt_get_ord(zh, zapt, reg_type, 0,
                              attributeSet, &ord) != ZEBRA_OK)
            continue;

        ord_len = key_SU_encode (ord, ord_buf);
        memcpy(term_dict, ord_buf, ord_len);
        strcpy(term_dict+ord_len, FIRST_IN_FIELD_STR);
        val = dict_lookup(zh->reg->dict, term_dict);
        if (!val)
            continue;
        assert(*val == sizeof(ISAM_P));
        memcpy(&isam_p, val+1, sizeof(isam_p));
        

        if (zh->reg->isamb)
            f_set[num_sets++] = rsisamb_create(rset_nmem, kc, kc->scope,
                                               zh->reg->isamb, isam_p, 0);
        else if (zh->reg->isamc)
            f_set[num_sets++] = rsisamc_create(rset_nmem, kc, kc->scope,
                                               zh->reg->isamc, isam_p, 0);
    }
    if (num_sets)
    {
        *rset = rset_create_or(rset_nmem, kc, kc->scope,
                               0 /* termid */, num_sets, f_set);
    }
    xfree(f_set);
    return ZEBRA_OK;
}
                                         
static ZEBRA_RES rpn_search_APT_phrase(ZebraHandle zh,
				       Z_AttributesPlusTerm *zapt,
				       const char *termz_org,
				       oid_value attributeSet,
				       NMEM stream,
				       int reg_type, int complete_flag,
				       const char *rank_type,
                                       const char *xpath_use,
				       int num_bases, char **basenames, 
				       NMEM rset_nmem,
				       RSET *rset,
				       struct rset_key_control *kc)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    ZEBRA_RES res =
	term_list_trunc(zh, zapt, termz_org, attributeSet,
			stream, reg_type, complete_flag,
			rank_type, xpath_use,
			num_bases, basenames,
			rset_nmem,
			&result_sets, &num_result_sets, kc);

    if (res != ZEBRA_OK)
	return res;

    if (num_result_sets > 0)
    {
        RSET first_set = 0;
        res = rpn_search_APT_position(zh, zapt, attributeSet, 
                                      reg_type,
                                      num_bases, basenames,
                                      rset_nmem, &first_set,
                                      kc);
        if (res != ZEBRA_OK)
            return res;
        if (first_set)
        {
            RSET *nsets = nmem_malloc(stream,
                                      sizeof(RSET) * (num_result_sets+1));
            nsets[0] = first_set;
            memcpy(nsets+1, result_sets, sizeof(RSET) * num_result_sets);
            result_sets = nsets;
            num_result_sets++;
        }
    }
    if (num_result_sets == 0)
	*rset = rset_create_null(rset_nmem, kc, 0); 
    else if (num_result_sets == 1)
	*rset = result_sets[0];
    else
	*rset = rset_create_prox(rset_nmem, kc, kc->scope,
                                 num_result_sets, result_sets,
                                 1 /* ordered */, 0 /* exclusion */,
                                 3 /* relation */, 1 /* distance */);
    if (!*rset)
	return ZEBRA_FAIL;
    return ZEBRA_OK;
}

static ZEBRA_RES rpn_search_APT_or_list(ZebraHandle zh,
					Z_AttributesPlusTerm *zapt,
					const char *termz_org,
					oid_value attributeSet,
					NMEM stream,
					int reg_type, int complete_flag,
					const char *rank_type,
                                        const char *xpath_use,
					int num_bases, char **basenames,
					NMEM rset_nmem,
					RSET *rset,
					struct rset_key_control *kc)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    int i;
    ZEBRA_RES res =
	term_list_trunc(zh, zapt, termz_org, attributeSet,
			stream, reg_type, complete_flag,
			rank_type, xpath_use,
			num_bases, basenames,
			rset_nmem,
			&result_sets, &num_result_sets, kc);
    if (res != ZEBRA_OK)
	return res;

    for (i = 0; i<num_result_sets; i++)
    {
        RSET first_set = 0;
        res = rpn_search_APT_position(zh, zapt, attributeSet, 
                                      reg_type,
                                      num_bases, basenames,
                                      rset_nmem, &first_set,
                                      kc);
        if (res != ZEBRA_OK)
        {
            for (i = 0; i<num_result_sets; i++)
                rset_delete(result_sets[i]);
            return res;
        }

        if (first_set)
        {
            RSET tmp_set[2];

            tmp_set[0] = first_set;
            tmp_set[1] = result_sets[i];
            
            result_sets[i] = rset_create_prox(
                rset_nmem, kc, kc->scope,
                2, tmp_set,
                1 /* ordered */, 0 /* exclusion */,
                3 /* relation */, 1 /* distance */);
        }
    }
    if (num_result_sets == 0)
	*rset = rset_create_null(rset_nmem, kc, 0); 
    else if (num_result_sets == 1)
	*rset = result_sets[0];
    else
	*rset = rset_create_or(rset_nmem, kc, kc->scope, 0 /* termid */,
                               num_result_sets, result_sets);
    if (!*rset)
	return ZEBRA_FAIL;
    return ZEBRA_OK;
}

static ZEBRA_RES rpn_search_APT_and_list(ZebraHandle zh,
					 Z_AttributesPlusTerm *zapt,
					 const char *termz_org,
					 oid_value attributeSet,
					 NMEM stream,
					 int reg_type, int complete_flag,
					 const char *rank_type, 
                                         const char *xpath_use,
					 int num_bases, char **basenames,
					 NMEM rset_nmem,
					 RSET *rset,
					 struct rset_key_control *kc)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    int i;
    ZEBRA_RES res =
	term_list_trunc(zh, zapt, termz_org, attributeSet,
			stream, reg_type, complete_flag,
			rank_type, xpath_use,
			num_bases, basenames,
			rset_nmem,
			&result_sets, &num_result_sets,
			kc);
    if (res != ZEBRA_OK)
	return res;
    for (i = 0; i<num_result_sets; i++)
    {
        RSET first_set = 0;
        res = rpn_search_APT_position(zh, zapt, attributeSet, 
                                      reg_type,
                                      num_bases, basenames,
                                      rset_nmem, &first_set,
                                      kc);
        if (res != ZEBRA_OK)
        {
            for (i = 0; i<num_result_sets; i++)
                rset_delete(result_sets[i]);
            return res;
        }

        if (first_set)
        {
            RSET tmp_set[2];

            tmp_set[0] = first_set;
            tmp_set[1] = result_sets[i];
            
            result_sets[i] = rset_create_prox(
                rset_nmem, kc, kc->scope,
                2, tmp_set,
                1 /* ordered */, 0 /* exclusion */,
                3 /* relation */, 1 /* distance */);
        }
    }


    if (num_result_sets == 0)
	*rset = rset_create_null(rset_nmem, kc, 0); 
    else if (num_result_sets == 1)
	*rset = result_sets[0];
    else
	*rset = rset_create_and(rset_nmem, kc, kc->scope,
                                num_result_sets, result_sets);
    if (!*rset)
	return ZEBRA_FAIL;
    return ZEBRA_OK;
}

static int numeric_relation(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			    const char **term_sub,
			    char *term_dict,
			    oid_value attributeSet,
			    struct grep_info *grep_info,
			    int *max_pos,
			    int reg_type,
			    char *term_dst,
			    int *error_code)
{
    AttrType relation;
    int relation_value;
    int term_value;
    int r;
    char *term_tmp = term_dict + strlen(term_dict);

    *error_code = 0;
    attr_init_APT(&relation, zapt, 2);
    relation_value = attr_find(&relation, NULL);

    yaz_log(log_level_rpn, "numeric relation value=%d", relation_value);

    switch (relation_value)
    {
    case 1:
        yaz_log(log_level_rpn, "Relation <");
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
                      term_dst))
            return 0;
        term_value = atoi (term_tmp);
        gen_regular_rel(term_tmp, term_value-1, 1);
        break;
    case 2:
        yaz_log(log_level_rpn, "Relation <=");
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
                      term_dst))
            return 0;
        term_value = atoi (term_tmp);
        gen_regular_rel(term_tmp, term_value, 1);
        break;
    case 4:
        yaz_log(log_level_rpn, "Relation >=");
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
                      term_dst))
            return 0;
        term_value = atoi (term_tmp);
        gen_regular_rel(term_tmp, term_value, 0);
        break;
    case 5:
        yaz_log(log_level_rpn, "Relation >");
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
                      term_dst))
            return 0;
        term_value = atoi (term_tmp);
        gen_regular_rel(term_tmp, term_value+1, 0);
        break;
    case -1:
    case 3:
        yaz_log(log_level_rpn, "Relation =");
        if (!term_100(zh->reg->zebra_maps, reg_type, term_sub, term_tmp, 1,
                      term_dst))
            return 0;
        term_value = atoi (term_tmp);
        sprintf(term_tmp, "(0*%d)", term_value);
	break;
    case 103:
        /* term_tmp untouched.. */
        while (**term_sub != '\0')
            (*term_sub)++;
        break;
    default:
	*error_code = YAZ_BIB1_UNSUPP_RELATION_ATTRIBUTE;
	return 0;
    }
    yaz_log(log_level_rpn, "dict_lookup_grep: %s", term_tmp);
    r = dict_lookup_grep(zh->reg->dict, term_dict, 0, grep_info, max_pos,
                          0, grep_handle);
    if (r)
        yaz_log(YLOG_WARN, "dict_lookup_grep fail, rel = gt: %d", r);
    yaz_log(log_level_rpn, "%d positions", grep_info->isam_p_indx);
    return 1;
}

static ZEBRA_RES numeric_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			      const char **term_sub, 
			      oid_value attributeSet, NMEM stream,
			      struct grep_info *grep_info,
			      int reg_type, int complete_flag,
			      int num_bases, char **basenames,
			      char *term_dst, 
                              const char *xpath_use,
                              struct ord_list **ol)
{
    char term_dict[2*IT_MAX_WORD+2];
    int base_no;
    const char *termp;
    struct rpn_char_map_info rcmi;

    int bases_ok = 0;     /* no of databases with OK attribute */

    *ol = ord_list_create(stream);

    rpn_char_map_prepare (zh->reg, reg_type, &rcmi);

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        int max_pos, prefix_len = 0;
	int relation_error = 0;
        int ord, ord_len, i;
        char ord_buf[32];

        termp = *term_sub;

        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[base_no]);
            return ZEBRA_FAIL;
        }

        if (zebra_apt_get_ord(zh, zapt, reg_type, xpath_use,
                              attributeSet, &ord) != ZEBRA_OK)
            continue;
        bases_ok++;

        *ol = ord_list_append(stream, *ol, ord);

        ord_len = key_SU_encode (ord, ord_buf);

        term_dict[prefix_len++] = '(';
        for (i = 0; i < ord_len; i++)
        {
            term_dict[prefix_len++] = 1;
            term_dict[prefix_len++] = ord_buf[i];
        }
        term_dict[prefix_len++] = ')';
        term_dict[prefix_len] = '\0';

        if (!numeric_relation(zh, zapt, &termp, term_dict,
			      attributeSet, grep_info, &max_pos, reg_type,
			      term_dst, &relation_error))
	{
	    if (relation_error)
	    {
		zebra_setError(zh, relation_error, 0);
		return ZEBRA_FAIL;
	    }
	    *term_sub = 0;
            return ZEBRA_OK;
	}
    }
    if (!bases_ok)
        return ZEBRA_FAIL;
    *term_sub = termp;
    yaz_log(YLOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return ZEBRA_OK;
}

				 
static ZEBRA_RES rpn_search_APT_numeric(ZebraHandle zh,
					Z_AttributesPlusTerm *zapt,
					const char *termz,
					oid_value attributeSet,
					NMEM stream,
					int reg_type, int complete_flag,
					const char *rank_type, 
                                        const char *xpath_use,
					int num_bases, char **basenames,
					NMEM rset_nmem,
					RSET *rset,
					struct rset_key_control *kc)
{
    char term_dst[IT_MAX_WORD+1];
    const char *termp = termz;
    RSET *result_sets = 0;
    int num_result_sets = 0;
    ZEBRA_RES res;
    struct grep_info grep_info;
    int alloc_sets = 0;
    zint hits_limit_value;
    const char *term_ref_id_str = 0;

    term_limits_APT(zh, zapt, &hits_limit_value, &term_ref_id_str, stream);

    yaz_log(log_level_rpn, "APT_numeric t='%s'", termz);
    if (grep_info_prepare(zh, zapt, &grep_info, reg_type) == ZEBRA_FAIL)
        return ZEBRA_FAIL;
    while (1)
    { 
        struct ord_list *ol;
	if (alloc_sets == num_result_sets)
	{
	    int add = 10;
	    RSET *rnew = (RSET *) nmem_malloc(stream, (alloc_sets+add) * 
					      sizeof(*rnew));
	    if (alloc_sets)
		memcpy(rnew, result_sets, alloc_sets * sizeof(*rnew));
	    alloc_sets = alloc_sets + add;
	    result_sets = rnew;
	}
        yaz_log(YLOG_DEBUG, "APT_numeric termp=%s", termp);
        grep_info.isam_p_indx = 0;
        res = numeric_term(zh, zapt, &termp, attributeSet, stream, &grep_info,
			   reg_type, complete_flag, num_bases, basenames,
			   term_dst, xpath_use, &ol);
	if (res == ZEBRA_FAIL || termp == 0)
	    break;
        yaz_log(YLOG_DEBUG, "term: %s", term_dst);
        result_sets[num_result_sets] =
	    rset_trunc(zh, grep_info.isam_p_buf,
		       grep_info.isam_p_indx, term_dst,
		       strlen(term_dst), rank_type,
		       0 /* preserve position */,
		       zapt->term->which, rset_nmem, 
		       kc, kc->scope, ol, reg_type,
		       hits_limit_value,
		       term_ref_id_str);
	if (!result_sets[num_result_sets])
	    break;
	num_result_sets++;
        if (!*termp)
            break;
    }
    grep_info_delete(&grep_info);

    if (res != ZEBRA_OK)
        return res;
    if (num_result_sets == 0)
        *rset = rset_create_null(rset_nmem, kc, 0);
    else if (num_result_sets == 1)
        *rset = result_sets[0];
    else
        *rset = rset_create_and(rset_nmem, kc, kc->scope,
                                num_result_sets, result_sets);
    if (!*rset)
        return ZEBRA_FAIL;
    return ZEBRA_OK;
}

static ZEBRA_RES rpn_search_APT_local(ZebraHandle zh,
				      Z_AttributesPlusTerm *zapt,
				      const char *termz,
				      oid_value attributeSet,
				      NMEM stream,
				      const char *rank_type, NMEM rset_nmem,
				      RSET *rset,
				      struct rset_key_control *kc)
{
    RSFD rsfd;
    struct it_key key;
    int sys;
    *rset = rset_create_temp(rset_nmem, kc, kc->scope,
                             res_get (zh->res, "setTmpDir"),0 );
    rsfd = rset_open(*rset, RSETF_WRITE);
    
    sys = atoi(termz);
    if (sys <= 0)
        sys = 1;
    key.mem[0] = sys;
    key.mem[1] = 1;
    key.len = 2;
    rset_write (rsfd, &key);
    rset_close (rsfd);
    return ZEBRA_OK;
}

static ZEBRA_RES rpn_sort_spec(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			       oid_value attributeSet, NMEM stream,
			       Z_SortKeySpecList *sort_sequence,
			       const char *rank_type,
			       NMEM rset_nmem,
			       RSET *rset,
			       struct rset_key_control *kc)
{
    int i;
    int sort_relation_value;
    AttrType sort_relation_type;
    Z_SortKeySpec *sks;
    Z_SortKey *sk;
    int oid[OID_SIZE];
    oident oe;
    char termz[20];
    
    attr_init_APT(&sort_relation_type, zapt, 7);
    sort_relation_value = attr_find(&sort_relation_type, &attributeSet);

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
    sprintf(termz, "%d", i);

    oe.proto = PROTO_Z3950;
    oe.oclass = CLASS_ATTSET;
    oe.value = attributeSet;
    if (!oid_ent_to_oid (&oe, oid))
        return ZEBRA_FAIL;

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
    sk->u.sortAttributes->list = zapt->attributes;

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
    *rset = rset_create_null(rset_nmem, kc, 0);
    return ZEBRA_OK;
}


static int rpn_check_xpath(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
                           oid_value attributeSet,
                           struct xpath_location_step *xpath, int max,
                           NMEM mem)
{
    oid_value curAttributeSet = attributeSet;
    AttrType use;
    const char *use_string = 0;
    
    attr_init_APT(&use, zapt, 1);
    attr_find_ex(&use, &curAttributeSet, &use_string);

    if (!use_string || *use_string != '/')
        return -1;

    return zebra_parse_xpath_str(use_string, xpath, max, mem);
}
 
               

static RSET xpath_trunc(ZebraHandle zh, NMEM stream,
                        int reg_type, const char *term, 
                        const char *xpath_use,
                        NMEM rset_nmem,
			struct rset_key_control *kc)
{
    RSET rset;
    struct grep_info grep_info;
    char term_dict[2048];
    char ord_buf[32];
    int prefix_len = 0;
    int ord = zebraExplain_lookup_attr_str(zh->reg->zei, 
                                           zinfo_index_category_index,
                                           reg_type,
                                           xpath_use);
    int ord_len, i, r, max_pos;
    int term_type = Z_Term_characterString;
    const char *flags = "void";

    if (grep_info_prepare(zh, 0 /* zapt */, &grep_info, '0') == ZEBRA_FAIL)
        return rset_create_null(rset_nmem, kc, 0);
    
    if (ord < 0)
        return rset_create_null(rset_nmem, kc, 0);
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
    strcpy(term_dict+prefix_len, term);
    
    grep_info.isam_p_indx = 0;
    r = dict_lookup_grep(zh->reg->dict, term_dict, 0,
                          &grep_info, &max_pos, 0, grep_handle);
    yaz_log(YLOG_DEBUG, "%s %d positions", term,
             grep_info.isam_p_indx);
    rset = rset_trunc(zh, grep_info.isam_p_buf,
		      grep_info.isam_p_indx, term, strlen(term),
		      flags, 1, term_type,rset_nmem,
		      kc, kc->scope, 0, reg_type, 0 /* hits_limit */,
		      0 /* term_ref_id_str */);
    grep_info_delete(&grep_info);
    return rset;
}

static
ZEBRA_RES rpn_search_xpath(ZebraHandle zh,
			   int num_bases, char **basenames,
			   NMEM stream, const char *rank_type, RSET rset,
			   int xpath_len, struct xpath_location_step *xpath,
			   NMEM rset_nmem,
			   RSET *rset_out,
			   struct rset_key_control *kc)
{
    int base_no;
    int i;
    int always_matches = rset ? 0 : 1;

    if (xpath_len < 0)
    {
	*rset_out = rset;
	return ZEBRA_OK;
    }

    yaz_log(YLOG_DEBUG, "xpath len=%d", xpath_len);
    for (i = 0; i<xpath_len; i++)
    {
        yaz_log(log_level_rpn, "XPATH %d %s", i, xpath[i].part);

    }

    /*
      //a    ->    a/.*
      //a/b  ->    b/a/.*
      /a     ->    a/
      /a/b   ->    b/a/

      /      ->    none

   a[@attr = value]/b[@other = othervalue]

 /e/@a val      range(e/,range(@a,freetext(w,1015,val),@a),e/)
 /a/b val       range(b/a/,freetext(w,1016,val),b/a/)
 /a/b/@c val    range(b/a/,range(@c,freetext(w,1016,val),@c),b/a/)
 /a/b[@c = y] val range(b/a/,freetext(w,1016,val),b/a/,@c = y)
 /a[@c = y]/b val range(a/,range(b/a/,freetext(w,1016,val),b/a/),a/,@c = y)
 /a[@c = x]/b[@c = y] range(a/,range(b/a/,freetext(w,1016,val),b/a/,@c = y),a/,@c = x)
      
    */

    dict_grep_cmap (zh->reg->dict, 0, 0);

    for (base_no = 0; base_no < num_bases; base_no++)
    {
        int level = xpath_len;
        int first_path = 1;
        
        if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
        {
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[base_no]);
	    *rset_out = rset;
	    return ZEBRA_FAIL;
        }
        while (--level >= 0)
        {
            WRBUF xpath_rev = wrbuf_alloc();
            int i;
            RSET rset_start_tag = 0, rset_end_tag = 0, rset_attr = 0;

            for (i = level; i >= 1; --i)
            {
                const char *cp = xpath[i].part;
                if (*cp)
                {
                    for (; *cp; cp++)
                    {
                        if (*cp == '*')
                            wrbuf_puts(xpath_rev, "[^/]*");
                        else if (*cp == ' ')
                            wrbuf_puts(xpath_rev, "\001 ");
                        else
                            wrbuf_putc(xpath_rev, *cp);

                        /* wrbuf_putc does not null-terminate , but
                           wrbuf_puts below ensures it does.. so xpath_rev
                           is OK iff length is > 0 */
                    }
                    wrbuf_puts(xpath_rev, "/");
                }
                else if (i == 1)  /* // case */
                    wrbuf_puts(xpath_rev, ".*");
            }
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
                    zh, stream, '0', wrbuf_buf(wbuf), ZEBRA_XPATH_ATTR_NAME, 
                    rset_nmem, kc);
                wrbuf_free(wbuf, 1);
            } 
            else 
            {
                if (!first_path)
                {
                    wrbuf_free(xpath_rev, 1);
                    continue;
                }
            }
            yaz_log(log_level_rpn, "xpath_rev (%d) = %.*s", level, 
                    wrbuf_len(xpath_rev), wrbuf_buf(xpath_rev));
            if (wrbuf_len(xpath_rev))
            {
                rset_start_tag = xpath_trunc(zh, stream, '0', 
                                             wrbuf_buf(xpath_rev),
                                             ZEBRA_XPATH_ELM_BEGIN, 
                                             rset_nmem, kc);
                if (always_matches)
                    rset = rset_start_tag;
                else
                {
                    rset_end_tag = xpath_trunc(zh, stream, '0', 
                                               wrbuf_buf(xpath_rev),
                                               ZEBRA_XPATH_ELM_END, 
                                               rset_nmem, kc);
                    
                    rset = rset_create_between(rset_nmem, kc, kc->scope,
                                               rset_start_tag, rset,
                                               rset_end_tag, rset_attr);
                }
            }
            wrbuf_free(xpath_rev, 1);
            first_path = 0;
        }
    }
    *rset_out = rset;
    return ZEBRA_OK;
}

#define MAX_XPATH_STEPS 10

static ZEBRA_RES rpn_search_APT(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				oid_value attributeSet, NMEM stream,
				Z_SortKeySpecList *sort_sequence,
				int num_bases, char **basenames, 
				NMEM rset_nmem,
				RSET *rset,
				struct rset_key_control *kc)
{
    ZEBRA_RES res = ZEBRA_OK;
    unsigned reg_id;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    char termz[IT_MAX_WORD+1];
    int xpath_len;
    const char *xpath_use = 0;
    struct xpath_location_step xpath[MAX_XPATH_STEPS];

    if (!log_level_set)
    {
        log_level_rpn = yaz_log_module_level("rpn");
        log_level_set = 1;
    }
    zebra_maps_attr(zh->reg->zebra_maps, zapt, &reg_id, &search_type,
		    rank_type, &complete_flag, &sort_flag);
    
    yaz_log(YLOG_DEBUG, "reg_id=%c", reg_id);
    yaz_log(YLOG_DEBUG, "complete_flag=%d", complete_flag);
    yaz_log(YLOG_DEBUG, "search_type=%s", search_type);
    yaz_log(YLOG_DEBUG, "rank_type=%s", rank_type);

    if (zapt_term_to_utf8(zh, zapt, termz) == ZEBRA_FAIL)
	return ZEBRA_FAIL;

    if (sort_flag)
        return rpn_sort_spec(zh, zapt, attributeSet, stream, sort_sequence,
			     rank_type, rset_nmem, rset, kc);
    /* consider if an X-Path query is used */
    xpath_len = rpn_check_xpath(zh, zapt, attributeSet, 
                                xpath, MAX_XPATH_STEPS, stream);
    if (xpath_len >= 0)
    {
        if (xpath[xpath_len-1].part[0] == '@') 
            xpath_use = ZEBRA_XPATH_ATTR_CDATA;  /* last step is attribute  */
        else
            xpath_use = ZEBRA_XPATH_CDATA;  /* searching for cdata */        

        if (1)
        {
            AttrType relation;
            int relation_value;

            attr_init_APT(&relation, zapt, 2);
            relation_value = attr_find(&relation, NULL);

            if (relation_value == 103) /* alwaysmatches */
            {
                *rset = 0; /* signal no "term" set */
                return rpn_search_xpath(zh, num_bases, basenames,
                                        stream, rank_type, *rset, 
                                        xpath_len, xpath, rset_nmem, rset, kc);
            }
        }
    }

    /* search using one of the various search type strategies
       termz is our UTF-8 search term
       attributeSet is top-level default attribute set 
       stream is ODR for search
       reg_id is the register type
       complete_flag is 1 for complete subfield, 0 for incomplete
       xpath_use is use-attribute to be used for X-Path search, 0 for none
    */
    if (!strcmp(search_type, "phrase"))
    {
        res = rpn_search_APT_phrase(zh, zapt, termz, attributeSet, stream,
				    reg_id, complete_flag, rank_type,
				    xpath_use,
				    num_bases, basenames, rset_nmem,
				    rset, kc);
    }
    else if (!strcmp(search_type, "and-list"))
    {
        res = rpn_search_APT_and_list(zh, zapt, termz, attributeSet, stream,
				      reg_id, complete_flag, rank_type,
				      xpath_use,
				      num_bases, basenames, rset_nmem,
				      rset, kc);
    }
    else if (!strcmp(search_type, "or-list"))
    {
        res = rpn_search_APT_or_list(zh, zapt, termz, attributeSet, stream,
				     reg_id, complete_flag, rank_type,
				     xpath_use,
				     num_bases, basenames, rset_nmem,
				     rset, kc);
    }
    else if (!strcmp(search_type, "local"))
    {
        res = rpn_search_APT_local(zh, zapt, termz, attributeSet, stream,
				   rank_type, rset_nmem, rset, kc);
    }
    else if (!strcmp(search_type, "numeric"))
    {
        res = rpn_search_APT_numeric(zh, zapt, termz, attributeSet, stream,
				     reg_id, complete_flag, rank_type,
				     xpath_use,
				     num_bases, basenames, rset_nmem,
				     rset, kc);
    }
    else
    {
	zebra_setError(zh, YAZ_BIB1_UNSUPP_SEARCH, 0);
	res = ZEBRA_FAIL;
    }
    if (res != ZEBRA_OK)
	return res;
    if (!*rset)
	return ZEBRA_FAIL;
    return rpn_search_xpath(zh, num_bases, basenames,
			    stream, rank_type, *rset, 
			    xpath_len, xpath, rset_nmem, rset, kc);
}

static ZEBRA_RES rpn_search_structure(ZebraHandle zh, Z_RPNStructure *zs,
				      oid_value attributeSet, 
				      NMEM stream, NMEM rset_nmem,
				      Z_SortKeySpecList *sort_sequence,
				      int num_bases, char **basenames,
				      RSET **result_sets, int *num_result_sets,
				      Z_Operator *parent_op,
				      struct rset_key_control *kc);

ZEBRA_RES rpn_search_top(ZebraHandle zh, Z_RPNStructure *zs,
			 oid_value attributeSet, 
			 NMEM stream, NMEM rset_nmem,
			 Z_SortKeySpecList *sort_sequence,
			 int num_bases, char **basenames,
			 RSET *result_set)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    ZEBRA_RES res;
    struct rset_key_control *kc = zebra_key_control_create(zh);

    res = rpn_search_structure(zh, zs, attributeSet,
			       stream, rset_nmem,
			       sort_sequence, 
			       num_bases, basenames,
			       &result_sets, &num_result_sets,
			       0 /* no parent op */,
			       kc);
    if (res != ZEBRA_OK)
    {
	int i;
	for (i = 0; i<num_result_sets; i++)
	    rset_delete(result_sets[i]);
	*result_set = 0;
    }
    else
    {
	assert(num_result_sets == 1);
	assert(result_sets);
	assert(*result_sets);
	*result_set = *result_sets;
    }
    (*kc->dec)(kc);
    return res;
}

ZEBRA_RES rpn_search_structure(ZebraHandle zh, Z_RPNStructure *zs,
			       oid_value attributeSet, 
			       NMEM stream, NMEM rset_nmem,
			       Z_SortKeySpecList *sort_sequence,
			       int num_bases, char **basenames,
			       RSET **result_sets, int *num_result_sets,
			       Z_Operator *parent_op,
			       struct rset_key_control *kc)
{
    *num_result_sets = 0;
    if (zs->which == Z_RPNStructure_complex)
    {
	ZEBRA_RES res;
        Z_Operator *zop = zs->u.complex->roperator;
	RSET *result_sets_l = 0;
	int num_result_sets_l = 0;
	RSET *result_sets_r = 0;
	int num_result_sets_r = 0;

        res = rpn_search_structure(zh, zs->u.complex->s1,
				   attributeSet, stream, rset_nmem,
				   sort_sequence,
				   num_bases, basenames,
				   &result_sets_l, &num_result_sets_l,
				   zop, kc);
	if (res != ZEBRA_OK)
	{
	    int i;
	    for (i = 0; i<num_result_sets_l; i++)
		rset_delete(result_sets_l[i]);
	    return res;
	}
        res = rpn_search_structure(zh, zs->u.complex->s2,
				   attributeSet, stream, rset_nmem,
				   sort_sequence,
				   num_bases, basenames,
				   &result_sets_r, &num_result_sets_r,
				   zop, kc);
	if (res != ZEBRA_OK)
	{
	    int i;
	    for (i = 0; i<num_result_sets_l; i++)
		rset_delete(result_sets_l[i]);
	    for (i = 0; i<num_result_sets_r; i++)
		rset_delete(result_sets_r[i]);
	    return res;
	}

	/* make a new list of result for all children */
	*num_result_sets = num_result_sets_l + num_result_sets_r;
	*result_sets = nmem_malloc(stream, *num_result_sets * 
				   sizeof(**result_sets));
	memcpy(*result_sets, result_sets_l, 
	       num_result_sets_l * sizeof(**result_sets));
	memcpy(*result_sets + num_result_sets_l, result_sets_r, 
	       num_result_sets_r * sizeof(**result_sets));

	if (!parent_op || parent_op->which != zop->which
	    || (zop->which != Z_Operator_and &&
		zop->which != Z_Operator_or))
	{
	    /* parent node different from this one (or non-present) */
	    /* we must combine result sets now */
	    RSET rset;
	    switch (zop->which)
	    {
	    case Z_Operator_and:
		rset = rset_create_and(rset_nmem, kc,
                                       kc->scope,
                                       *num_result_sets, *result_sets);
		break;
	    case Z_Operator_or:
		rset = rset_create_or(rset_nmem, kc,
                                      kc->scope, 0, /* termid */
                                      *num_result_sets, *result_sets);
		break;
	    case Z_Operator_and_not:
		rset = rset_create_not(rset_nmem, kc,
                                       kc->scope,
                                       (*result_sets)[0],
                                       (*result_sets)[1]);
		break;
	    case Z_Operator_prox:
		if (zop->u.prox->which != Z_ProximityOperator_known)
		{
		    zebra_setError(zh, 
				   YAZ_BIB1_UNSUPP_PROX_UNIT_CODE,
				   0);
		    return ZEBRA_FAIL;
		}
		if (*zop->u.prox->u.known != Z_ProxUnit_word)
		{
		    zebra_setError_zint(zh,
					YAZ_BIB1_UNSUPP_PROX_UNIT_CODE,
					*zop->u.prox->u.known);
		    return ZEBRA_FAIL;
		}
		else
		{
		    rset = rset_create_prox(rset_nmem, kc,
                                            kc->scope,
                                            *num_result_sets, *result_sets, 
                                            *zop->u.prox->ordered,
                                            (!zop->u.prox->exclusion ? 
                                             0 : *zop->u.prox->exclusion),
                                            *zop->u.prox->relationType,
                                            *zop->u.prox->distance );
		}
		break;
	    default:
		zebra_setError(zh, YAZ_BIB1_OPERATOR_UNSUPP, 0);
		return ZEBRA_FAIL;
	    }
	    *num_result_sets = 1;
	    *result_sets = nmem_malloc(stream, *num_result_sets * 
				       sizeof(**result_sets));
	    (*result_sets)[0] = rset;
	}
    }
    else if (zs->which == Z_RPNStructure_simple)
    {
	RSET rset;
	ZEBRA_RES res;

        if (zs->u.simple->which == Z_Operand_APT)
        {
            yaz_log(YLOG_DEBUG, "rpn_search_APT");
            res = rpn_search_APT(zh, zs->u.simple->u.attributesPlusTerm,
				 attributeSet, stream, sort_sequence,
				 num_bases, basenames, rset_nmem, &rset,
				 kc);
	    if (res != ZEBRA_OK)
		return res;
        }
        else if (zs->u.simple->which == Z_Operand_resultSetId)
        {
            yaz_log(YLOG_DEBUG, "rpn_search_ref");
            rset = resultSetRef(zh, zs->u.simple->u.resultSetId);
            if (!rset)
            {
		zebra_setError(zh, 
			       YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
			       zs->u.simple->u.resultSetId);
		return ZEBRA_FAIL;
            }
	    rset_dup(rset);
        }
        else
        {
	    zebra_setError(zh, YAZ_BIB1_UNSUPP_SEARCH, 0);
            return ZEBRA_FAIL;
        }
	*num_result_sets = 1;
	*result_sets = nmem_malloc(stream, *num_result_sets * 
				   sizeof(**result_sets));
	(*result_sets)[0] = rset;
    }
    else
    {
	zebra_setError(zh, YAZ_BIB1_UNSUPP_SEARCH, 0);
        return ZEBRA_FAIL;
    }
    return ZEBRA_OK;
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
    struct scan_info *scan_info = (struct scan_info *) client;

    len_prefix = strlen(scan_info->prefix);
    if (memcmp (name, scan_info->prefix, len_prefix))
        return 1;
    if (pos > 0)
	idx = scan_info->after - pos + scan_info->before;
    else
        idx = - pos - 1;

    /* skip special terms.. of no interest */
    if (name[len_prefix] < 4)
        return 1;

    if (idx < 0)
	return 0;
    scan_info->list[idx].term = (char *)
        odr_malloc(scan_info->odr, strlen(name + len_prefix)+1);
    strcpy(scan_info->list[idx].term, name + len_prefix);
    assert (*info == sizeof(ISAM_P));
    memcpy (&scan_info->list[idx].isam_p, info+1, sizeof(ISAM_P));
    return 0;
}

void zebra_term_untrans_iconv(ZebraHandle zh, NMEM stream, int reg_type,
			      char **dst, const char *src)
{
    char term_src[IT_MAX_WORD];
    char term_dst[IT_MAX_WORD];
    
    zebra_term_untrans (zh, reg_type, term_src, src);

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

static void count_set(ZebraHandle zh, RSET rset, zint *count)
{
    zint psysno = 0;
    struct it_key key;
    RSFD rfd;

    yaz_log(YLOG_DEBUG, "count_set");

    rset->hits_limit = zh->approx_limit;

    *count = 0;
    rfd = rset_open(rset, RSETF_READ);
    while (rset_read(rfd, &key,0 /* never mind terms */))
    {
        if (key.mem[0] != psysno)
        {
            psysno = key.mem[0];
	    if (rfd->counted_items >= rset->hits_limit)
		break;
        }
    }
    rset_close (rfd);
    *count = rset->hits_count;
}

#define RPN_MAX_ORDS 32

ZEBRA_RES rpn_scan(ZebraHandle zh, ODR stream, Z_AttributesPlusTerm *zapt,
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
    struct scan_info *scan_info_array;
    ZebraScanEntry *glist;
    int ords[RPN_MAX_ORDS], ord_no = 0;
    int ptr[RPN_MAX_ORDS];

    unsigned index_type;
    char *search_type = NULL;
    char rank_type[128];
    int complete_flag;
    int sort_flag;
    NMEM rset_nmem = NULL; 
    struct rset_key_control *kc = 0;

    *list = 0;
    *is_partial = 0;

    if (attributeset == VAL_NONE)
        attributeset = VAL_BIB1;

    if (!limit_set)
    {
        AttrType termset;
        int termset_value_numeric;
        const char *termset_value_string;
        attr_init_APT(&termset, zapt, 8);
        termset_value_numeric =
            attr_find_ex(&termset, NULL, &termset_value_string);
        if (termset_value_numeric != -1)
        {
            char resname[32];
            const char *termset_name = 0;
            
            if (termset_value_numeric != -2)
            {
                
                sprintf(resname, "%d", termset_value_numeric);
                termset_name = resname;
            }
            else
                termset_name = termset_value_string;
            
            limit_set = resultSetRef (zh, termset_name);
        }
    }
        
    yaz_log(YLOG_DEBUG, "position = %d, num = %d set=%d",
	    pos, num, attributeset);
        
    if (zebra_maps_attr(zh->reg->zebra_maps, zapt, &index_type, &search_type,
			rank_type, &complete_flag, &sort_flag))
    {
        *num_entries = 0;
	zebra_setError(zh, YAZ_BIB1_UNSUPP_ATTRIBUTE_TYPE, 0);
        return ZEBRA_FAIL;
    }
    for (base_no = 0; base_no < num_bases && ord_no < RPN_MAX_ORDS; base_no++)
    {
	int ord;

	if (zebraExplain_curDatabase (zh->reg->zei, basenames[base_no]))
	{
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[base_no]);
	    *num_entries = 0;
	    return ZEBRA_FAIL;
	}
        if (zebra_apt_get_ord(zh, zapt, index_type, 0, attributeset, &ord) 
            != ZEBRA_OK)
            continue;
        ords[ord_no++] = ord;
    }
    if (ord_no == 0)
    {
        *num_entries = 0;
        return ZEBRA_FAIL;
    }
    /* prepare dictionary scanning */
    if (num < 1)
    {
	*num_entries = 0;
	return ZEBRA_OK;
    }
    before = pos-1;
    if (before < 0)
	before = 0;
    after = 1+num-pos;
    if (after < 0)
	after = 0;
    yaz_log(YLOG_DEBUG, "rpn_scan pos=%d num=%d before=%d "
	    "after=%d before+after=%d",
	    pos, num, before, after, before+after);
    scan_info_array = (struct scan_info *)
        odr_malloc(stream, ord_no * sizeof(*scan_info_array));
    for (i = 0; i < ord_no; i++)
    {
        int j, prefix_len = 0;
        int before_tmp = before, after_tmp = after;
        struct scan_info *scan_info = scan_info_array + i;
        struct rpn_char_map_info rcmi;

        rpn_char_map_prepare (zh->reg, index_type, &rcmi);

        scan_info->before = before;
        scan_info->after = after;
        scan_info->odr = stream;

        scan_info->list = (struct scan_info_entry *)
            odr_malloc(stream, (before+after) * sizeof(*scan_info->list));
        for (j = 0; j<before+after; j++)
            scan_info->list[j].term = NULL;

        prefix_len += key_SU_encode (ords[i], termz + prefix_len);
        termz[prefix_len] = 0;
        strcpy(scan_info->prefix, termz);

        if (trans_scan_term(zh, zapt, termz+prefix_len, index_type) == 
            ZEBRA_FAIL)
            return ZEBRA_FAIL;
	
        dict_scan(zh->reg->dict, termz, &before_tmp, &after_tmp,
		  scan_info, scan_handle);
    }
    glist = (ZebraScanEntry *)
        odr_malloc(stream, (before+after)*sizeof(*glist));

    rset_nmem = nmem_create();
    kc = zebra_key_control_create(zh);

    /* consider terms after main term */
    for (i = 0; i < ord_no; i++)
        ptr[i] = before;
    
    *is_partial = 0;
    for (i = 0; i<after; i++)
    {
        int j, j0 = -1;
        const char *mterm = NULL;
        const char *tst;
        RSET rset = 0;
	int lo = i + pos-1; /* offset in result list */

	/* find: j0 is the first of the minimal values */
        for (j = 0; j < ord_no; j++)
        {
            if (ptr[j] < before+after && ptr[j] >= 0 &&
                (tst = scan_info_array[j].list[ptr[j]].term) &&
                (!mterm || strcmp (tst, mterm) < 0))
            {
                j0 = j;
                mterm = tst;
            }
        }
        if (j0 == -1)
            break;  /* no value found, stop */

	/* get result set for first one , but only if it's within bounds */
	if (lo >= 0)
	{
	    /* get result set for first term */
	    zebra_term_untrans_iconv(zh, stream->mem, index_type,
				     &glist[lo].term, mterm);
	    rset = rset_trunc(zh, &scan_info_array[j0].list[ptr[j0]].isam_p, 1,
			      glist[lo].term, strlen(glist[lo].term),
			      NULL, 0, zapt->term->which, rset_nmem, 
			      kc, kc->scope, 0, index_type, 0 /* hits_limit */,
			      0 /* term_ref_id_str */);
	}
	ptr[j0]++; /* move index for this set .. */
	/* get result set for remaining scan terms */
        for (j = j0+1; j<ord_no; j++)
        {
            if (ptr[j] < before+after && ptr[j] >= 0 &&
                (tst = scan_info_array[j].list[ptr[j]].term) &&
                !strcmp (tst, mterm))
            {
		if (lo >= 0)
		{
		    RSET rsets[2];
		    
		    rsets[0] = rset;
		    rsets[1] =
			rset_trunc(
			    zh, &scan_info_array[j].list[ptr[j]].isam_p, 1,
			    glist[lo].term,
			    strlen(glist[lo].term), NULL, 0,
			    zapt->term->which,rset_nmem,
			    kc, kc->scope, 0, index_type, 0 /* hits_limit */,
			    0 /* term_ref_id_str */ );
		    rset = rset_create_or(rset_nmem, kc,
                                          kc->scope, 0 /* termid */,
                                          2, rsets);
		}
                ptr[j]++;
            }
        }
	if (lo >= 0)
	{
	    zint count;
	    /* merge with limit_set if given */
	    if (limit_set)
	    {
		RSET rsets[2];
		rsets[0] = rset;
		rsets[1] = rset_dup(limit_set);
		
		rset = rset_create_and(rset_nmem, kc, kc->scope, 2, rsets);
	    }
	    /* count it */
	    count_set(zh, rset, &count);
	    glist[lo].occurrences = count;
	    rset_delete(rset);
	}
    }
    if (i < after)
    {
	*num_entries -= (after-i);
	*is_partial = 1;
	if (*num_entries < 0)
	{
	    (*kc->dec)(kc);
	    nmem_destroy(rset_nmem);
	    *num_entries = 0;
	    return ZEBRA_OK;
	}
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
	int lo = before-1-i; /* offset in result list */
	zint count;
	
	for (j = 0; j <ord_no; j++)
	{
	    if (ptr[j] < before && ptr[j] >= 0 &&
		(tst = scan_info_array[j].list[before-1-ptr[j]].term) &&
		(!mterm || strcmp (tst, mterm) > 0))
	    {
		j0 = j;
		    mterm = tst;
	    }
	}
	if (j0 == -1)
	    break;
	
	zebra_term_untrans_iconv(zh, stream->mem, index_type,
				 &glist[lo].term, mterm);
	
	rset = rset_trunc
	    (zh, &scan_info_array[j0].list[before-1-ptr[j0]].isam_p, 1,
	     glist[lo].term, strlen(glist[lo].term),
	     NULL, 0, zapt->term->which, rset_nmem,
	     kc, kc->scope, 0, index_type, 0 /* hits_limit */,
	     0 /* term_ref_id_str */);
	
	ptr[j0]++;
	
	for (j = j0+1; j<ord_no; j++)
	{
	    if (ptr[j] < before && ptr[j] >= 0 &&
		(tst = scan_info_array[j].list[before-1-ptr[j]].term) &&
		!strcmp (tst, mterm))
	    {
		RSET rsets[2];
		
		rsets[0] = rset;
		rsets[1] = rset_trunc(
		    zh,
		    &scan_info_array[j].list[before-1-ptr[j]].isam_p, 1,
		    glist[lo].term,
		    strlen(glist[lo].term), NULL, 0,
		    zapt->term->which, rset_nmem,
		    kc, kc->scope, 0, index_type, 0 /* hits_limit */,
		    0 /* term_ref_id_str */);
		rset = rset_create_or(rset_nmem, kc,
                                      kc->scope, 0 /* termid */, 2, rsets);
		
		ptr[j]++;
	    }
	}
        if (limit_set)
	{
	    RSET rsets[2];
	    rsets[0] = rset;
	    rsets[1] = rset_dup(limit_set);
	    
	    rset = rset_create_and(rset_nmem, kc, kc->scope, 2, rsets);
	}
	count_set(zh, rset, &count);
	glist[lo].occurrences = count;
	rset_delete (rset);
    }
    (*kc->dec)(kc);
    nmem_destroy(rset_nmem);
    i = before-i;
    if (i)
    {
        *is_partial = 1;
        *position -= i;
        *num_entries -= i;
	if (*num_entries <= 0)
	{
	    *num_entries = 0;
	    return ZEBRA_OK;
	}
    }
    
    *list = glist + i;               /* list is set to first 'real' entry */
    
    yaz_log(YLOG_DEBUG, "position = %d, num_entries = %d",
	    *position, *num_entries);
    return ZEBRA_OK;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

