/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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


/**
 * \file charmap.c
 * \brief character conversions (.chr)
 *  
 * Support module to handle character-conversions into and out of the
 * Zebra dictionary.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned ucs4_t;

#include <charmap.h>

#include <yaz/yaz-util.h>

#define CHR_MAXSTR 1024
#define CHR_MAXEQUIV 32

const unsigned char CHR_FIELD_BEGIN = '^';

const char *CHR_UNKNOWN = "\001";
const char *CHR_SPACE   = "\002";
const char *CHR_CUT     = "\003";
const char *CHR_BASE    = "\005"; /* CHECK CHR_BASE_CHAR as well */

struct chrmaptab_info
{
    chr_t_entry *input;         /* mapping table for input data */
    chr_t_entry *q_input;       /* mapping table for queries */
    unsigned char *output[256]; /* return mapping - for display of registers */
    int base_uppercase;         /* Start of upper-case ordinals */
    NMEM nmem;
};

/*
 * Character map trie node.
 */
struct chr_t_entry
{
    chr_t_entry **children;  /* array of children */
    unsigned char **target;  /* target for this node, if any */
};

/*
 * General argument structure for callback functions (internal use only)
 */
typedef struct chrwork 
{
    chrmaptab map;
    char string[CHR_MAXSTR+1];
} chrwork;

/*
 * Callback for equivalent stuff
 */
typedef struct
{
    NMEM nmem;
    int no_eq;
    char *eq[CHR_MAXEQUIV];
} chr_equiv_work;
/*
 * Add an entry to the character map.
 */
static chr_t_entry *set_map_string(chr_t_entry *root, NMEM nmem,
				   const char *from, int len, char *to,
                                   const char *from_0)
{
    if (!from_0)
        from_0 = from;
    if (!root)
    {
	root = (chr_t_entry *) nmem_malloc(nmem, sizeof(*root));
	root->children = 0;
	root->target = 0;
    }
    if (!len)
    {
	if (!root->target || !root->target[0] || 
	    strcmp((const char *) root->target[0], to))
	{
            if (from_0 && 
                root->target && root->target[0] && root->target[0][0] &&
                strcmp((const char *) root->target[0], CHR_UNKNOWN))
            {
                yaz_log(YLOG_WARN, "duplicate entry for charmap from '%s'",
                        from_0);
            }
	    root->target = (unsigned char **)
		nmem_malloc(nmem, sizeof(*root->target)*2);
	    root->target[0] = (unsigned char *) nmem_strdup(nmem, to);
	    root->target[1] = 0;
	}
    }
    else
    {
	if (!root->children)
	{
	    int i;

	    root->children = (chr_t_entry **)
		nmem_malloc(nmem, sizeof(chr_t_entry*) * 256);
	    for (i = 0; i < 256; i++)
		root->children[i] = 0;
	}
	if (!(root->children[(unsigned char) *from] =
              set_map_string(root->children[(unsigned char) *from], nmem,
                             from + 1, len - 1, to, from_0)))
	    return 0;
    }
    return root;
}

static chr_t_entry *find_entry_x(chr_t_entry *t, const char **from, int *len, int first)
{
    chr_t_entry *res;

    while (*len <= 0)
    {   /* switch to next buffer */
	if (*len < 0)
	    break;
	from++;
	len++;
    }
    if (*len > 0 && t->children)
    {
	const char *old_from = *from;
	int old_len = *len;

	res = 0;

	if (first && t->children[CHR_FIELD_BEGIN])
	{
	    if ((res = find_entry_x(t->children[CHR_FIELD_BEGIN], from, len, 0)) && res != t->children[CHR_FIELD_BEGIN])
		return res;
            else
	        res = 0;
	    /* otherwhise there was no match on beginning of field, move on */
	} 
	
	if (!res && t->children[(unsigned char) **from])
	{
	    (*len)--;
	    (*from)++;
	    if ((res = find_entry_x(t->children[(unsigned char) *old_from],
				    from, len, 0)))
		return res;
	    /* no match */
	    *len = old_len;
	    *from = old_from;
	}
    }
    /* no children match. use ourselves, if we have a target */
    return t->target ? t : 0;
}

const char **chr_map_input_x(chrmaptab maptab, const char **from, int *len, int first)
{
    chr_t_entry *t = maptab->input;
    chr_t_entry *res;

    if (!(res = find_entry_x(t, from, len, first)))
	abort();
    return (const char **) (res->target);
}

const char **chr_map_input(chrmaptab maptab, const char **from, int len, int first)
{
    chr_t_entry *t = maptab->input;
    chr_t_entry *res;
    int len_tmp[2];

    len_tmp[0] = len;
    len_tmp[1] = -1;
    if (!(res = find_entry_x(t, from, len_tmp, first)))
	abort();
    return (const char **) (res->target);
}

const char **chr_map_q_input(chrmaptab maptab,
			     const char **from, int len, int first)
{
    chr_t_entry *t = maptab->q_input;
    chr_t_entry *res;
    int len_tmp[2];
    
    len_tmp[0] = len;
    len_tmp[1] = -1;
    if (!(res = find_entry_x(t, from, len_tmp, first)))
	return 0;
    return (const char **) (res->target);
}

const char *chr_map_output(chrmaptab maptab, const char **from, int len)
{
    unsigned char c = ** (unsigned char **) from;
    const char *out = (const char*) maptab->output[c];

    if (out)
        (*from)++;
    return out;
}

static int zebra_ucs4_strlen(ucs4_t *s)
{
    int i = 0;
    while (*s++)
	i++;
    return i;
}

ucs4_t zebra_prim_w(ucs4_t **s)
{
    ucs4_t c;
    ucs4_t i = 0;
    char fmtstr[8];

    if (**s == '\\' && 1[*s])
    {
	(*s)++;
	c = **s;
	switch (c)
	{
	case '\\': c = '\\'; (*s)++; break;
	case 'r': c = '\r'; (*s)++; break;
	case 'n': c = '\n'; (*s)++; break;
	case 't': c = '\t'; (*s)++; break;
	case 's': c = ' '; (*s)++; break;
	case 'x': 
	    if (zebra_ucs4_strlen(*s) >= 3)
	    {
		fmtstr[0] = (*s)[1];
		fmtstr[1] = (*s)[2];
		fmtstr[2] = 0;
		sscanf(fmtstr, "%x", &i);
		c = i;
		*s += 3;
	    }
	    break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
	    if (zebra_ucs4_strlen(*s) >= 3)
	    {
		fmtstr[0] = (*s)[0];
		fmtstr[1] = (*s)[1];
		fmtstr[2] = (*s)[2];
		fmtstr[3] = 0;
		sscanf(fmtstr, "%o", &i);
		c = i;
		*s += 3;
	    }
            break;
	case 'L':
	    if (zebra_ucs4_strlen(*s) >= 5)
	    {
		fmtstr[0] = (*s)[1];
		fmtstr[1] = (*s)[2];
		fmtstr[2] = (*s)[3];
		fmtstr[3] = (*s)[4];
		fmtstr[4] = 0;
		sscanf(fmtstr, "%x", &i);
		c = i;
		*s += 5;
	    }
	    break;
        default:
            (*s)++;
	}
    }
    else
    {
        c = **s;
        ++(*s);
    }
    yaz_log(YLOG_DEBUG, "out %d", c);
    return c;
}

/*
 * Callback function.
 * Add an entry to the value space.
 */
static void fun_addentry(const char *s, void *data, int num)
{
    chrmaptab tab = (chrmaptab) data;
    char tmp[2];
    
    tmp[0] = num; tmp[1] = '\0';
    tab->input = set_map_string(tab->input, tab->nmem, s, strlen(s), tmp, 0);
    tab->output[num + tab->base_uppercase] =
	(unsigned char *) nmem_strdup(tab->nmem, s);
}

/* 
 * Callback function.
 * Add a space-entry to the value space.
 */
static void fun_addspace(const char *s, void *data, int num)
{
    chrmaptab tab = (chrmaptab) data;
    tab->input = set_map_string(tab->input, tab->nmem, s, strlen(s),
				(char*) CHR_SPACE, 0);
}

/* 
 * Callback function.
 * Add a space-entry to the value space.
 */
static void fun_addcut(const char *s, void *data, int num)
{
    chrmaptab tab = (chrmaptab) data;
    tab->input = set_map_string(tab->input, tab->nmem, s, strlen(s),
				(char*) CHR_CUT, 0);
}

/*
 * Create a string containing the mapped characters provided.
 */
static void fun_mkstring(const char *s, void *data, int num)
{
    chrwork *arg = (chrwork *) data;
    const char **res, *p = s;

    res = chr_map_input(arg->map, &s, strlen(s), 0);
    if (*res == (char*) CHR_UNKNOWN)
	yaz_log(YLOG_WARN, "Map: '%s' has no mapping", p);
    strncat(arg->string, *res, CHR_MAXSTR - strlen(arg->string));
    arg->string[CHR_MAXSTR] = '\0';
}

/*
 * Create an unmodified string (scan_string handler).
 */
static void fun_add_equivalent_string(const char *s, void *data, int num)
{
    chr_equiv_work *arg = (chr_equiv_work *) data;
    
    if (arg->no_eq == CHR_MAXEQUIV)
	return;
    arg->eq[arg->no_eq++] = nmem_strdup(arg->nmem, s);
}

/*
 * Add a map to the string contained in the argument.
 */
static void fun_add_map(const char *s, void *data, int num)
{
    chrwork *arg = (chrwork *) data;

    assert(arg->map->input);
    yaz_log(YLOG_DEBUG, "set map %.*s", (int) strlen(s), s);
    set_map_string(arg->map->input, arg->map->nmem, s, strlen(s), arg->string,
                   0);
    for (s = arg->string; *s; s++)
	yaz_log(YLOG_DEBUG, " %3d", (unsigned char) *s);
}

static int scan_to_utf8(yaz_iconv_t t, ucs4_t *from, size_t inlen,
                        char *outbuf, size_t outbytesleft)
{
    size_t inbytesleft = inlen * sizeof(ucs4_t);
    char *inbuf = (char*) from;
    size_t ret;
   
    if (t == 0)
        *outbuf++ = *from;  /* ISO-8859-1 is OK here */
    else
    {
        ret = yaz_iconv(t, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
        if (ret != (size_t) (-1))
            ret = yaz_iconv(t, 0, 0, &outbuf, &outbytesleft);
        
            
        if (ret == (size_t) (-1))
        {
	    yaz_log(YLOG_LOG, "from: %2X %2X %2X %2X",
		    from[0], from[1], from[2], from[3]);
            yaz_log(YLOG_WARN|YLOG_ERRNO, "bad unicode sequence");
            return -1;
        }
    }
    *outbuf = '\0';
    return 0;
}

static int scan_string(char *s_native,
                       yaz_iconv_t t_unicode, yaz_iconv_t t_utf8,
		       void (*fun)(const char *c, void *data, int num),
		       void *data, int *num)
{
    char str[1024];

    ucs4_t arg[512];
    ucs4_t arg_prim[512];
    ucs4_t *s = arg;
    ucs4_t c, begin, end;
    size_t i;

    if (t_unicode != 0)
    {
        char *outbuf = (char *) arg;
        char *inbuf = s_native;
        size_t outbytesleft = sizeof(arg)-4;
        size_t inbytesleft = strlen(s_native);
        size_t ret;        	
	ret = yaz_iconv(t_unicode, &inbuf, &inbytesleft,
                        &outbuf, &outbytesleft);
        if (ret != (size_t)(-1))
            ret = yaz_iconv(t_unicode, 0, 0, &outbuf, &outbytesleft);
            
        if (ret == (size_t)(-1))
            return -1;
        i = (outbuf - (char*) arg)/sizeof(ucs4_t);
    }
    else
    { 
        for (i = 0; s_native[i]; i++)
            arg[i] = s_native[i] & 255; /* ISO-8859-1 conversion */
    }
    arg[i] = 0;      /* terminate */
    if (s[0] == 0xfeff || s[0] == 0xfeff)  /* skip byte Order Mark */
        s++;
    while (*s)
    {
	switch (*s)
	{
	case '{':
	    s++;
	    begin = zebra_prim_w(&s);
	    if (*s != '-')
	    {
		yaz_log(YLOG_FATAL, "Bad range in char-map");
		return -1;
	    }
	    s++;
	    end = zebra_prim_w(&s);
	    if (end <= begin)
	    {
		yaz_log(YLOG_FATAL, "Bad range in char-map");
		return -1;
	    }
	    s++;
	    for (c = begin; c <= end; c++)
	    {
                if (scan_to_utf8(t_utf8, &c, 1, str, sizeof(str)-1))
                    return -1;
		(*fun)(str, data, num ? (*num)++ : 0);
	    }
	    break;
	case '(':
            ++s;
	    i = 0;
	    while (*s != ')' || s[-1] == '\\')
            {
                if (*s == '\0')
                {
                    yaz_log(YLOG_FATAL, "Missing ) in charmap");
                    return -1;
                }
		arg_prim[i++] = zebra_prim_w(&s);
            }
	    arg_prim[i] = 0;
            if (scan_to_utf8(t_utf8, arg_prim, zebra_ucs4_strlen(arg_prim), str, sizeof(str)-1))
                return -1;
	    (*fun)(str, data, num ? (*num)++ : 0);
	    s++;
	    break;
	default:
	    c = zebra_prim_w(&s);
            if (scan_to_utf8(t_utf8, &c, 1, str, sizeof(str)-1))
                return -1;
	    (*fun)(str, data, num ? (*num)++ : 0);
	}
    }
    return 0;
}

chrmaptab chrmaptab_create(const char *tabpath, const char *name,
			   const char *tabroot)
{
    FILE *f;
    char line[512], *argv[50];
    chrmaptab res;
    int lineno = 0;
    int no_directives = 0;
    int errors = 0;
    int argc, num = (int) *CHR_BASE, i;
    NMEM nmem;
    yaz_iconv_t t_unicode = 0;
    yaz_iconv_t t_utf8 = 0;
    unsigned endian = 31;
    const char *ucs4_native = "UCS-4";

    yaz_log(YLOG_DEBUG, "maptab %s open", name);
    if (!(f = yaz_fopen(tabpath, name, "r", tabroot)))
    {
	yaz_log(YLOG_WARN|YLOG_ERRNO, "%s", name);
	return 0;
    }

    if (*(char*) &endian == 31)      /* little endian? */
        ucs4_native = "UCS-4LE";

    t_utf8 = yaz_iconv_open("UTF-8", ucs4_native);

    nmem = nmem_create();
    res = (chrmaptab) nmem_malloc(nmem, sizeof(*res));
    res->nmem = nmem;
    res->input = (chr_t_entry *) nmem_malloc(res->nmem, sizeof(*res->input));
    res->input->target = (unsigned char **)
	nmem_malloc(res->nmem, sizeof(*res->input->target) * 2);
    res->input->target[0] = (unsigned char*) CHR_UNKNOWN;
    res->input->target[1] = 0;
    res->input->children = (chr_t_entry **)
	nmem_malloc(res->nmem, sizeof(res->input) * 256);
    for (i = 0; i < 256; i++)
    {
	res->input->children[i] = (chr_t_entry *)
	    nmem_malloc(res->nmem, sizeof(*res->input));
	res->input->children[i]->children = 0;
	res->input->children[i]->target = (unsigned char **)
	    nmem_malloc(res->nmem, 2 * sizeof(unsigned char *));
	res->input->children[i]->target[1] = 0;
	res->input->children[i]->target[0] = (unsigned char*) CHR_UNKNOWN;
    }
    res->q_input = (chr_t_entry *)
	nmem_malloc(res->nmem, sizeof(*res->q_input));
    res->q_input->target = 0;
    res->q_input->children = 0;

    for (i = *CHR_BASE; i < 256; i++)
	res->output[i] = 0;
    res->output[(int) *CHR_SPACE] = (unsigned char *) " ";
    res->output[(int) *CHR_UNKNOWN] = (unsigned char*) "@";
    res->base_uppercase = 0;

    while (!errors && (argc = readconf_line(f, &lineno, line, 512, argv, 50)))
    {
        no_directives++;
	if (!yaz_matchstr(argv[0], "lowercase"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_FATAL, "Syntax error in charmap");
		++errors;
	    }
	    if (scan_string(argv[1], t_unicode, t_utf8, fun_addentry,
                            res, &num) < 0)
	    {
		yaz_log(YLOG_FATAL, "Bad value-set specification");
		++errors;
	    }
	    res->base_uppercase = num;
	    res->output[(int) *CHR_SPACE + num] = (unsigned char *) " ";
	    res->output[(int) *CHR_UNKNOWN + num] = (unsigned char*) "@";
	    num = (int) *CHR_BASE;
	}
	else if (!yaz_matchstr(argv[0], "uppercase"))
	{
	    if (!res->base_uppercase)
	    {
		yaz_log(YLOG_FATAL, "Uppercase directive with no lowercase set");
		++errors;
	    }
	    if (argc != 2)
	    {
		yaz_log(YLOG_FATAL, "Missing arg for uppercase directive");
		++errors;
	    }
	    if (scan_string(argv[1], t_unicode, t_utf8, fun_addentry,
                            res, &num) < 0)
	    {
		yaz_log(YLOG_FATAL, "Bad value-set specification");
		++errors;
	    }
	}
	else if (!yaz_matchstr(argv[0], "space"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_FATAL, "Syntax error in charmap for space");
		++errors;
	    }
	    if (scan_string(argv[1], t_unicode, t_utf8,
                            fun_addspace, res, 0) < 0)
	    {
		yaz_log(YLOG_FATAL, "Bad space specification");
		++errors;
	    }
	}
	else if (!yaz_matchstr(argv[0], "cut"))
	{
	    if (argc != 2)
	    {
		yaz_log(YLOG_FATAL, "Syntax error in charmap for cut");
		++errors;
	    }
	    if (scan_string(argv[1], t_unicode, t_utf8,
                            fun_addcut, res, 0) < 0)
	    {
		yaz_log(YLOG_FATAL, "Bad cut specification");
		++errors;
	    }
	}
	else if (!yaz_matchstr(argv[0], "map"))
	{
	    chrwork buf;

	    if (argc != 3)
	    {
		yaz_log(YLOG_FATAL, "charmap directive map requires 2 args");
		++errors;
	    }
	    buf.map = res;
	    buf.string[0] = '\0';
	    if (scan_string(argv[2], t_unicode, t_utf8,
                            fun_mkstring, &buf, 0) < 0)
	    {
		yaz_log(YLOG_FATAL, "Bad map target");
		++errors;
	    }
	    if (scan_string(argv[1], t_unicode, t_utf8,
                            fun_add_map, &buf, 0) < 0)
	    {
		yaz_log(YLOG_FATAL, "Bad map source");
		++errors;
	    }
	}
	else if (!yaz_matchstr(argv[0], "equivalent"))
	{
	    chr_equiv_work w;

	    if (argc != 2)
	    {
		yaz_log(YLOG_FATAL, "equivalent requires 1 argument");
		++errors;
	    }
	    w.nmem = res->nmem;
	    w.no_eq = 0;
	    if (scan_string(argv[1], t_unicode, t_utf8, 
                            fun_add_equivalent_string, &w, 0) < 0)
	    {
		yaz_log(YLOG_FATAL, "equivalent: invalid string");
		++errors;
	    }
	    else if (w.no_eq == 0)
	    {
		yaz_log(YLOG_FATAL, "equivalent: no strings");
		++errors;
	    }
	    else
	    {
		char *result_str;
		int i, slen = 5;

		/* determine length of regular expression */
		for (i = 0; i<w.no_eq; i++)
		    slen += strlen(w.eq[i]) + 1;
		result_str = nmem_malloc(res->nmem, slen + 5);

		/* build the regular expression */
		*result_str = '\0';
		slen = 0;
		for (i = 0; i<w.no_eq; i++)
		{
		    result_str[slen++]  = i ? '|' : '(';
		    strcpy(result_str + slen, w.eq[i]);
		    slen += strlen(w.eq[i]);
		}
		result_str[slen++] = ')';
		result_str[slen] = '\0';

		/* each eq will map to this regular expression */
		for (i = 0; i<w.no_eq; i++)
		{
		    set_map_string(res->q_input, res->nmem,
				   w.eq[i], strlen(w.eq[i]),
				   result_str, 0);
		}
	    }
	}
        else if (!yaz_matchstr(argv[0], "encoding"))
        {
            if (t_unicode != 0)
                yaz_iconv_close(t_unicode);
            t_unicode = yaz_iconv_open(ucs4_native, argv[1]);
        }
	else
	{
	    yaz_log(YLOG_WARN, "Syntax error at '%s' in %s", line, name);
            errors++;
	}
    }   
    yaz_fclose(f);
    if (no_directives == 0)
    {
        yaz_log(YLOG_WARN, "No directives in '%s'", name);
        errors++;
    }
    if (errors)
    {
	chrmaptab_destroy(res);
	res = 0;
    }
    yaz_log(YLOG_DEBUG, "maptab %s num=%d close %d errors", name, num, errors);
    if (t_utf8 != 0)
        yaz_iconv_close(t_utf8);
    if (t_unicode != 0)
        yaz_iconv_close(t_unicode);
    return res;
}

void chrmaptab_destroy(chrmaptab tab)
{
    if (tab)
	nmem_destroy(tab->nmem);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

