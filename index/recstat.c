/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recstat.c,v $
 * Revision 1.1  1995-12-06 12:41:26  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "recindxp.h"

void rec_prstat (void)
{
    Records records = rec_open (0);
    int i;
    int total_bytes = 0;
    
    logf (LOG_LOG,
          "Total records                        %8d",
          records->head.no_records);

    for (i = 0; i< REC_BLOCK_TYPES; i++)
    {
        logf (LOG_LOG, "Record blocks of size %d",
              records->head.block_size[i]);
        logf (LOG_LOG,
          " Used/Total/Bytes used            %d/%d/%d",
              records->head.block_used[i], records->head.block_last[i]-1,
              records->head.block_used[i] * records->head.block_size[i]);
        total_bytes +=
            records->head.block_used[i] * records->head.block_size[i];
    }
    logf (LOG_LOG,
          "Total size of record index in bytes  %8d",
          records->head.total_bytes);
    logf (LOG_LOG,
          "Total size with overhead             %8d", total_bytes);
    rec_close (&records);
}
