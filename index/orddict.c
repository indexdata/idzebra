
#include <yaz/xmalloc.h>

#include "orddict.h"

struct zebra_ord_dict {
    char *str;
    size_t str_sz;
    Dict dict;
};

Zebra_ord_dict zebra_ord_dict_open(Dict dict)
{
    Zebra_ord_dict zod = xmalloc(sizeof(*zod));
    zod->str_sz = 50;
    zod->str = xmalloc(zod->str_sz);
    zod->dict = dict;
    return zod;
}

void zebra_ord_dict_close(Zebra_ord_dict zod)
{
    if (!zod)
	return;
    dict_close(zod->dict);
    xfree(zod->str);
    xfree(zod);
}

char *zebra_ord_dict_lookup (Zebra_ord_dict zod, int ord, 
			     const char *p)
{
    return dict_lookup(zod->dict, p);
}

