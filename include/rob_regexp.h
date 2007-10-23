/* $Id: rob_regexp.h,v 1.2 2007-10-23 12:36:22 adam Exp $
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/** 
    \file rob_regexp.h
    \brief Rob Pike's regular expression matcher
*/

#ifndef ZEBRA_ROB_REGEXP_H
#define ZEBRA_ROB_REGEXP_H

#include <yaz/yconfig.h>

YAZ_BEGIN_CDECL

/** \brief matches a regular expression against text
    \param regexp regular expression
    \param text the text
    \retval 0 no match
    \retval 1 match

    Operators: c (literal char), . (any char), ^ (begin), $ (end),
    * (zero or more)
*/
int zebra_rob_regexp(const char *regexp, const char *text);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

