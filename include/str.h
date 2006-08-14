/* $Id: str.h,v 1.5.2.1 2006-08-14 10:38:56 adam Exp $
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
