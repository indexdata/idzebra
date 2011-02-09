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

#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <yaz/diagbib1.h>
#include "index.h"
#include <charmap.h>

/* convert APT search term to UTF8 */
ZEBRA_RES zapt_term_to_utf8(ZebraHandle zh, Z_AttributesPlusTerm *zapt,
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
            yaz_iconv(zh->iconv_to_utf8, 0, 0, &outbuf, &outleft);
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
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

