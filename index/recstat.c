/* $Id: recstat.c,v 1.13 2005-01-15 19:38:26 adam Exp $
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
    
    yaz_log (YLOG_LOG,
          "Total records                        %8" ZINT_FORMAT0,
          records->head.no_records);

    for (i = 0; i< REC_BLOCK_TYPES; i++)
    {
        yaz_log (YLOG_LOG, "Record blocks of size "ZINT_FORMAT,
              records->head.block_size[i]);
        yaz_log (YLOG_LOG,
          " Used/Total/Bytes used            "
	      ZINT_FORMAT "/" ZINT_FORMAT "/" ZINT_FORMAT,
              records->head.block_used[i], records->head.block_last[i]-1,
              records->head.block_used[i] * records->head.block_size[i]);
        total_bytes +=
            records->head.block_used[i] * records->head.block_size[i];
    }
    yaz_log (YLOG_LOG,
          "Total size of record index in bytes  %8" ZINT_FORMAT0,
          records->head.total_bytes);
    yaz_log (YLOG_LOG,
          "Total size with overhead             %8" ZINT_FORMAT0,
	  total_bytes);
}
