/* $Id: dcompact.c,v 1.13 2005-01-15 19:38:21 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dict-p.h"

static void dict_copy_page(Dict dict, char *to_p, char *from_p, int *map)
{
    int i, slen, no = 0;
    short *from_indxp, *to_indxp;
    char *from_info, *to_info;
    
    from_indxp = (short*) ((char*) from_p+DICT_bsize(from_p));
    to_indxp = (short*) ((char*) to_p+DICT_bsize(to_p));
    to_info = (char*) to_p + DICT_infoffset;
    for (i = DICT_nodir (from_p); --i >= 0; )
    {
        if (*--from_indxp > 0) /* tail string here! */
        {
            /* string (Dict_char *) DICT_EOS terminated */
            /* unsigned char        length of information */
            /* char *               information */

            from_info = (char*) from_p + *from_indxp;
            *--to_indxp = to_info - to_p;
            slen = (dict_strlen((Dict_char*) from_info)+1)*sizeof(Dict_char);
            memcpy (to_info, from_info, slen);
	    from_info += slen;
            to_info += slen;
        }
        else
        {
	    Dict_ptr subptr;
	    Dict_char subchar;
            /* Dict_ptr             subptr */
            /* Dict_char            sub char */
            /* unsigned char        length of information */
            /* char *               information */

            *--to_indxp = -(to_info - to_p);
            from_info = (char*) from_p - *from_indxp;

	    memcpy (&subptr, from_info, sizeof(subptr));
	    subptr = map[subptr];
	    from_info += sizeof(Dict_ptr);
	    memcpy (&subchar, from_info, sizeof(subchar));
	    from_info += sizeof(Dict_char);
	    		    
            memcpy (to_info, &subptr, sizeof(Dict_ptr));
	    to_info += sizeof(Dict_ptr);
	    memcpy (to_info, &subchar, sizeof(Dict_char));
	    to_info += sizeof(Dict_char);
        }
	assert (to_info < (char*) to_indxp);
        slen = *from_info+1;
        memcpy (to_info, from_info, slen);
        to_info += slen;
        ++no;
    }
    DICT_size(to_p) = to_info - to_p;
    DICT_type(to_p) = 0;
    DICT_nodir(to_p) = no;
}

int dict_copy_compact (BFiles bfs, const char *from_name, const char *to_name)
{
    int no_dir = 0;
    Dict dict_from, dict_to;
    int *map, i;
    dict_from = dict_open (bfs, from_name, 0, 0, 0, 4096);
    if (!dict_from)
	return -1;
    map = (int *) xmalloc ((dict_from->head.last+1) * sizeof(*map));
    for (i = 0; i <= (int) (dict_from->head.last); i++)
	map[i] = -1;
    dict_to = dict_open (bfs, to_name, 0, 1, 1, 4096);
    if (!dict_to)
	return -1;
    map[0] = 0;
    map[1] = dict_from->head.page_size;
    
    for (i = 1; i < (int) (dict_from->head.last); i++)
    {
	void *buf;
	int size;
#if 0
	yaz_log (YLOG_LOG, "map[%d] = %d", i, map[i]);
#endif
	dict_bf_readp (dict_from->dbf, i, &buf);
	size = ((DICT_size(buf)+sizeof(short)-1)/sizeof(short) +
		DICT_nodir(buf))*sizeof(short);
	map[i+1] = map[i] + size;
	no_dir += DICT_nodir(buf);
    }
#if 0
    yaz_log (YLOG_LOG, "map[%d] = %d", i, map[i]);
    yaz_log (YLOG_LOG, "nodir = %d", no_dir);
#endif
    dict_to->head.root = map[1];
    dict_to->head.last = map[i];
    for (i = 1; i< (int) (dict_from->head.last); i++)
    {
	void *old_p, *new_p;
	dict_bf_readp (dict_from->dbf, i, &old_p);

	yaz_log (YLOG_LOG, "dict_bf_newp no=%d size=%d", map[i],
	      map[i+1] - map[i]);
        dict_bf_newp (dict_to->dbf, map[i], &new_p, map[i+1] - map[i]);

	DICT_type(new_p) = 0;
	DICT_backptr(new_p) = map[i-1];
	DICT_bsize(new_p) = map[i+1] - map[i];

	dict_copy_page(dict_from, (char*) new_p, (char*) old_p, map);
    }
    dict_close (dict_from);
    dict_close (dict_to);
    return 0;
}
