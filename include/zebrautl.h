/*
 * Copyright (C) 1995-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebrautl.h,v $
 * Revision 1.5  1999-02-02 14:50:47  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.4  1997/10/27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.3  1997/09/17 12:19:11  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.2  1997/09/05 15:30:06  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.1  1996/10/29 13:46:12  adam
 * Removed obsolete headers alexpath, alexutil. Created zebrautl.h as
 * a replacement.
 *
 */

#ifndef ZEBRA_UTIL_H
#define ZEBRA_UTIL_H

#include <yaz-util.h>
#include <res.h>

#endif
