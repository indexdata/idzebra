/* $Id: recstat.c,v 1.10 2004-08-06 12:55:01 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
   Index Data Aps

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
    zint total_bytes = 0;
    
    logf (LOG_LOG,
          "Total records                        %8" ZINT_FORMAT0,
          records->head.no_records);

    for (i = 0; i< REC_BLOCK_TYPES; i++)
    {
        logf (LOG_LOG, "Record blocks of size %d",
              records->head.block_size[i]);
        logf (LOG_LOG,
          " Used/Total/Bytes used            "
	      ZINT_FORMAT "/" ZINT_FORMAT "/" ZINT_FORMAT,
              records->head.block_used[i], records->head.block_last[i]-1,
              records->head.block_used[i] * records->head.block_size[i]);
        total_bytes +=
            records->head.block_used[i] * records->head.block_size[i];
    }
    logf (LOG_LOG,
          "Total size of record index in bytes  %8" ZINT_FORMAT0,
          records->head.total_bytes);
    logf (LOG_LOG,
          "Total size with overhead             %8" ZINT_FORMAT0,
	  total_bytes);
}
