/* $Id: str.h,v 1.6 2005-01-15 19:38:24 adam Exp $
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



#ifndef STR_H
#define STR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct str_index
{
    int size;
    char *data;
} str_index;

typedef struct strings_data
{
    str_index *index;
    int num_index;
    char *bulk;
} strings_data, *STRINGS;

STRINGS str_open(char *lang, char *name);
void str_close(STRINGS st);
char *str(STRINGS st, int num);
char *strf(STRINGS st, int num, ...);

#ifdef __cplusplus
}
#endif

#endif
