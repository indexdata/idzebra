/* $Id: rob_regexp.c,v 1.1 2007-10-23 12:26:26 adam Exp $
   Copyright (C) 1995-2007
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

/** 
    \brief Rob Pike's regular expresion parser
    
    Taken verbatim from Beautiful code.. ANSIfied a bit.
 */
  

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rob_regexp.h"
#include <yaz/xmalloc.h>
#include <yaz/wrbuf.h>
#include <yaz/log.h>

static int matchhere(const char *regexp, const char *text);
static int matchstar(int c, const char *regexp, const char *text);

int zebra_rob_regexp(const char *regexp, const char *text)
{
    if (regexp[0] == '^')
        return matchhere(regexp+1, text);
    do 
    {
        if (matchhere(regexp, text))
            return 1;
    }
    while (*text++);
    return 0;
}

static int matchhere(const char *regexp, const char *text)
{
    if (regexp[0] == '\0')
        return 1;
    if (regexp[1] == '*')
        return matchstar(regexp[0], regexp+2, text);
    if (regexp[0] == '$' && regexp[1] == '\0')
        return *text == '\0';
    if (*text && (regexp[0] == '.' || regexp[0] == *text))
        return matchhere(regexp+1, text+1);
    return 0;
}

static int matchstar(int c, const char *regexp, const char *text)
{
    do
    {
        if (matchhere(regexp, text))
            return 1;
    }
    while (*text && (*text++ == c || c == '.'));
    return 0;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

