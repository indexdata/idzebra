/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dcompact.c,v $
 * Revision 1.5  1999-05-26 07:49:12  adam
 * C++ compilation.
 *
 * Revision 1.4  1999/05/15 14:36:37  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.3  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.2  1999/03/09 16:27:49  adam
 * More work on SDRKit integration.
 *
 * Revision 1.1  1999/03/09 13:07:06  adam
 * Work on dict_compact routine.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include <dict.h>

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
    dict_from = dict_open (bfs, from_name, 0, 0, 0);
    if (!dict_from)
	return -1;
    map = (int *) xmalloc ((dict_from->head.last+1) * sizeof(*map));
    for (i = 0; i <= (int) (dict_from->head.last); i++)
	map[i] = -1;
    dict_to = dict_open (bfs, to_name, 0, 1, 1);
    if (!dict_to)
	return -1;
    map[0] = 0;
    map[1] = dict_from->head.page_size;
    
    for (i = 1; i < (int) (dict_from->head.last); i++)
    {
	void *buf;
	int size;
#if 0
	logf (LOG_LOG, "map[%d] = %d", i, map[i]);
#endif
	dict_bf_readp (dict_from->dbf, i, &buf);
	size = ((DICT_size(buf)+sizeof(short)-1)/sizeof(short) +
		DICT_nodir(buf))*sizeof(short);
	map[i+1] = map[i] + size;
	no_dir += DICT_nodir(buf);
    }
    logf (LOG_LOG, "map[%d] = %d", i, map[i]);
    logf (LOG_LOG, "nodir = %d", no_dir);
    dict_to->head.root = map[1];
    dict_to->head.last = map[i];
    for (i = 1; i< (int) (dict_from->head.last); i++)
    {
	void *old_p, *new_p;
	dict_bf_readp (dict_from->dbf, i, &old_p);

	logf (LOG_LOG, "dict_bf_newp no=%d size=%d", map[i],
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
