/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recctrl.c,v $
 * Revision 1.1  1996-10-11 10:57:24  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 *
 * Revision 1.5  1996/06/04 10:18:59  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.4  1995/12/04  17:59:24  adam
 * More work on regular expression conversion.
 *
 * Revision 1.3  1995/12/04  14:22:30  adam
 * Extra arg to recType_byName.
 * Started work on new regular expression parsed input to
 * structured records.
 *
 * Revision 1.2  1995/11/15  14:46:19  adam
 * Started work on better record management system.
 *
 * Revision 1.1  1995/09/27  12:22:28  adam
 * More work on extract in record control.
 * Field name is not in isam keys but in prefix in dictionary words.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <alexutil.h>
#include "rectext.h"
#include "recgrs.h"

RecType recType_byName (const char *name, char *subType)
{
    char *p;
    char tmpname[256];

    strcpy (tmpname, name);
    if ((p = strchr (tmpname, '.')))
    {
        *p = '\0';
        strcpy (subType, p+1);
    }
    else
        *subType = '\0';
    if (!strcmp (recTypeGrs->name, tmpname))
        return recTypeGrs;
    if (!strcmp (recTypeText->name, tmpname))
        return recTypeText;
    return NULL;
}

