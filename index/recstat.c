/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recstat.c,v $
 * Revision 1.7  1999-02-02 14:51:06  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.6  1998/01/12 15:04:08  adam
 * The test option (-s) only uses read-lock (and not write lock).
 *
 * Revision 1.5  1997/09/17 12:19:17  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.4  1997/09/09 13:38:08  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.3  1996/06/04 10:19:00  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.2  1996/05/14  14:04:34  adam
 * In zebraidx, the 'stat' command is improved. Statistics about ISAM/DICT
 * is collected.
 *
 * Revision 1.1  1995/12/06  12:41:26  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "recindxp.h"

void rec_prstat (Records records)
{
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
}
