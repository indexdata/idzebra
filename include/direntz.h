/* $Id: direntz.h,v 1.8 2003-01-13 15:43:25 adam Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/




#ifdef WIN32
/* make WIN32 version of dirent */
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dirent {
    char d_name[MAX_PATH];
};

typedef struct DIR DIR;

DIR *opendir (const char *path);
struct dirent *readdir (DIR *dd);
void closedir (DIR *dd);

#ifdef __cplusplus
}
#endif

#else
/* include UNIX version */
#include <sys/types.h>
#include <dirent.h>
#endif

