/*
 * Copyright (c) 1995-1999, Index Data.
 * 
 * All rights reserved.
 * 
 * Use and redistribution in source or binary form, with or without
 * modification, of any or all of this software and documentation is
 * permitted, provided that the following conditions are met:
 * 
 * 1. This copyright and permission notice appear with all copies of the
 * software and its documentation. Notices of copyright or attribution
 * which appear at the beginning of any file must remain unchanged.
 * 
 * 2. The names of Index Data or the individual authors may not be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 * 
 * 3. Source code or binary versions of this software and its
 * documentation may be used freely in not-for-profit applications. For
 * profit applications - such as providing for-pay database services,
 * marketing a product based in whole or in part on this software or its
 * documentation, or generally distributing this software or its
 * documentation under a different license - requires a commercial
 * license from Index Data. The software may be installed and used for
 * evaluation purposes in conjunction with a commercial application for a
 * trial period no longer than 60 days.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED, OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL INDEX DATA BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR
 * NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * $Log: charmap.h,v $
 * Revision 1.5  1999-09-07 07:19:21  adam
 * Work on character mapping. Implemented replace rules.
 *
 * Revision 1.4  1997/10/27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.3  1997/09/05 15:29:59  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 */

#ifndef CHARMAP_H
#define CHARMAP_H

#include <yconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

YAZ_EXPORT extern const char *CHR_UNKNOWN;
YAZ_EXPORT extern const char *CHR_SPACE;
YAZ_EXPORT extern const char *CHR_BASE;

struct chr_t_entry;
typedef struct chr_t_entry chr_t_entry;

typedef struct chrmaptab_info *chrmaptab;

YAZ_EXPORT chrmaptab chrmaptab_create(const char *tabpath, const char *name,
				      int map_only);
YAZ_EXPORT void chrmaptab_destroy (chrmaptab tab);

YAZ_EXPORT const char **chr_map_input(chrmaptab t, const char **from, int len);
YAZ_EXPORT const char **chr_map_input_x(chrmaptab t,
					const char **from, int *len);
YAZ_EXPORT const char **chr_map_input_q(chrmaptab maptab,
					const char **from, int len,
					const char **qmap);
    
YAZ_EXPORT const char *chr_map_output(chrmaptab t, const char **from, int len);

YAZ_EXPORT unsigned char zebra_prim(char **s);

#ifdef __cplusplus
}
#endif

#endif
