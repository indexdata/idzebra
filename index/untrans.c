/* This file is part of the Zebra server.
   Copyright (C) Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <yaz/diagbib1.h>
#include "index.h"
#include <charmap.h>

int zebra_term_untrans(ZebraHandle zh, const char *index_type,
                       char *dst, const char *src)
{
    zebra_map_t zm = zebra_map_get(zh->reg->zebra_maps, index_type);
    if (!zm)
    {
        return -2;
    }
    if (zebra_maps_is_icu(zm))
    {
        return -1;
    }
    else
    {
        int len = 0;
        while (*src)
        {
            const char *cp = zebra_maps_output(zm, &src);
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
    return 0;
}

void zebra_term_untrans_iconv2(ZebraHandle zh, NMEM stream,
                               char **dst, const char *src)
{
    char term_dst[IT_MAX_WORD];

    if (zh->iconv_from_utf8 != 0)
    {
        int len;
        char *inbuf = (char *) src;
        size_t inleft = strlen(src);
        char *outbuf = term_dst;
        size_t outleft = sizeof(term_dst)-1;
        size_t ret;

        ret = yaz_iconv (zh->iconv_from_utf8, &inbuf, &inleft,
                         &outbuf, &outleft);
        if (ret == (size_t)(-1))
            len = 0;
        else
        {
            yaz_iconv (zh->iconv_from_utf8, 0, 0, &outbuf, &outleft);
            len = outbuf - term_dst;
        }
        *dst = nmem_malloc(stream, len + 1);
        if (len > 0)
            memcpy (*dst, term_dst, len);
        (*dst)[len] = '\0';
    }
    else
        *dst = nmem_strdup(stream, src);
}

int zebra_term_untrans_iconv(ZebraHandle zh, NMEM stream,
                             const char *index_type,
                             char **dst, const char *src)
{
    int r;
    char term_src[IT_MAX_WORD];

    r = zebra_term_untrans(zh, index_type, term_src, src);
    if (r == 0)
        zebra_term_untrans_iconv2(zh, stream, dst, term_src);
    return r;
}



/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

