/* $Id: zebrautl.h,v 1.11 2004-12-10 11:56:21 heikki Exp $
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

#ifndef ZEBRA_UTIL_H
#define ZEBRA_UTIL_H

/* Old yaz-util includes */
#include <yaz/yconfig.h>
#include <yaz/yaz-version.h>
#include <yaz/xmalloc.h>
#include <yaz/ylog.h>  
#include <yaz/tpath.h>
#include <yaz/options.h>
#include <yaz/wrbuf.h>
#include <yaz/nmem.h>
#include <yaz/readconf.h>
#include <yaz/marcdisp.h>
#include <yaz/yaz-iconv.h>

#include <idzebra/res.h>
#include <idzebra/version.h>

/* check that we don't have all too old yaz */
#ifndef YLOG_ERRNO
#error Need a modern yaz with ylog.h
#endif

YAZ_BEGIN_CDECL
zint atoi_zn (const char *buf, zint len);
YAZ_END_CDECL

#endif
