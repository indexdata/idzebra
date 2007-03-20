/* $Id: untrans.c,v 1.3 2007-03-20 22:07:35 adam Exp $
   Copyright (C) 1995-2007
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
#include <ctype.h>

#include <yaz/diagbib1.h>
#include "index.h"
#include <charmap.h>

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
        *dst = nmem_strdup(stream, term_src);
}



/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
