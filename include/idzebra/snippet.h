/* $Id: snippet.h,v 1.6 2006-08-14 10:40:14 adam Exp $
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

#ifndef IDZEBRA_SNIPPET_H
#define IDZEBRA_SNIPPET_H

#include <idzebra/util.h>

YAZ_BEGIN_CDECL

struct zebra_snippet_word {
    zint seqno;
    int ord;
    char *term;
    int match;
    struct zebra_snippet_word *next;
};

typedef struct zebra_snippets zebra_snippets;
typedef struct zebra_snippet_word zebra_snippet_word;

YAZ_EXPORT
zebra_snippets *zebra_snippets_create();

YAZ_EXPORT
void zebra_snippets_destroy(zebra_snippets *l);

YAZ_EXPORT
void zebra_snippets_append(zebra_snippets *l,
			   zint seqno, int ord, const char *term);

YAZ_EXPORT
void zebra_snippets_append_match(zebra_snippets *l,
				 zint seqno, int ord, const char *term,
				 int match);

YAZ_EXPORT
zebra_snippet_word *zebra_snippets_list(zebra_snippets *l);

YAZ_EXPORT
void zebra_snippets_log(zebra_snippets *l, int log_level);

YAZ_EXPORT
zebra_snippets *zebra_snippets_window(zebra_snippets *doc, zebra_snippets *hit,
				      int window_size);

YAZ_END_CDECL

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

