/* $Id: snippet.h,v 1.3 2005-08-18 19:20:37 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#ifndef SNIPPET_H
#define SNIPPET_H

#include <idzebra/util.h>

YAZ_BEGIN_CDECL

struct zebra_snippet_word {
    zint seqno;
    int reg_type;
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
			   zint seqno, int reg_type,
			   int ord, const char *term);

YAZ_EXPORT
void zebra_snippets_append_match(zebra_snippets *l,
				 zint seqno, int reg_type,
				 int ord, const char *term,
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
