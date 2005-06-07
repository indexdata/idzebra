/* $Id: snippet.c,v 1.1 2005-06-07 11:36:43 adam Exp $
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

#include <stddef.h>
#include <yaz/nmem.h>
#include <yaz/log.h>
#include <idzebra/snippet.h>

struct zebra_snippets {
    NMEM nmem;
    zebra_snippet_word *front;
    zebra_snippet_word *tail;
};

zebra_snippets *zebra_snippets_create()
{
    NMEM nmem = nmem_create();
    zebra_snippets *l = nmem_malloc(nmem, sizeof(*l));
    l->nmem = nmem;
    l->front = l->tail = 0;
    return l;
}

void zebra_snippets_destroy(zebra_snippets *l)
{
    if (l)
	nmem_destroy(l->nmem);
}

void zebra_snippets_append(zebra_snippets *l,
			   zint seqno, int ord, const char *term)
{
    zebra_snippets_append_match(l, seqno, ord, term, 0);
}

void zebra_snippets_append_match(zebra_snippets *l,
				 zint seqno, int ord, const char *term,
				 int match)
{
    struct zebra_snippet_word *w = nmem_malloc(l->nmem, sizeof(*w));

    w->next = 0;
    if (l->tail)
	l->tail->next = w;
    else
	l->front = w;
    l->tail = w;

    w->seqno = seqno;
    w->ord = ord;
    w->term = nmem_strdup(l->nmem, term);
    w->match = match;
}

zebra_snippet_word *zebra_snippets_list(zebra_snippets *l)
{
    return l->front;
}

void zebra_snippets_log(zebra_snippets *l, int log_level)
{
    zebra_snippet_word *w;
    for (w = l->front; w; w = w->next)
	yaz_log(log_level, "term=%s%s seqno=" ZINT_FORMAT " ord=%d",
		w->term, (w->match ? "*" : ""), w->seqno, w->ord);
}

zebra_snippets *zebra_snippets_window(zebra_snippets *doc, zebra_snippets *hit,
				      int window_size)
{
    int ord = -1;

    zebra_snippets *result = zebra_snippets_create();

    while(1)
    {
	int window_start;
	zebra_snippet_word *hit_w, *doc_w;
	int min_ord = 0; /* not set yet */
	for (hit_w = zebra_snippets_list(hit); hit_w; hit_w = hit_w->next)
	    if (hit_w->ord > ord &&
		(min_ord == 0 || hit_w->ord < min_ord))
		min_ord = hit_w->ord;
	if (min_ord == 0)
	    break;
	ord = min_ord;

	int first_seq_no_best_window = 0;
	int last_seq_no_best_window = 0;
	int number_best_window = 0;

	for (hit_w = zebra_snippets_list(hit); hit_w; hit_w = hit_w->next)
	{
	    if (hit_w->ord == ord)
	    {
		zebra_snippet_word *look_w = hit_w;
		int number_this = 0;
		int seq_no_last = 0;
		while (look_w && look_w->seqno < hit_w->seqno + window_size)
		{
		    if (look_w->ord == ord)
		    {
			seq_no_last = look_w->seqno;
			number_this++;
		    }
		    look_w = look_w->next;
		}
		if (number_this > number_best_window)
		{
		    number_best_window = number_this;
		    first_seq_no_best_window = hit_w->seqno;
		    last_seq_no_best_window = seq_no_last;
		}
	    }
	}
	yaz_log(YLOG_LOG, "ord=%d", ord);
	yaz_log(YLOG_LOG, "first_seq_no_best_window=%d", first_seq_no_best_window);
	yaz_log(YLOG_LOG, "last_seq_no_best_window=%d", last_seq_no_best_window);
	yaz_log(YLOG_LOG, "number_best_window=%d", number_best_window);

	window_start = (first_seq_no_best_window + last_seq_no_best_window -
			window_size) / 2;
	for (doc_w = zebra_snippets_list(doc); doc_w; doc_w = doc_w->next)
	    if (doc_w->ord == ord 
		&& doc_w->seqno >= window_start
		&& doc_w->seqno < window_start + window_size)
	    {
		int match = 0;
		for (hit_w = zebra_snippets_list(hit); hit_w; hit_w = hit_w->next)
		{
		    if (hit_w->ord == ord && hit_w->seqno == doc_w->seqno)
			
		    {
			match = 1;
			break;
		    }
		}
		zebra_snippets_append_match(result, doc_w->seqno, ord,
					    doc_w->term, match);
	    }
    }
    return result;
}
			 
