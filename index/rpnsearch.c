/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

static int log_level_set = 0;
static int log_level_rpn = 0;

#define TERMSET_DISABLE 1

static const char **rpn_char_map_handler(void *vp, const char **from, int len)
{
    struct rpn_char_map_info *p = (struct rpn_char_map_info *) vp;
    const char **out = zebra_maps_input(p->zm, from, len, 0);
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

void rpn_char_map_prepare(struct zebra_register *reg, zebra_map_t zm,
                          struct rpn_char_map_info *map_info)
{
    map_info->zm = zm;
    if (zebra_maps_is_icu(zm))
        dict_grep_cmap(reg->dict, 0, 0);
    else
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
    int trunc_max;
    ZebraHandle zh;
    const char *index_type;
    ZebraSet termset;
};        

static int add_isam_p(const char *name, const char *info,
                      struct grep_info *p)
{
    if (!log_level_set)
    {
        log_level_rpn = yaz_log_module_level("rpn");
        log_level_set = 1;
    }
    /* we may have to stop this madness.. NOTE: -1 so that if
       truncmax == trunxlimit we do *not* generate result sets */
    if (p->isam_p_indx >= p->trunc_max - 1)
        return 1;

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
        int len = key_SU_decode(&ord, (const unsigned char *) name);
        
        zebra_term_untrans (p->zh, p->index_type, term_tmp, name+len);
        yaz_log(log_level_rpn, "grep: %d %c %s", ord, name[len], term_tmp);
        zebraExplain_lookup_ord(p->zh->reg->zei,
                                ord, 0 /* index_type */, &db, &index_name);
        yaz_log(log_level_rpn, "grep:  db=%s index=%s", db, index_name);
        
        resultSetAddTerm(p->zh, p->termset, name[len], db,
			 index_name, term_tmp);
    }
    (p->isam_p_indx)++;
    return 0;
}

static int grep_handle(char *name, const char *info, void *p)
{
    return add_isam_p(name, info, (struct grep_info *) p);
}

static int term_pre(zebra_map_t zm, const char **src,
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
        map = zebra_maps_input(zm, &s1, strlen(s1), first);
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

#define REGEX_CHARS " ^[]()|.*+?!\"$"

static void add_non_space(const char *start, const char *end,
                          WRBUF term_dict,
                          WRBUF display_term,
                          const char **map, int q_map_match)
{
    size_t sz = end - start;

    wrbuf_write(display_term, start, sz);
    if (!q_map_match)
    {
        while (start < end)
        {
            if (strchr(REGEX_CHARS, *start))
                wrbuf_putc(term_dict, '\\');
            wrbuf_putc(term_dict, *start);
            start++;
        }
    }
    else
    {
        char tmpbuf[80];
        esc_str(tmpbuf, sizeof(tmpbuf), map[0], strlen(map[0]));
        
        wrbuf_puts(term_dict, map[0]);
    }
}


static int term_100_icu(zebra_map_t zm,
                        const char **src, WRBUF term_dict, int space_split,
                        WRBUF display_term,
                        int right_trunc)
{
    int i;
    const char *res_buf = 0;
    size_t res_len = 0;
    const char *display_buf;
    size_t display_len;
    if (!zebra_map_tokenize_next(zm, &res_buf, &res_len,
                                 &display_buf, &display_len))
    {
        *src += strlen(*src);
        return 0;
    }
    wrbuf_write(display_term, display_buf, display_len);
    if (right_trunc)
    {
        /* ICU sort keys seem to be of the form
           basechars \x01 accents \x01 length
           For now we'll just right truncate from basechars . This 
           may give false hits due to accents not being used.
        */
        i = res_len;
        while (--i >= 0 && res_buf[i] != '\x01')
            ;
        if (i > 0)
        {
            while (--i >= 0 && res_buf[i] != '\x01')
                ;
        }
        if (i == 0)
        {  /* did not find base chars at all. Throw error */
            return -1;
        }
        res_len = i; /* reduce res_len */
    }
    for (i = 0; i < res_len; i++)
    {
        if (strchr(REGEX_CHARS "\\", res_buf[i]))
            wrbuf_putc(term_dict, '\\');
        if (res_buf[i] < 32)
            wrbuf_putc(term_dict, 1);
            
        wrbuf_putc(term_dict, res_buf[i]);
    }
    if (right_trunc)
        wrbuf_puts(term_dict, ".*");
    return 1;
}

/* term_100: handle term, where trunc = none(no operators at all) */
static int term_100(zebra_map_t zm,
		    const char **src, WRBUF term_dict, int space_split,
		    WRBUF display_term)
{
    const char *s0;
    const char **map;
    int i = 0;

    const char *space_start = 0;
    const char *space_end = 0;

    if (!term_pre(zm, src, NULL, NULL, !space_split))
        return 0;
    s0 = *src;
    while (*s0)
    {
        const char *s1 = s0;
	int q_map_match = 0;
        map = zebra_maps_search(zm, &s0, strlen(s0), &q_map_match);
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
                        wrbuf_putc(term_dict, '\\');
                    wrbuf_putc(display_term, *space_start);
                    wrbuf_putc(term_dict, *space_start);
                    space_start++;
                               
                }
                /* and reset */
                space_start = space_end = 0;
            }
        }
        i++;

        add_non_space(s1, s0, term_dict, display_term, map, q_map_match);
    }
    *src = s0;
    return i;
}

/* term_101: handle term, where trunc = Process # */
static int term_101(zebra_map_t zm,
		    const char **src, WRBUF term_dict, int space_split,
		    WRBUF display_term)
{
    const char *s0;
    const char **map;
    int i = 0;

    if (!term_pre(zm, src, "#", "#", !space_split))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '#')
        {
            i++;
            wrbuf_puts(term_dict, ".*");
            wrbuf_putc(display_term, *s0);
            s0++;
        }
        else
        {
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zm, &s0, strlen(s0), &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

            i++;
            add_non_space(s1, s0, term_dict, display_term, map, q_map_match);
        }
    }
    *src = s0;
    return i;
}

/* term_103: handle term, where trunc = re-2 (regular expressions) */
static int term_103(zebra_map_t zm, const char **src,
		    WRBUF term_dict, int *errors, int space_split,
		    WRBUF display_term)
{
    int i = 0;
    const char *s0;
    const char **map;

    if (!term_pre(zm, src, "^\\()[].*+?|", "(", !space_split))
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
            wrbuf_putc(display_term, *s0);
            wrbuf_putc(term_dict, *s0);
            s0++;
            i++;
        }
        else
        {
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zm, &s0, strlen(s0),  &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

            i++;
            add_non_space(s1, s0, term_dict, display_term, map, q_map_match);
        }
    }
    *src = s0;
    
    return i;
}

/* term_103: handle term, where trunc = re-1 (regular expressions) */
static int term_102(zebra_map_t zm, const char **src,
		    WRBUF term_dict, int space_split, WRBUF display_term)
{
    return term_103(zm, src, term_dict, NULL, space_split, display_term);
}


/* term_104: handle term, process # and ! */
static int term_104(zebra_map_t zm, const char **src, 
                    WRBUF term_dict, int space_split, WRBUF display_term)
{
    const char *s0;
    const char **map;
    int i = 0;

    if (!term_pre(zm, src, "?*#", "?*#", !space_split))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '?')
        {
            i++;
            wrbuf_putc(display_term, *s0);
            s0++;
            if (*s0 >= '0' && *s0 <= '9')
            {
                int limit = 0;
                while (*s0 >= '0' && *s0 <= '9')
                {
                    limit = limit * 10 + (*s0 - '0');
                    wrbuf_putc(display_term, *s0);
                    s0++;
                }
                if (limit > 20)
                    limit = 20;
                while (--limit >= 0)
                {
                    wrbuf_puts(term_dict, ".?");
                }
            }
            else
            {
                wrbuf_puts(term_dict, ".*");
            }
        }
        else if (*s0 == '*')
        {
            i++;
            wrbuf_puts(term_dict, ".*");
            wrbuf_putc(display_term, *s0);
            s0++;
        }
        else if (*s0 == '#')
        {
            i++;
            wrbuf_puts(term_dict, ".");
            wrbuf_putc(display_term, *s0);
            s0++;
        }
	else
        {
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zm, &s0, strlen(s0), &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

            i++;
            add_non_space(s1, s0, term_dict, display_term, map, q_map_match);
        }
    }
    *src = s0;
    return i;
}

/* term_105/106: handle term, where trunc = Process * and ! and right trunc */
static int term_105(zebra_map_t zm, const char **src, 
                    WRBUF term_dict, int space_split,
		    WRBUF display_term, int right_truncate)
{
    const char *s0;
    const char **map;
    int i = 0;

    if (!term_pre(zm, src, "*!", "*!", !space_split))
        return 0;
    s0 = *src;
    while (*s0)
    {
        if (*s0 == '*')
        {
            i++;
            wrbuf_puts(term_dict, ".*");
            wrbuf_putc(display_term, *s0);
            s0++;
        }
        else if (*s0 == '!')
        {
            i++;
            wrbuf_putc(term_dict, '.');
            wrbuf_putc(display_term, *s0);
            s0++;
        }
	else
        {
	    const char *s1 = s0;
	    int q_map_match = 0;
	    map = zebra_maps_search(zm, &s0, strlen(s0), &q_map_match);
            if (space_split && **map == *CHR_SPACE)
                break;

            i++;
            add_non_space(s1, s0, term_dict, display_term, map, q_map_match);
        }
    }
    if (right_truncate)
        wrbuf_puts(term_dict, ".*");
    *src = s0;
    return i;
}


/* gen_regular_rel - generate regular expression from relation
 *  val:     border value (inclusive)
 *  islt:    1 if <=; 0 if >=.
 */
static void gen_regular_rel(WRBUF term_dict, int val, int islt)
{
    char dst_buf[20*5*20]; /* assuming enough for expansion */
    char *dst = dst_buf;
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
    wrbuf_puts(term_dict, dst);
}

void string_rel_add_char(WRBUF term_p, WRBUF wsrc, int *indx)
{
    const char *src = wrbuf_cstr(wsrc);
    if (src[*indx] == '\\')
    {
        wrbuf_putc(term_p, src[*indx]);
        (*indx)++;
    }
    wrbuf_putc(term_p, src[*indx]);
    (*indx)++;
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
			   const char **term_sub, WRBUF term_dict,
			   const Odr_oid *attributeSet,
			   zebra_map_t zm, int space_split, 
                           WRBUF display_term,
			   int *error_code)
{
    AttrType relation;
    int relation_value;
    int i;
    WRBUF term_component = wrbuf_alloc();

    attr_init_APT(&relation, zapt, 2);
    relation_value = attr_find(&relation, NULL);

    *error_code = 0;
    yaz_log(YLOG_DEBUG, "string relation value=%d", relation_value);
    switch (relation_value)
    {
    case 1:
        if (!term_100(zm, term_sub, term_component, space_split, display_term))
        {
            wrbuf_destroy(term_component);
            return 0;
        }
        yaz_log(log_level_rpn, "Relation <");
        
        wrbuf_putc(term_dict, '(');
        for (i = 0; i < wrbuf_len(term_component); )
        {
            int j = 0;
            
            if (i)
                wrbuf_putc(term_dict, '|');
            while (j < i)
                string_rel_add_char(term_dict, term_component, &j);

            wrbuf_putc(term_dict, '[');

            wrbuf_putc(term_dict, '^');
            
            wrbuf_putc(term_dict, 1);
            wrbuf_putc(term_dict, FIRST_IN_FIELD_CHAR);
            
            string_rel_add_char(term_dict, term_component, &i);
            wrbuf_putc(term_dict, '-');
            
            wrbuf_putc(term_dict, ']');
            wrbuf_putc(term_dict, '.');
            wrbuf_putc(term_dict, '*');
        }
        wrbuf_putc(term_dict, ')');
        break;
    case 2:
        if (!term_100(zm, term_sub, term_component, space_split, display_term))
        {
            wrbuf_destroy(term_component);
            return 0;
        }
        yaz_log(log_level_rpn, "Relation <=");

        wrbuf_putc(term_dict, '(');
        for (i = 0; i < wrbuf_len(term_component); )
        {
            int j = 0;

            while (j < i)
                string_rel_add_char(term_dict, term_component, &j);
            wrbuf_putc(term_dict, '[');

            wrbuf_putc(term_dict, '^');

            wrbuf_putc(term_dict, 1);
            wrbuf_putc(term_dict, FIRST_IN_FIELD_CHAR);

            string_rel_add_char(term_dict, term_component, &i);
            wrbuf_putc(term_dict, '-');

            wrbuf_putc(term_dict, ']');
            wrbuf_putc(term_dict, '.');
            wrbuf_putc(term_dict, '*');

            wrbuf_putc(term_dict, '|');
        }
        for (i = 0; i < wrbuf_len(term_component); )
            string_rel_add_char(term_dict, term_component, &i);
        wrbuf_putc(term_dict, ')');
        break;
    case 5:
        if (!term_100(zm, term_sub, term_component, space_split, display_term))
        {
            wrbuf_destroy(term_component);
            return 0;
        }
        yaz_log(log_level_rpn, "Relation >");

        wrbuf_putc(term_dict, '(');
        for (i = 0; i < wrbuf_len(term_component); )
        {
            int j = 0;

            while (j < i)
                string_rel_add_char(term_dict, term_component, &j);
            wrbuf_putc(term_dict, '[');
            
            wrbuf_putc(term_dict, '^');
            wrbuf_putc(term_dict, '-');
            string_rel_add_char(term_dict, term_component, &i);

            wrbuf_putc(term_dict, ']');
            wrbuf_putc(term_dict, '.');
            wrbuf_putc(term_dict, '*');

            wrbuf_putc(term_dict, '|');
        }
        for (i = 0; i < wrbuf_len(term_component); )
            string_rel_add_char(term_dict, term_component, &i);
        wrbuf_putc(term_dict, '.');
        wrbuf_putc(term_dict, '+');
        wrbuf_putc(term_dict, ')');
        break;
    case 4:
        if (!term_100(zm, term_sub, term_component, space_split, display_term))
        {
            wrbuf_destroy(term_component);
            return 0;
        }
        yaz_log(log_level_rpn, "Relation >=");

        wrbuf_putc(term_dict, '(');
        for (i = 0; i < wrbuf_len(term_component); )
        {
            int j = 0;

            if (i)
                wrbuf_putc(term_dict, '|');
            while (j < i)
                string_rel_add_char(term_dict, term_component, &j);
            wrbuf_putc(term_dict, '[');

            if (i < wrbuf_len(term_component)-1)
            {
                wrbuf_putc(term_dict, '^');
                wrbuf_putc(term_dict, '-');
                string_rel_add_char(term_dict, term_component, &i);
            }
            else
            {
                string_rel_add_char(term_dict, term_component, &i);
                wrbuf_putc(term_dict, '-');
            }
            wrbuf_putc(term_dict, ']');
            wrbuf_putc(term_dict, '.');
            wrbuf_putc(term_dict, '*');
        }
        wrbuf_putc(term_dict, ')');
        break;
    case 3:
    case 102:
    case -1:
        if (!**term_sub)
            return 1;
        yaz_log(log_level_rpn, "Relation =");
        if (!term_100(zm, term_sub, term_component, space_split, display_term))
        {
            wrbuf_destroy(term_component);
            return 0;
        }
        wrbuf_puts(term_dict, "(");
        wrbuf_puts(term_dict, wrbuf_cstr(term_component));
        wrbuf_puts(term_dict, ")");
	break;
    case 103:
        yaz_log(log_level_rpn, "Relation always matches");
        /* skip to end of term (we don't care what it is) */
        while (**term_sub != '\0')
            (*term_sub)++;
        break;
    default:
	*error_code = YAZ_BIB1_UNSUPP_RELATION_ATTRIBUTE;
        wrbuf_destroy(term_component);
	return 0;
    }
    wrbuf_destroy(term_component);
    return 1;
}

static ZEBRA_RES string_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			     const char **term_sub, 
                             WRBUF term_dict,
			     const Odr_oid *attributeSet, NMEM stream,
			     struct grep_info *grep_info,
			     const char *index_type, int complete_flag,
			     WRBUF display_term,
                             const char *xpath_use,
			     struct ord_list **ol,
                             zebra_map_t zm);

ZEBRA_RES zebra_term_limits_APT(ZebraHandle zh,
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

/** \brief search for term (which may be truncated)
 */
static ZEBRA_RES search_term(ZebraHandle zh,
                             Z_AttributesPlusTerm *zapt,
                             const char **term_sub, 
                             const Odr_oid *attributeSet, NMEM stream,
                             struct grep_info *grep_info,
                             const char *index_type, int complete_flag,
                             const char *rank_type, 
                             const char *xpath_use,
                             NMEM rset_nmem,
                             RSET *rset,
                             struct rset_key_control *kc,
                             zebra_map_t zm)
{
    ZEBRA_RES res;
    struct ord_list *ol;
    zint hits_limit_value;
    const char *term_ref_id_str = 0;
    WRBUF term_dict = wrbuf_alloc();
    WRBUF display_term = wrbuf_alloc();
    *rset = 0;
    zebra_term_limits_APT(zh, zapt, &hits_limit_value, &term_ref_id_str,
                          stream);
    grep_info->isam_p_indx = 0;
    res = string_term(zh, zapt, term_sub, term_dict,
                      attributeSet, stream, grep_info,
		      index_type, complete_flag,
		      display_term, xpath_use, &ol, zm);
    wrbuf_destroy(term_dict);
    if (res == ZEBRA_OK && *term_sub)
    {
        yaz_log(log_level_rpn, "term: %s", wrbuf_cstr(display_term));
        *rset = rset_trunc(zh, grep_info->isam_p_buf,
                           grep_info->isam_p_indx, wrbuf_buf(display_term),
                           wrbuf_len(display_term), rank_type, 
                           1 /* preserve pos */,
                           zapt->term->which, rset_nmem,
                           kc, kc->scope, ol, index_type, hits_limit_value,
                           term_ref_id_str);
        if (!*rset)
            res = ZEBRA_FAIL;
    }
    wrbuf_destroy(display_term);
    return res;
}

static ZEBRA_RES string_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			     const char **term_sub, 
                             WRBUF term_dict,
			     const Odr_oid *attributeSet, NMEM stream,
			     struct grep_info *grep_info,
			     const char *index_type, int complete_flag,
			     WRBUF display_term,
                             const char *xpath_use,
			     struct ord_list **ol,
                             zebra_map_t zm)
{
    int r;
    AttrType truncation;
    int truncation_value;
    const char *termp;
    struct rpn_char_map_info rcmi;

    int space_split = complete_flag ? 0 : 1;
    int ord = -1;
    int regex_range = 0;
    int max_pos, prefix_len = 0;
    int relation_error;
    char ord_buf[32];
    int ord_len, i;

    *ol = ord_list_create(stream);

    rpn_char_map_prepare(zh->reg, zm, &rcmi);
    attr_init_APT(&truncation, zapt, 5);
    truncation_value = attr_find(&truncation, NULL);
    yaz_log(log_level_rpn, "truncation value %d", truncation_value);

    termp = *term_sub; /* start of term for each database */
    
    if (zebra_apt_get_ord(zh, zapt, index_type, xpath_use,
                          attributeSet, &ord) != ZEBRA_OK)
    {
        *term_sub = 0;
        return ZEBRA_FAIL;
    }
    
    wrbuf_rewind(term_dict); /* new dictionary regexp term */
    
    *ol = ord_list_append(stream, *ol, ord);
    ord_len = key_SU_encode(ord, ord_buf);
    
    wrbuf_putc(term_dict, '(');
    
    for (i = 0; i<ord_len; i++)
    {
        wrbuf_putc(term_dict, 1);  /* our internal regexp escape char */
        wrbuf_putc(term_dict, ord_buf[i]);
    }
    wrbuf_putc(term_dict, ')');
    
    prefix_len = wrbuf_len(term_dict);

    if (zebra_maps_is_icu(zm))
    {
        int relation_value;
        AttrType relation;
        
        attr_init_APT(&relation, zapt, 2);
        relation_value = attr_find(&relation, NULL);
        if (relation_value == 103) /* always matches */
            termp += strlen(termp); /* move to end of term */
        else if (relation_value == 3 || relation_value == 102 || relation_value == -1)
        {
            /* ICU case */
            switch (truncation_value)
            {
            case -1:         /* not specified */
            case 100:        /* do not truncate */
                if (!term_100_icu(zm, &termp, term_dict, space_split, display_term, 0))
                {
                    *term_sub = 0;
                    return ZEBRA_OK;
                }
                break;
            case 1:          /* right truncation */
                if (!term_100_icu(zm, &termp, term_dict, space_split, display_term, 1))
                {
                    *term_sub = 0;
                    return ZEBRA_OK;
                }
                break;
            default:
                zebra_setError_zint(zh,
                                    YAZ_BIB1_UNSUPP_TRUNCATION_ATTRIBUTE,
                                    truncation_value);
                return ZEBRA_FAIL;
            }
        }
        else
        {
            zebra_setError_zint(zh,
                                YAZ_BIB1_UNSUPP_RELATION_ATTRIBUTE,
                                relation_value);
            return ZEBRA_FAIL;
        }
    }
    else
    {
        /* non-ICU case. using string.chr and friends */
        switch (truncation_value)
        {
        case -1:         /* not specified */
        case 100:        /* do not truncate */
            if (!string_relation(zh, zapt, &termp, term_dict,
                                 attributeSet,
                                 zm, space_split, display_term,
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
            wrbuf_putc(term_dict, '(');
            if (!term_100(zm, &termp, term_dict, space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_puts(term_dict, ".*)");
            break;
        case 2:          /* left truncation */
            wrbuf_puts(term_dict, "(.*");
            if (!term_100(zm, &termp, term_dict, space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_putc(term_dict, ')');
            break;
        case 3:          /* left&right truncation */
            wrbuf_puts(term_dict, "(.*");
            if (!term_100(zm, &termp, term_dict, space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_puts(term_dict, ".*)");
            break;
        case 101:        /* process # in term */
            wrbuf_putc(term_dict, '(');
            if (!term_101(zm, &termp, term_dict, space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_puts(term_dict, ")");
            break;
        case 102:        /* Regexp-1 */
            wrbuf_putc(term_dict, '(');
            if (!term_102(zm, &termp, term_dict, space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_putc(term_dict, ')');
            break;
        case 103:       /* Regexp-2 */
            regex_range = 1;
            wrbuf_putc(term_dict, '(');
            if (!term_103(zm, &termp, term_dict, &regex_range,
                          space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_putc(term_dict, ')');
            break;
        case 104:        /* process # and ! in term */
            wrbuf_putc(term_dict, '(');
            if (!term_104(zm, &termp, term_dict, space_split, display_term))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_putc(term_dict, ')');
            break;
        case 105:        /* process * and ! in term */
            wrbuf_putc(term_dict, '(');
            if (!term_105(zm, &termp, term_dict, space_split, display_term, 1))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_putc(term_dict, ')');
            break;
        case 106:        /* process * and ! in term */
            wrbuf_putc(term_dict, '(');
            if (!term_105(zm, &termp, term_dict, space_split, display_term, 0))
            {
                *term_sub = 0;
                return ZEBRA_OK;
            }
            wrbuf_putc(term_dict, ')');
            break;
        default:
            zebra_setError_zint(zh,
                                YAZ_BIB1_UNSUPP_TRUNCATION_ATTRIBUTE,
                                truncation_value);
            return ZEBRA_FAIL;
        }
    }
    if (1)
    {
        char buf[1000];
        const char *input = wrbuf_cstr(term_dict) + prefix_len;
        esc_str(buf, sizeof(buf), input, strlen(input));
    }
    {
        WRBUF pr_wr = wrbuf_alloc();

        wrbuf_write_escaped(pr_wr, wrbuf_buf(term_dict), wrbuf_len(term_dict));
        yaz_log(YLOG_LOG, "dict_lookup_grep: %s", wrbuf_cstr(pr_wr));
        wrbuf_destroy(pr_wr);
    }
    r = dict_lookup_grep(zh->reg->dict, wrbuf_cstr(term_dict), regex_range,
                         grep_info, &max_pos, 
                         ord_len /* number of "exact" chars */,
                         grep_handle);
    if (r == 1)
        zebra_set_partial_result(zh);
    else if (r)
        yaz_log(YLOG_WARN, "dict_lookup_grep fail %d", r);
    *term_sub = termp;
    yaz_log(YLOG_DEBUG, "%d positions", grep_info->isam_p_indx);
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
				   const char *index_type)
{
#ifdef TERM_COUNT
    grep_info->term_no = 0;
#endif
    grep_info->trunc_max = atoi(res_get_def(zh->res, "truncmax", "10000"));
    grep_info->isam_p_size = 0;
    grep_info->isam_p_buf = NULL;
    grep_info->zh = zh;
    grep_info->index_type = index_type;
    grep_info->termset = 0;
    if (zapt)
    {
        AttrType truncmax;
        int truncmax_value;

        attr_init_APT(&truncmax, zapt, 13);
        truncmax_value = attr_find(&truncmax, NULL);
        if (truncmax_value != -1)
            grep_info->trunc_max = truncmax_value;
    }
    if (zapt)
    {
        AttrType termset;
        int termset_value_numeric;
        const char *termset_value_string;

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
    }
    return ZEBRA_OK;
}

static ZEBRA_RES search_terms_chrmap(ZebraHandle zh,
                                     Z_AttributesPlusTerm *zapt,
                                     const char *termz,
                                     const Odr_oid *attributeSet,
                                     NMEM stream,
                                     const char *index_type, int complete_flag,
                                     const char *rank_type,
                                     const char *xpath_use,
                                     NMEM rset_nmem,
                                     RSET **result_sets, int *num_result_sets,
                                     struct rset_key_control *kc,
                                     zebra_map_t zm)
{
    struct grep_info grep_info;
    const char *termp = termz;
    int alloc_sets = 0;
    
    *num_result_sets = 0;
    if (grep_info_prepare(zh, zapt, &grep_info, index_type) == ZEBRA_FAIL)
        return ZEBRA_FAIL;
    while (1)
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
        res = search_term(zh, zapt, &termp, attributeSet,
                          stream, &grep_info,
                          index_type, complete_flag,
                          rank_type,
                          xpath_use, rset_nmem,
                          &(*result_sets)[*num_result_sets],
                          kc, zm);
	if (res != ZEBRA_OK)
	{
	    int i;
	    for (i = 0; i < *num_result_sets; i++)
		rset_delete((*result_sets)[i]);
	    grep_info_delete(&grep_info);
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
                               
/**
   \brief Create result set(s) for list of terms
   \param zh Zebra Handle
   \param zapt Attributes Plust Term (RPN leaf)
   \param termz term as used in query but converted to UTF-8
   \param attributeSet default attribute set
   \param stream memory for result
   \param index_type register type ("w", "p",..)
   \param complete_flag whether it's phrases or not
   \param rank_type term flags for ranking
   \param xpath_use use attribute for X-Path (-1 for no X-path)
   \param rset_nmem memory for result sets
   \param result_sets output result set for each term in list (output)
   \param num_result_sets number of output result sets
   \param kc rset key control to be used for created result sets
*/
static ZEBRA_RES search_terms_list(ZebraHandle zh,
                                   Z_AttributesPlusTerm *zapt,
                                   const char *termz,
                                   const Odr_oid *attributeSet,
                                   NMEM stream,
                                   const char *index_type, int complete_flag,
                                   const char *rank_type,
                                   const char *xpath_use,
                                   NMEM rset_nmem,
                                   RSET **result_sets, int *num_result_sets,
                                   struct rset_key_control *kc)
{
    zebra_map_t zm = zebra_map_get_or_add(zh->reg->zebra_maps, index_type);
    if (zebra_maps_is_icu(zm))
        zebra_map_tokenize_start(zm, termz, strlen(termz));
    return search_terms_chrmap(zh, zapt, termz, attributeSet,
                               stream, index_type, complete_flag,
                               rank_type, xpath_use,
                               rset_nmem, result_sets, num_result_sets,
                               kc, zm);
}


/** \brief limit a search by position - returns result set
 */
static ZEBRA_RES search_position(ZebraHandle zh,
                                 Z_AttributesPlusTerm *zapt,
                                 const Odr_oid *attributeSet,
                                 const char *index_type,
                                 NMEM rset_nmem,
                                 RSET *rset,
                                 struct rset_key_control *kc)
{
    int position_value;
    AttrType position;
    int ord = -1;
    char ord_buf[32];
    char term_dict[100];
    int ord_len;
    char *val;
    ISAM_P isam_p;
    zebra_map_t zm = zebra_map_get_or_add(zh->reg->zebra_maps, index_type);
    
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


    if (!zebra_maps_is_first_in_field(zm))
    {
        zebra_setError_zint(zh, YAZ_BIB1_UNSUPP_POSITION_ATTRIBUTE,
                            position_value);
        return ZEBRA_FAIL;
    }

    if (zebra_apt_get_ord(zh, zapt, index_type, 0,
                          attributeSet, &ord) != ZEBRA_OK)
    {
        return ZEBRA_FAIL;
    }
    ord_len = key_SU_encode(ord, ord_buf);
    memcpy(term_dict, ord_buf, ord_len);
    strcpy(term_dict+ord_len, FIRST_IN_FIELD_STR);
    val = dict_lookup(zh->reg->dict, term_dict);
    if (val)
    {
        assert(*val == sizeof(ISAM_P));
        memcpy(&isam_p, val+1, sizeof(isam_p));

        *rset = zebra_create_rset_isam(zh, rset_nmem, kc, kc->scope, 
                                       isam_p, 0);
    }
    return ZEBRA_OK;
}

/** \brief returns result set for phrase search
 */
static ZEBRA_RES rpn_search_APT_phrase(ZebraHandle zh,
				       Z_AttributesPlusTerm *zapt,
				       const char *termz_org,
				       const Odr_oid *attributeSet,
				       NMEM stream,
				       const char *index_type,
                                       int complete_flag,
				       const char *rank_type,
                                       const char *xpath_use,
				       NMEM rset_nmem,
				       RSET *rset,
				       struct rset_key_control *kc)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    ZEBRA_RES res =
	search_terms_list(zh, zapt, termz_org, attributeSet,
                          stream, index_type, complete_flag,
                          rank_type, xpath_use,
                          rset_nmem,
                          &result_sets, &num_result_sets, kc);
    
    if (res != ZEBRA_OK)
	return res;

    if (num_result_sets > 0)
    {
        RSET first_set = 0;
        res = search_position(zh, zapt, attributeSet, 
                              index_type,
                              rset_nmem, &first_set,
                              kc);
        if (res != ZEBRA_OK)
        {
            int i;
            for (i = 0; i<num_result_sets; i++)
                rset_delete(result_sets[i]);
            return res;
        }
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

/** \brief returns result set for or-list search
 */
static ZEBRA_RES rpn_search_APT_or_list(ZebraHandle zh,
					Z_AttributesPlusTerm *zapt,
					const char *termz_org,
					const Odr_oid *attributeSet,
					NMEM stream,
					const char *index_type, 
                                        int complete_flag,
					const char *rank_type,
                                        const char *xpath_use,
					NMEM rset_nmem,
					RSET *rset,
					struct rset_key_control *kc)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    int i;
    ZEBRA_RES res =
	search_terms_list(zh, zapt, termz_org, attributeSet,
                          stream, index_type, complete_flag,
                          rank_type, xpath_use,
                          rset_nmem,
                          &result_sets, &num_result_sets, kc);
    if (res != ZEBRA_OK)
	return res;

    for (i = 0; i<num_result_sets; i++)
    {
        RSET first_set = 0;
        res = search_position(zh, zapt, attributeSet, 
                              index_type,
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

/** \brief returns result set for and-list search
 */
static ZEBRA_RES rpn_search_APT_and_list(ZebraHandle zh,
					 Z_AttributesPlusTerm *zapt,
					 const char *termz_org,
					 const Odr_oid *attributeSet,
					 NMEM stream,
					 const char *index_type, 
                                         int complete_flag,
					 const char *rank_type, 
                                         const char *xpath_use,
					 NMEM rset_nmem,
					 RSET *rset,
					 struct rset_key_control *kc)
{
    RSET *result_sets = 0;
    int num_result_sets = 0;
    int i;
    ZEBRA_RES res =
	search_terms_list(zh, zapt, termz_org, attributeSet,
                          stream, index_type, complete_flag,
                          rank_type, xpath_use,
                          rset_nmem,
                          &result_sets, &num_result_sets,
                          kc);
    if (res != ZEBRA_OK)
	return res;
    for (i = 0; i<num_result_sets; i++)
    {
        RSET first_set = 0;
        res = search_position(zh, zapt, attributeSet, 
                              index_type,
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
			    WRBUF term_dict,
			    const Odr_oid *attributeSet,
			    struct grep_info *grep_info,
			    int *max_pos,
			    zebra_map_t zm,
			    WRBUF display_term,
			    int *error_code)
{
    AttrType relation;
    int relation_value;
    int term_value;
    int r;
    WRBUF term_num = wrbuf_alloc();

    *error_code = 0;
    attr_init_APT(&relation, zapt, 2);
    relation_value = attr_find(&relation, NULL);

    yaz_log(log_level_rpn, "numeric relation value=%d", relation_value);

    switch (relation_value)
    {
    case 1:
        yaz_log(log_level_rpn, "Relation <");
        if (!term_100(zm, term_sub, term_num, 1, display_term))
        { 
            wrbuf_destroy(term_num);
            return 0;
        }
        term_value = atoi(wrbuf_cstr(term_num));
        gen_regular_rel(term_dict, term_value-1, 1);
        break;
    case 2:
        yaz_log(log_level_rpn, "Relation <=");
        if (!term_100(zm, term_sub, term_num, 1, display_term))
        {
            wrbuf_destroy(term_num);
            return 0;
        }
        term_value = atoi(wrbuf_cstr(term_num));
        gen_regular_rel(term_dict, term_value, 1);
        break;
    case 4:
        yaz_log(log_level_rpn, "Relation >=");
        if (!term_100(zm, term_sub, term_num, 1, display_term))
        {
            wrbuf_destroy(term_num);
            return 0;
        }
        term_value = atoi(wrbuf_cstr(term_num));
        gen_regular_rel(term_dict, term_value, 0);
        break;
    case 5:
        yaz_log(log_level_rpn, "Relation >");
        if (!term_100(zm, term_sub, term_num, 1, display_term))
        {
            wrbuf_destroy(term_num);
            return 0;
        }
        term_value = atoi(wrbuf_cstr(term_num));
        gen_regular_rel(term_dict, term_value+1, 0);
        break;
    case -1:
    case 3:
        yaz_log(log_level_rpn, "Relation =");
        if (!term_100(zm, term_sub, term_num, 1, display_term))
        {
            wrbuf_destroy(term_num);
            return 0; 
        }
        term_value = atoi(wrbuf_cstr(term_num));
        wrbuf_printf(term_dict, "(0*%d)", term_value);
	break;
    case 103:
        /* term_tmp untouched.. */
        while (**term_sub != '\0')
            (*term_sub)++;
        break;
    default:
	*error_code = YAZ_BIB1_UNSUPP_RELATION_ATTRIBUTE;
	wrbuf_destroy(term_num); 
        return 0;
    }
    r = dict_lookup_grep(zh->reg->dict, wrbuf_cstr(term_dict), 
                         0, grep_info, max_pos, 0, grep_handle);

    if (r == 1)
        zebra_set_partial_result(zh);
    else if (r)
        yaz_log(YLOG_WARN, "dict_lookup_grep fail, rel = gt: %d", r);
    yaz_log(log_level_rpn, "%d positions", grep_info->isam_p_indx);
    wrbuf_destroy(term_num);
    return 1;
}

static ZEBRA_RES numeric_term(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			      const char **term_sub, 
                              WRBUF term_dict,
			      const Odr_oid *attributeSet, NMEM stream,
			      struct grep_info *grep_info,
			      const char *index_type, int complete_flag,
			      WRBUF display_term,
                              const char *xpath_use,
                              struct ord_list **ol)
{
    const char *termp;
    struct rpn_char_map_info rcmi;
    int max_pos;
    int relation_error = 0;
    int ord, ord_len, i;
    char ord_buf[32];
    zebra_map_t zm = zebra_map_get_or_add(zh->reg->zebra_maps, index_type);
    
    *ol = ord_list_create(stream);

    rpn_char_map_prepare(zh->reg, zm, &rcmi);

    termp = *term_sub;
    
    if (zebra_apt_get_ord(zh, zapt, index_type, xpath_use,
                          attributeSet, &ord) != ZEBRA_OK)
    {
        return ZEBRA_FAIL;
    }
    
    wrbuf_rewind(term_dict);
    
    *ol = ord_list_append(stream, *ol, ord);
    
    ord_len = key_SU_encode(ord, ord_buf);
    
    wrbuf_putc(term_dict, '(');
    for (i = 0; i < ord_len; i++)
    {
        wrbuf_putc(term_dict, 1);
        wrbuf_putc(term_dict, ord_buf[i]);
    }
    wrbuf_putc(term_dict, ')');
    
    if (!numeric_relation(zh, zapt, &termp, term_dict,
                          attributeSet, grep_info, &max_pos, zm,
                          display_term, &relation_error))
    {
        if (relation_error)
        {
            zebra_setError(zh, relation_error, 0);
            return ZEBRA_FAIL;
        }
        *term_sub = 0;
        return ZEBRA_OK;
    }
    *term_sub = termp;
    yaz_log(YLOG_DEBUG, "%d positions", grep_info->isam_p_indx);
    return ZEBRA_OK;
}

				 
static ZEBRA_RES rpn_search_APT_numeric(ZebraHandle zh,
					Z_AttributesPlusTerm *zapt,
					const char *termz,
					const Odr_oid *attributeSet,
					NMEM stream,
					const char *index_type, 
                                        int complete_flag,
					const char *rank_type, 
                                        const char *xpath_use,
					NMEM rset_nmem,
					RSET *rset,
					struct rset_key_control *kc)
{
    const char *termp = termz;
    RSET *result_sets = 0;
    int num_result_sets = 0;
    ZEBRA_RES res;
    struct grep_info grep_info;
    int alloc_sets = 0;
    zint hits_limit_value;
    const char *term_ref_id_str = 0;

    zebra_term_limits_APT(zh, zapt, &hits_limit_value, &term_ref_id_str,
                          stream);

    yaz_log(log_level_rpn, "APT_numeric t='%s'", termz);
    if (grep_info_prepare(zh, zapt, &grep_info, index_type) == ZEBRA_FAIL)
        return ZEBRA_FAIL;
    while (1)
    { 
        struct ord_list *ol;
        WRBUF term_dict = wrbuf_alloc();
        WRBUF display_term = wrbuf_alloc();
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
        res = numeric_term(zh, zapt, &termp, term_dict,
                           attributeSet, stream, &grep_info,
			   index_type, complete_flag,
			   display_term, xpath_use, &ol);
        wrbuf_destroy(term_dict);
	if (res == ZEBRA_FAIL || termp == 0)
        {
            wrbuf_destroy(display_term);
	    break;
        }
        yaz_log(YLOG_DEBUG, "term: %s", wrbuf_cstr(display_term));
        result_sets[num_result_sets] =
	    rset_trunc(zh, grep_info.isam_p_buf,
		       grep_info.isam_p_indx, wrbuf_buf(display_term),
		       wrbuf_len(display_term), rank_type,
		       0 /* preserve position */,
		       zapt->term->which, rset_nmem, 
		       kc, kc->scope, ol, index_type,
		       hits_limit_value,
		       term_ref_id_str);
        wrbuf_destroy(display_term);
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
				      const Odr_oid *attributeSet,
				      NMEM stream,
				      const char *rank_type, NMEM rset_nmem,
				      RSET *rset,
				      struct rset_key_control *kc)
{
    Record rec;
    zint sysno = atozint(termz);
    
    if (sysno <= 0)
        sysno = 0;
    rec = rec_get(zh->reg->records, sysno);
    if (!rec)
        sysno = 0;

    rec_free(&rec);

    if (sysno <= 0)
    {
        *rset = rset_create_null(rset_nmem, kc, 0);
    }
    else
    {
        RSFD rsfd;
        struct it_key key;
        *rset = rset_create_temp(rset_nmem, kc, kc->scope,
                                 res_get(zh->res, "setTmpDir"), 0);
        rsfd = rset_open(*rset, RSETF_WRITE);
        
        key.mem[0] = sysno;
        key.mem[1] = 1;
        key.len = 2;
        rset_write(rsfd, &key);
        rset_close(rsfd);
    }
    return ZEBRA_OK;
}

static ZEBRA_RES rpn_sort_spec(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
			       const Odr_oid *attributeSet, NMEM stream,
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
        i = atoi_n((char *) zapt->term->u.general->buf,
                   zapt->term->u.general->len);
    if (i >= sort_sequence->num_specs)
        i = 0;
    sprintf(termz, "%d", i);

    sks = (Z_SortKeySpec *) nmem_malloc(stream, sizeof(*sks));
    sks->sortElement = (Z_SortElement *)
        nmem_malloc(stream, sizeof(*sks->sortElement));
    sks->sortElement->which = Z_SortElement_generic;
    sk = sks->sortElement->u.generic = (Z_SortKey *)
        nmem_malloc(stream, sizeof(*sk));
    sk->which = Z_SortKey_sortAttributes;
    sk->u.sortAttributes = (Z_SortAttributes *)
        nmem_malloc(stream, sizeof(*sk->u.sortAttributes));

    sk->u.sortAttributes->id = odr_oiddup_nmem(stream, attributeSet);
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
                           const Odr_oid *attributeSet,
                           struct xpath_location_step *xpath, int max,
                           NMEM mem)
{
    const Odr_oid *curAttributeSet = attributeSet;
    AttrType use;
    const char *use_string = 0;
    
    attr_init_APT(&use, zapt, 1);
    attr_find_ex(&use, &curAttributeSet, &use_string);

    if (!use_string || *use_string != '/')
        return -1;

    return zebra_parse_xpath_str(use_string, xpath, max, mem);
}
 
               

static RSET xpath_trunc(ZebraHandle zh, NMEM stream,
                        const char *index_type, const char *term, 
                        const char *xpath_use,
                        NMEM rset_nmem,
			struct rset_key_control *kc)
{
    struct grep_info grep_info;
    int ord = zebraExplain_lookup_attr_str(zh->reg->zei, 
                                           zinfo_index_category_index,
                                           index_type, xpath_use);
    if (grep_info_prepare(zh, 0 /* zapt */, &grep_info, "0") == ZEBRA_FAIL)
        return rset_create_null(rset_nmem, kc, 0);
    
    if (ord < 0)
        return rset_create_null(rset_nmem, kc, 0);
    else
    {
        int i, r, max_pos;
        char ord_buf[32];
        RSET rset;
        WRBUF term_dict = wrbuf_alloc();
        int ord_len = key_SU_encode(ord, ord_buf);
        int term_type = Z_Term_characterString;
        const char *flags = "void";

        wrbuf_putc(term_dict, '(');
        for (i = 0; i<ord_len; i++)
        {
            wrbuf_putc(term_dict, 1);
            wrbuf_putc(term_dict, ord_buf[i]);
        }
        wrbuf_putc(term_dict, ')');
        wrbuf_puts(term_dict, term);
        
        grep_info.isam_p_indx = 0;
        r = dict_lookup_grep(zh->reg->dict, wrbuf_cstr(term_dict), 0,
                             &grep_info, &max_pos, 0, grep_handle);
        yaz_log(YLOG_DEBUG, "%s %d positions", term,
                grep_info.isam_p_indx);
        rset = rset_trunc(zh, grep_info.isam_p_buf,
                          grep_info.isam_p_indx, term, strlen(term),
                          flags, 1, term_type, rset_nmem,
                          kc, kc->scope, 0, index_type, 0 /* hits_limit */,
                          0 /* term_ref_id_str */);
        grep_info_delete(&grep_info);
        wrbuf_destroy(term_dict);
        return rset;
    }
}

static
ZEBRA_RES rpn_search_xpath(ZebraHandle zh,
			   NMEM stream, const char *rank_type, RSET rset,
			   int xpath_len, struct xpath_location_step *xpath,
			   NMEM rset_nmem,
			   RSET *rset_out,
			   struct rset_key_control *kc)
{
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

    dict_grep_cmap(zh->reg->dict, 0, 0);
    
    {
        int level = xpath_len;
        int first_path = 1;
        
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
                rset_attr = xpath_trunc(
                    zh, stream, "0", wrbuf_cstr(wbuf), 
                    ZEBRA_XPATH_ATTR_NAME, 
                    rset_nmem, kc);
                wrbuf_destroy(wbuf);
            } 
            else 
            {
                if (!first_path)
                {
                    wrbuf_destroy(xpath_rev);
                    continue;
                }
            }
            yaz_log(log_level_rpn, "xpath_rev (%d) = %s", level, 
                    wrbuf_cstr(xpath_rev));
            if (wrbuf_len(xpath_rev))
            {
                rset_start_tag = xpath_trunc(zh, stream, "0", 
                                             wrbuf_cstr(xpath_rev),
                                             ZEBRA_XPATH_ELM_BEGIN, 
                                             rset_nmem, kc);
                if (always_matches)
                    rset = rset_start_tag;
                else
                {
                    rset_end_tag = xpath_trunc(zh, stream, "0", 
                                               wrbuf_cstr(xpath_rev),
                                               ZEBRA_XPATH_ELM_END, 
                                               rset_nmem, kc);
                    
                    rset = rset_create_between(rset_nmem, kc, kc->scope,
                                               rset_start_tag, rset,
                                               rset_end_tag, rset_attr);
                }
            }
            wrbuf_destroy(xpath_rev);
            first_path = 0;
        }
    }
    *rset_out = rset;
    return ZEBRA_OK;
}

#define MAX_XPATH_STEPS 10

static ZEBRA_RES rpn_search_database(ZebraHandle zh, 
                                     Z_AttributesPlusTerm *zapt,
                                     const Odr_oid *attributeSet, NMEM stream,
                                     Z_SortKeySpecList *sort_sequence,
                                     NMEM rset_nmem,
                                     RSET *rset,
                                     struct rset_key_control *kc);

static ZEBRA_RES rpn_search_APT(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
				const Odr_oid *attributeSet, NMEM stream,
				Z_SortKeySpecList *sort_sequence,
				int num_bases, const char **basenames, 
				NMEM rset_nmem,
				RSET *rset,
				struct rset_key_control *kc)
{
    RSET *rsets = nmem_malloc(stream, num_bases * sizeof(*rsets));
    ZEBRA_RES res = ZEBRA_OK;
    int i;
    for (i = 0; i < num_bases; i++)
    {

        if (zebraExplain_curDatabase(zh->reg->zei, basenames[i]))
        {
	    zebra_setError(zh, YAZ_BIB1_DATABASE_UNAVAILABLE,
			   basenames[i]);
            res = ZEBRA_FAIL;
            break;
        }
        res = rpn_search_database(zh, zapt, attributeSet, stream,
                                  sort_sequence,
                                  rset_nmem, rsets+i, kc);
        if (res != ZEBRA_OK)
            break;
    }
    if (res != ZEBRA_OK)
    {   /* must clean up the already created sets */
        while (--i >= 0)
            rset_delete(rsets[i]);
        *rset = 0;
    }
    else 
    {
        if (num_bases == 1)
            *rset = rsets[0];
        else if (num_bases == 0)
            *rset = rset_create_null(rset_nmem, kc, 0); 
        else
            *rset = rset_create_or(rset_nmem, kc, kc->scope, 0 /* TERMID */,
                                   num_bases, rsets);
    }
    return res;
}

static ZEBRA_RES rpn_search_database(ZebraHandle zh, 
                                     Z_AttributesPlusTerm *zapt,
                                     const Odr_oid *attributeSet, NMEM stream,
                                     Z_SortKeySpecList *sort_sequence,
                                     NMEM rset_nmem,
                                     RSET *rset,
                                     struct rset_key_control *kc)
{
    ZEBRA_RES res = ZEBRA_OK;
    const char *index_type;
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
    zebra_maps_attr(zh->reg->zebra_maps, zapt, &index_type, &search_type,
		    rank_type, &complete_flag, &sort_flag);
    
    yaz_log(YLOG_DEBUG, "index_type=%s", index_type);
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
                return rpn_search_xpath(zh, stream, rank_type, *rset, 
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
				    index_type, complete_flag, rank_type,
				    xpath_use,
				    rset_nmem,
				    rset, kc);
    }
    else if (!strcmp(search_type, "and-list"))
    {
        res = rpn_search_APT_and_list(zh, zapt, termz, attributeSet, stream,
				      index_type, complete_flag, rank_type,
				      xpath_use,
				      rset_nmem,
				      rset, kc);
    }
    else if (!strcmp(search_type, "or-list"))
    {
        res = rpn_search_APT_or_list(zh, zapt, termz, attributeSet, stream,
				     index_type, complete_flag, rank_type,
				     xpath_use,
                                     rset_nmem,
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
				     index_type, complete_flag, rank_type,
				     xpath_use,
				     rset_nmem,
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
    return rpn_search_xpath(zh, stream, rank_type, *rset, 
			    xpath_len, xpath, rset_nmem, rset, kc);
}

static ZEBRA_RES rpn_search_structure(ZebraHandle zh, Z_RPNStructure *zs,
				      const Odr_oid *attributeSet, 
				      NMEM stream, NMEM rset_nmem,
				      Z_SortKeySpecList *sort_sequence,
				      int num_bases, const char **basenames,
				      RSET **result_sets, int *num_result_sets,
				      Z_Operator *parent_op,
				      struct rset_key_control *kc);

ZEBRA_RES rpn_get_top_approx_limit(ZebraHandle zh, Z_RPNStructure *zs,
                                   zint *approx_limit)
{
    ZEBRA_RES res = ZEBRA_OK;
    if (zs->which == Z_RPNStructure_complex)
    {
        if (res == ZEBRA_OK)
            res = rpn_get_top_approx_limit(zh, zs->u.complex->s1,
                                           approx_limit);
        if (res == ZEBRA_OK)
            res = rpn_get_top_approx_limit(zh, zs->u.complex->s2,
                                           approx_limit);
    }
    else if (zs->which == Z_RPNStructure_simple)
    {
        if (zs->u.simple->which == Z_Operand_APT)
        {
            Z_AttributesPlusTerm *zapt = zs->u.simple->u.attributesPlusTerm;
            AttrType global_hits_limit_attr;
            int l;
            
            attr_init_APT(&global_hits_limit_attr, zapt, 12);
            
            l = attr_find(&global_hits_limit_attr, NULL);
            if (l != -1)
                *approx_limit = l;
        }
    }
    return res;
}

ZEBRA_RES rpn_search_top(ZebraHandle zh, Z_RPNStructure *zs,
			 const Odr_oid *attributeSet, 
			 NMEM stream, NMEM rset_nmem,
			 Z_SortKeySpecList *sort_sequence,
			 int num_bases, const char **basenames,
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
			       const Odr_oid *attributeSet, 
			       NMEM stream, NMEM rset_nmem,
			       Z_SortKeySpecList *sort_sequence,
			       int num_bases, const char **basenames,
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



/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

