/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: close.c,v $
 * Revision 1.5  1994-09-01 17:49:36  adam
 * Removed stupid line. Work on insertion in dictionary. Not finished yet.
 *
 * Revision 1.4  1994/09/01  17:44:06  adam
 * depend include change.
 *
 * Revision 1.3  1994/08/18  12:40:52  adam
 * Some development of dictionary. Not finished at all!
 *
 * Revision 1.2  1994/08/17  13:32:19  adam
 * Use cache in dict - not in bfile.
 *
 * Revision 1.1  1994/08/16  16:26:47  adam
 * Added dict.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dict.h>

int dict_close (Dict dict)
{
    assert (dict);

    if (dict->rw)
    {
        void *head_buf;
        dict_bf_readp (dict->dbf, 0, &head_buf);
        memcpy (head_buf, &dict->head, sizeof(dict->head));
        dict_bf_touch (dict->dbf, 0);        
    }
    dict_bf_close (dict->dbf);
    xfree (dict);
    return 0;
}

