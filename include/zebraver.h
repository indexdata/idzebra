/* $Id: zebraver.h,v 1.40 2004-08-06 12:28:22 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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

#ifndef ZEBRAVER

#define ZEBRAVER "1.4.0"

#define ZEBRADATE "$Date: 2004-08-06 12:28:22 $"

#ifdef __GNUC__
typedef long long int zint;
#define ZINT_FORMAT "%lld"
#define ZINT_FORMAT0 "lld"
#else
#ifdef WIN32
typedef __int64 zint;
#define ZINT_FORMAT "%I64d"
#define ZINT_FORMAT0 "I64d"
#else
typedef long zint;
#define ZINT_FORMAT "%ld"
#define ZINT_FORMAT0 "ld"
#endif
#endif

typedef zint SYSNO;

#endif
