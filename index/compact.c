/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: compact.c,v $
 * Revision 1.1  1999-03-09 10:16:35  adam
 * Work on compaction of dictionary/isamc.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "index.h"
#include "recindex.h"

void inv_compact (BFiles bfs)
{
    dict_compact (bfs, FNAME_DICT, "out");
}
