/* This file is part of the Zebra server.
   Copyright (C) Index Data

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



#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "dict-p.h"

int dict_close(Dict dict)
{
    if (!dict)
        return 0;

    if (dict->rw)
    {
        void *head_buf;
        dict_bf_readp(dict->dbf, 0, &head_buf);
        memcpy(head_buf, &dict->head, sizeof(dict->head));
        dict_bf_touch(dict->dbf, 0);
    }
    dict_bf_close(dict->dbf);
    xfree(dict);
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

