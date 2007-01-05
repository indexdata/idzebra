/* $Id: util.h,v 1.14 2007-01-05 10:45:11 adam Exp $
   Copyright (C) 1995-2006
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

#ifndef ZEBRA_UTIL_H
#define ZEBRA_UTIL_H

#include <yaz/yconfig.h>
#include <yaz/log.h>

#include <idzebra/version.h>

/**
  expand GCC_ATTRIBUTE if GCC is in use. See :
  http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html

  To see gcc pre-defines for c:
  gcc -E -dM -x c /dev/null
*/

#ifdef __GNUC__
#if __GNUC__ >= 4
#define ZEBRA_GCC_ATTR(x) __attribute__ (x)
#endif
#endif

#ifndef ZEBRA_GCC_ATTR
#define ZEBRA_GCC_ATTR(x)
#endif

/* check that we don't have all too old yaz */
#ifndef YLOG_ERRNO
#error Need a modern yaz with YLOG_ defines
#endif

YAZ_BEGIN_CDECL

/** \var zint
 * \brief Zebra integer
 *
 * This integer is used in various Zebra APIs to specify record identifires,
 * number of occurrences etc. It is a "large" integer and is usually
 * 64-bit on newer architectures.
 */
#ifdef WIN32
typedef __int64 zint;
#define ZINT_FORMAT0 "I64d"
#else

#ifndef ZEBRA_ZINT
#error ZEBRA_ZINT undefined. idzebra-config not in use?
#endif

#if ZEBRA_ZINT > 0
typedef long long int zint;
#define ZINT_FORMAT0 "lld"
#else
typedef long zint;
#define ZINT_FORMAT0 "ld"
#endif

#endif

#define ZINT_FORMAT "%" ZINT_FORMAT0

/** \var typedef ZEBRA_RES
 * \brief Common return type for Zebra API
 *
 * This return code indicates success with code ZEBRA_OK and failure
 * with ZEBRA_FAIL
 */
typedef short ZEBRA_RES;
#define ZEBRA_FAIL -1
#define ZEBRA_OK   0

YAZ_EXPORT zint atoi_zn(const char *buf, zint len);

YAZ_EXPORT void zebra_zint_encode(char **dst, zint pos);

YAZ_EXPORT void zebra_zint_decode(const char **src, zint *pos);

YAZ_EXPORT void zebra_exit(const char *msg);

YAZ_EXPORT zint atozint(const char *src);

YAZ_END_CDECL

#define CAST_ZINT_TO_INT(x) (int)(x)
#define CAST_ZINT_TO_DOUBLE(x) (double)(x)

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

