/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

int zebra_check_res(Res res)
{
    int errors = 0;
    Res v = res_open(0, 0);
    
    res_add(v, "attset", "");
    res_add(v, "chdir", "");
    res_add(v, "dbaccess", "");
    res_add(v, "encoding", "");
    res_add(v, "estimatehits", "");
    res_add(v, "group", "");
    res_add(v, "index", "");
    res_add(v, "isam", "");
    res_add(v, "isamcDebug", "");
    res_add(v, "isamsDebug", "");
    res_add(v, "keyTmpDir", "");
    res_add(v, "lockDir", "");
    res_add(v, "memmax", "");
    res_add(v, "modulePath", "");
    res_add(v, "perm", "s");
    res_add(v, "passwd", "");
    res_add(v, "passwd.c", "");
    res_add(v, "profilePath", "");
    res_add(v, "rank", "");
    res_add(v, "recordCompression", "");
    res_add(v, "register", "");
    res_add(v, "root", "");
    res_add(v, "shadow", "");
    res_add(v, "segment", "");
    res_add(v, "setTmpDir", "");
    res_add(v, "sortindex", "");
    res_add(v, "sortmax", "");
    res_add(v, "staticrank", "");
    res_add(v, "threads", "");
    res_add(v, "trunclimit", "");
    res_add(v, "truncmax", "");
    res_add(v, "database", "p");
    res_add(v, "explainDatabase", "p");
    res_add(v, "fileVerboseLimit", "p");
    res_add(v, "followLinks", "p");
    res_add(v, "recordId", "p");
    res_add(v, "recordType", "ps");
    res_add(v, "storeKeys", "p");
    res_add(v, "storeData", "p");
    res_add(v, "openRW", "p");
    res_add(v, "facetNumRecs", "");
    res_add(v, "facetMaxChunks", "");
    
    errors = res_check(res, v);
 
    res_close(v);
    return errors;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

