/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dcompact.c,v $
 * Revision 1.1  1999-03-09 13:07:06  adam
 * Work on dict_compact routine.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <log.h>
#include <dict.h>

int dict_compact (BFiles bfs, const char *from_name, const char *to_name)
{
    int no_dir = 0;
    Dict from, to;
    int *map, i;
    map = xmalloc (100);
    from = dict_open (bfs, from_name, 0, 0);
    if (!from)
	return -1;
    map = xmalloc ((from->head.last+1) * sizeof(*map));
    for (i = 0; i <= from->head.last; i++)
	map[i] = -1;
    to = dict_open (bfs, to_name, 0, 1);
    if (!to)
	return -1;
    map[0] = 0;
    map[1] = DICT_pagesize(from);
    
    for (i = 1; i < from->head.last; i++)
    {
	void *buf;
	logf (LOG_LOG, "map[%d] = %d", i, map[i]);
	dict_bf_readp (from->dbf, i, &buf);
	map[i+1] = map[i] + DICT_size(buf);
	no_dir += DICT_nodir(buf);
    }
    logf (LOG_LOG, "map[%d] = %d", i, map[i]);
    logf (LOG_LOG, "nodir = %d", no_dir);
    dict_close (from);
    dict_close (to);
    return 0;
}
