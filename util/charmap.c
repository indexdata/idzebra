/*
 * Copyright (C) 1996-2002, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: charmap.c,v 1.20 2002-04-04 20:50:37 adam Exp $
 *
 */

/*
 * Support module to handle character-conversions into and out of the
 * Zebra dictionary.
 */

#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <yaz/yaz-util.h>
#include <charmap.h>

#define CHR_MAXSTR 1024
#define CHR_MAXEQUIV 32

const char *CHR_UNKNOWN = "\001";
const char *CHR_SPACE   = "\002";
const char *CHR_BASE    = "\003";

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
	if (!root->target || !root->target[0] || strcmp(root->target[0], to))
	{
            if (from_0 && 
                root->target && root->target[0] && root->target[0][0] &&
                strcmp (root->target[0], CHR_UNKNOWN))
            {
                yaz_log (LOG_WARN, "duplicate entry for charmap from '%s'",
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

static chr_t_entry *find_entry(chr_t_entry *t, const char **from, int len)
{
    chr_t_entry *res;

    if (len && t->children && t->children[(unsigned char) **from])
    {
	const char *pos = *from;

	(*from)++;
	if ((res = find_entry(t->children[(unsigned char) *pos],
	    from, len - 1)))
	    return res;
	/* no match */
	*from = pos;
    }
    /* no children match. use ourselves, if we have a target */
    return t->target ? t : 0;
}

static chr_t_entry *find_entry_x(chr_t_entry *t, const char **from, int *len)
{
    chr_t_entry *res;

    while (*len <= 0)
    {   /* switch to next buffer */
	if (*len < 0)
	    break;
	from++;
	len++;
    }
    if (*len > 0 && t->children && t->children[(unsigned char) **from])
    {
	const char *old_from = *from;
	int old_len = *len;
	
	(*len)--;
	(*from)++;
	if ((res = find_entry_x(t->children[(unsigned char) *old_from],
				from, len)))
	    return res;
	/* no match */
	*len = old_len;
	*from = old_from;
    }
    /* no children match. use ourselves, if we have a target */
    return t->target ? t : 0;
}

const char **chr_map_input_x(chrmaptab maptab, const char **from, int *len)
{
    chr_t_entry *t = maptab->input;
    chr_t_entry *res;

    if (!(res = find_entry_x(t, from, len)))
	abort();
    return (const char **) (res->target);
}

const char **chr_map_input(chrmaptab maptab, const char **from, int len)
{
    chr_t_entry *t = maptab->input;
    chr_t_entry *res;
    int len_tmp[2];

    len_tmp[0] = len;
    len_tmp[1] = -1;
    if (!(res = find_entry_x(t, from, len_tmp)))
	abort();
    return (const char **) (res->target);
}

const char *chr_map_output(chrmaptab maptab, const char **from, int len)
{
    unsigned char c = ** (unsigned char **) from;
    (*from)++;
    return (const char*) maptab->output[c];
}

unsigned char zebra_prim(char **s)
{
    unsigned char c;
    unsigned int i;

    yaz_log (LOG_DEBUG, "prim %.3s", *s);
    if (**s == '\\')
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
	case 'x': sscanf(*s, "x%2x", &i); c = i; *s += 3; break;
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
	    sscanf(*s, "%3o", &i);
            c = i;
            *s += 3;
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
    yaz_log (LOG_DEBUG, "out %d", c);
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
 * Create a string containing the mapped characters provided.
 */
static void fun_mkstring(const char *s, void *data, int num)
{
    chrwork *arg = (chrwork *) data;
    const char **res, *p = s;

    res = chr_map_input(arg->map, &s, strlen(s));
    if (*res == (char*) CHR_UNKNOWN)
	logf(LOG_WARN, "Map: '%s' has no mapping", p);
    strncat(arg->string, *res, CHR_MAXSTR - strlen(arg->string));
    arg->string[CHR_MAXSTR] = '\0';
}

/*
 * Add a map to the string contained in the argument.
 */
static void fun_add_map(const char *s, void *data, int num)
{
    chrwork *arg = (chrwork *) data;

    assert(arg->map->input);
    logf (LOG_DEBUG, "set map %.*s", (int) strlen(s), s);
    set_map_string(arg->map->input, arg->map->nmem, s, strlen(s), arg->string,
                   0);
    for (s = arg->string; *s; s++)
	logf (LOG_DEBUG, " %3d", (unsigned char) *s);
}

/*
 * Add a query map to the string contained in the argument.
 */
static void fun_add_qmap(const char *s, void *data, int num)
{
    chrwork *arg = (chrwork *) data;

    assert(arg->map->q_input);
    logf (LOG_DEBUG, "set qmap %.*s", (int) strlen(s), s);
    set_map_string(arg->map->q_input, arg->map->nmem, s,
		   strlen(s), arg->string, 0);
    for (s = arg->string; *s; s++)
	logf (LOG_DEBUG, " %3d", (unsigned char) *s);
}


static int scan_string(char *s,
		       void (*fun)(const char *c, void *data, int num),
		       void *data, int *num)
{
    unsigned char c, str[1024], begin, end, *p;
    
    while (*s)
    {
	switch (*s)
	{
	case '{':
	    s++;
	    begin = zebra_prim(&s);
	    if (*s != '-')
	    {
		logf(LOG_FATAL, "Bad range in char-map");
		return -1;
	    }
	    s++;
	    end = zebra_prim(&s);
	    if (end <= begin)
	    {
		logf(LOG_FATAL, "Bad range in char-map");
		return -1;
	    }
	    s++;
	    for (c = begin; c <= end; c++)
	    {
		str[0] = c; str[1] = '\0';
		(*fun)((char *) str, data, num ? (*num)++ : 0);
	    }
	    break;
	case '[': s++; abort(); break;
	case '(':
	    p = (unsigned char*) ++s;
		/* Find the end-marker, ignoring escapes */
	    do
	    {
		if (!(p = (unsigned char*) strchr((char*) p, ')')))
		{
		    logf(LOG_FATAL, "Missing ')' in string");
		    return -1;
		}
	    }
	    while (*(p -  1) == '\\');
	    *p = 0;
	    (*fun)(s, data, num ? (*num)++ : 0);
	    s = (char*) p + 1;
	    break;
	default:
	    c = zebra_prim(&s);
	    str[0] = c; str[1] = '\0';
	    (*fun)((char *) str, data, num ? (*num)++ : 0);
	}
    }
    return 0;
}

chrmaptab chrmaptab_create(const char *tabpath, const char *name, int map_only,
                           const char *tabroot)
{
    FILE *f;
    char line[512], *argv[50];
    chrmaptab res;
    int lineno = 0;
    int errors = 0;
    int argc, num = (int) *CHR_BASE, i;
    NMEM nmem;

    logf (LOG_DEBUG, "maptab %s open", name);
    if (!(f = yaz_path_fopen_base(tabpath, name, "r", tabroot)))
    {
	logf(LOG_WARN|LOG_ERRNO, "%s", name);
	return 0;
    }
    nmem = nmem_create ();
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
	    nmem_malloc (res->nmem, 2 * sizeof(unsigned char *));
	res->input->children[i]->target[1] = 0;
	if (map_only)
	{
	    res->input->children[i]->target[0] = (unsigned char *)
		nmem_malloc (res->nmem, 2 * sizeof(unsigned char));
	    res->input->children[i]->target[0][0] = i;
	    res->input->children[i]->target[0][1] = 0;
	}
	else
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
	if (!map_only && !yaz_matchstr(argv[0], "lowercase"))
	{
	    if (argc != 2)
	    {
		logf(LOG_FATAL, "Syntax error in charmap");
		++errors;
	    }
	    if (scan_string(argv[1], fun_addentry, res, &num) < 0)
	    {
		logf(LOG_FATAL, "Bad value-set specification");
		++errors;
	    }
	    res->base_uppercase = num;
	    res->output[(int) *CHR_SPACE + num] = (unsigned char *) " ";
	    res->output[(int) *CHR_UNKNOWN + num] = (unsigned char*) "@";
	    num = (int) *CHR_BASE;
	}
	else if (!map_only && !yaz_matchstr(argv[0], "uppercase"))
	{
	    if (!res->base_uppercase)
	    {
		logf(LOG_FATAL, "Uppercase directive with no lowercase set");
		++errors;
	    }
	    if (argc != 2)
	    {
		logf(LOG_FATAL, "Missing arg for uppercase directive");
		++errors;
	    }
	    if (scan_string(argv[1], fun_addentry, res, &num) < 0)
	    {
		logf(LOG_FATAL, "Bad value-set specification");
		++errors;
	    }
	}
	else if (!map_only && !yaz_matchstr(argv[0], "space"))
	{
	    if (argc != 2)
	    {
		logf(LOG_FATAL, "Syntax error in charmap");
		++errors;
	    }
	    if (scan_string(argv[1], fun_addspace, res, 0) < 0)
	    {
		logf(LOG_FATAL, "Bad space specification");
		++errors;
	    }
	}
	else if (!yaz_matchstr(argv[0], "map"))
	{
	    chrwork buf;

	    if (argc != 3)
	    {
		logf(LOG_FATAL, "charmap directive map requires 2 args");
		++errors;
	    }
	    buf.map = res;
	    buf.string[0] = '\0';
	    if (scan_string(argv[2], fun_mkstring, &buf, 0) < 0)
	    {
		logf(LOG_FATAL, "Bad map target");
		++errors;
	    }
	    if (scan_string(argv[1], fun_add_map, &buf, 0) < 0)
	    {
		logf(LOG_FATAL, "Bad map source");
		++errors;
	    }
	}
	else if (!yaz_matchstr(argv[0], "qmap"))
	{
	    chrwork buf;

	    if (argc != 3)
	    {
		logf(LOG_FATAL, "charmap directive qmap requires 2 args");
		++errors;
	    }
	    buf.map = res;
	    buf.string[0] = '\0';
	    if (scan_string(argv[2], fun_mkstring, &buf, 0) < 0)
	    {
		logf(LOG_FATAL, "Bad qmap target");
		++errors;
	    }
	    if (scan_string(argv[1], fun_add_qmap, &buf, 0) < 0)
	    {
		logf(LOG_FATAL, "Bad qmap source");
		++errors;
	    }
	}
	else
	{
	    logf(LOG_WARN, "Syntax error at '%s' in %s", line, name);
	}
    
    fclose(f);
    if (errors)
    {
	chrmaptab_destroy(res);
	res = 0;
    }
    logf (LOG_DEBUG, "maptab %s close %d errors", name, errors);
    return res;
}

void chrmaptab_destroy(chrmaptab tab)
{
    if (tab)
	nmem_destroy (tab->nmem);
}


