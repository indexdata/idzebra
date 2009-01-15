/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#ifndef IDZEBRA_SNIPPET_H
#define IDZEBRA_SNIPPET_H

#include <idzebra/util.h>

YAZ_BEGIN_CDECL

struct zebra_snippet_word {
    zint seqno;  /**< sequence number */
    int ord;     /**< ordinal, i.e. database,field,type */
    char *term;  /**< term itself */
    int match;   /**< both part and real match */
    int mark;    /**< part of snippet */
    int ws;      /**< white space flag (not indexed material) */
    struct zebra_snippet_word *next;
    struct zebra_snippet_word *prev;
};

typedef struct zebra_snippets zebra_snippets;
typedef struct zebra_snippet_word zebra_snippet_word;

YAZ_EXPORT
zebra_snippets *zebra_snippets_create(void);

YAZ_EXPORT
void zebra_snippets_destroy(zebra_snippets *l);

YAZ_EXPORT
void zebra_snippets_append(zebra_snippets *l,
			   zint seqno, int ws, int ord, const char *term);

YAZ_EXPORT
void zebra_snippets_appendn(zebra_snippets *l,
                            zint seqno, int ws, int ord,
                            const char *term, size_t term_len);

YAZ_EXPORT
void zebra_snippets_append_match(zebra_snippets *l,
				 zint seqno, int ws, int ord,
                                 const char *term, size_t term_len,
				 int match);

YAZ_EXPORT
zebra_snippet_word *zebra_snippets_list(zebra_snippets *l);

YAZ_EXPORT
const zebra_snippet_word *zebra_snippets_constlist(const zebra_snippets *l);

YAZ_EXPORT
void zebra_snippets_log(const zebra_snippets *l, int log_level, int all);

YAZ_EXPORT
zebra_snippets *zebra_snippets_window(const zebra_snippets *doc,
                                      const zebra_snippets *hit,
				      int window_size);

YAZ_EXPORT
void zebra_snippets_ring(zebra_snippets *doc, const zebra_snippets *hit,
                         int before, int after);


YAZ_EXPORT
const struct zebra_snippet_word *zebra_snippets_lookup(
    const zebra_snippets *doc, const zebra_snippets *hit);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

