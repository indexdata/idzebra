/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#ifndef ZEBRA_INDEXPLUGINH
#define ZEBRA_INDEXPLUGINH

#include "index.h"

typedef int (*indexList)(ZebraHandle zh, const char *driverArg, enum zebra_recctrl_action_t action);

typedef struct 
{
	indexList idxList;
} zebra_index_plugin_object;

void addDriverFunction(indexList);
void zebraIndexBuffer(ZebraHandle zh, char *data, int dataLength, enum zebra_recctrl_action_t action, char *name);

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

