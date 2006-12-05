/* $Id: recstat.c,v 1.8.2.2 2006-12-05 21:14:40 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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
    
    yaz_log(YLOG_LOG,
          "Total records                        %8d",
          records->head.no_records);

    for (i = 0; i< REC_BLOCK_TYPES; i++)
    {
        yaz_log(YLOG_LOG, "Record blocks of size %d",
              records->head.block_size[i]);
        yaz_log(YLOG_LOG,
          " Used/Total/Bytes used            %d/%d/%d",
              records->head.block_used[i], records->head.block_last[i]-1,
              records->head.block_used[i] * records->head.block_size[i]);
        total_bytes +=
            records->head.block_used[i] * records->head.block_size[i];
    }
    yaz_log(YLOG_LOG,
          "Total size of record index in bytes  %8d",
          records->head.total_bytes);
    yaz_log(YLOG_LOG,
          "Total size with overhead             %8d", total_bytes);
}
