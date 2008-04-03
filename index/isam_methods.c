/* This file is part of the Zebra server.
   Copyright (C) 1995-2008 Index Data

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "index.h"


ISAMS_M *key_isams_m (Res res, ISAMS_M *me)
{
    isams_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->codec.start = iscz1_start;
    me->codec.decode = iscz1_decode;
    me->codec.encode = iscz1_encode;
    me->codec.stop = iscz1_stop;
    me->codec.reset = iscz1_reset;

    me->debug = atoi(res_get_def (res, "isamsDebug", "0"));

    return me;
}

ISAMC_M *key_isamc_m (Res res, ISAMC_M *me)
{
    isamc_getmethod (me);

    me->compare_item = key_compare;
    me->log_item = key_logdump_txt;

    me->codec.start = iscz1_start;
    me->codec.decode = iscz1_decode;
    me->codec.encode = iscz1_encode;
    me->codec.stop = iscz1_stop;
    me->codec.reset = iscz1_reset;

    me->debug = atoi(res_get_def (res, "isamcDebug", "0"));

    return me;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

