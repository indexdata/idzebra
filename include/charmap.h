/*
 * Copyright (c) 1995, Index Data.
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
 */

#ifndef CHARMAP_H
#define CHARMAP_H

extern const char *CHR_UNKNOWN;
extern const char *CHR_SPACE;
extern const char *CHR_BASE;

struct chr_t_entry;
typedef struct chr_t_entry chr_t_entry;

typedef struct chrmaptab
{
    chr_t_entry *input;         /* mapping table for input data */
    chr_t_entry *query_equiv;   /* mapping table for queries */
    unsigned char *output[256]; /* return mapping - for display of registers */
    int base_uppercase;         /* Start of upper-case ordinals */
} chrmaptab, *CHRMAPTAB;

chrmaptab *chr_read_maptab(const char *tabpath, const char *name);
int chr_map_chrs(chr_t_entry *t, char **from, int len, int *read, char **to,
    int max);
char **chr_map_input(chr_t_entry *t, char **from, int len);

#endif
