/* $Id: snippet.c,v 1.15 2008-01-24 16:13:29 adam Exp $
   Copyright (C) 1995-2007
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

#include <stddef.h>
#include <string.h>
#include <yaz/nmem.h>
#include <yaz/log.h>
#include <yaz/wrbuf.h>
#include <idzebra/snippet.h>

struct zebra_snippets {
    NMEM nmem;
    zebra_snippet_word *front;
    zebra_snippet_word *tail;
};

zebra_snippets *zebra_snippets_create(void)
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
			   zint seqno, int ws, int ord, const char *term)
{
    zebra_snippets_append_match(l, seqno, ws, ord, term, strlen(term), 0);
}

void zebra_snippets_appendn(zebra_snippets *l,
                            zint seqno, int ws, int ord, const char *term,
                            size_t term_len)
{
    zebra_snippets_append_match(l, seqno, ws, ord, term, term_len, 0);
}


void zebra_snippets_append_match(zebra_snippets *l,
				 zint seqno, int ws, int ord,
                                 const char *term, size_t term_len,
				 int match)
{
    struct zebra_snippet_word *w = nmem_malloc(l->nmem, sizeof(*w));

    w->next = 0;
    w->prev = l->tail;
    if (l->tail)
    {
	l->tail->next = w;
    }
    else
    {
	l->front = w;
    }
    l->tail = w;

    w->seqno = seqno;
    w->ws = ws;
    w->ord = ord;
    w->term = nmem_malloc(l->nmem, term_len+1);
    memcpy(w->term, term, term_len);
    w->term[term_len] = '\0';
    w->match = match;
    w->mark = 0;
}

zebra_snippet_word *zebra_snippets_list(zebra_snippets *l)
{
    return l->front;
}

const zebra_snippet_word *zebra_snippets_constlist(const zebra_snippets *l)
{
    return l->front;
}

void zebra_snippets_log(const zebra_snippets *l, int log_level, int all)
{
    zebra_snippet_word *w;
    for (w = l->front; w; w = w->next)
    {
        WRBUF wr_term = wrbuf_alloc();
        wrbuf_puts_escaped(wr_term, w->term);

        if (all || w->mark)
            yaz_log(log_level, "term='%s'%s mark=%d seqno=" ZINT_FORMAT " ord=%d",
                    wrbuf_cstr(wr_term), 
                    (w->match && !w->ws ? "*" : ""), w->mark,
                    w->seqno, w->ord);
        wrbuf_destroy(wr_term);
    }
}

zebra_snippets *zebra_snippets_window(const zebra_snippets *doc,
                                      const zebra_snippets *hit,
				      int window_size)
{
    int ord = -1;
    zebra_snippets *result = zebra_snippets_create();
    if (window_size == 0)
	window_size = 1000000;

    while(1)
    {
	zint window_start;
	zint first_seq_no_best_window = 0;
	zint last_seq_no_best_window = 0;
	int number_best_window = 0;
	const zebra_snippet_word *hit_w, *doc_w;
	int min_ord = 0; /* not set yet */

	for (hit_w = zebra_snippets_constlist(hit); hit_w; hit_w = hit_w->next)
	    if (hit_w->ord > ord &&
		(min_ord == 0 || hit_w->ord < min_ord))
	    {
		min_ord = hit_w->ord;
	    }
	if (min_ord == 0)
	    break;
	ord = min_ord;

	for (hit_w = zebra_snippets_constlist(hit); hit_w; hit_w = hit_w->next)
	{
	    if (hit_w->ord == ord)
	    {
		const zebra_snippet_word *look_w = hit_w;
		int number_this = 0;
		zint seq_no_last = 0;
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
	yaz_log(YLOG_DEBUG, "ord=%d", ord);
	yaz_log(YLOG_DEBUG, "first_seq_no_best_window=" ZINT_FORMAT,
                first_seq_no_best_window);
	yaz_log(YLOG_DEBUG, "last_seq_no_best_window=" ZINT_FORMAT,
                last_seq_no_best_window);
	yaz_log(YLOG_DEBUG, "number_best_window=%d", number_best_window);

	window_start = (first_seq_no_best_window + last_seq_no_best_window -
			window_size) / 2;
	for (doc_w = zebra_snippets_constlist(doc); doc_w; doc_w = doc_w->next)
	    if (doc_w->ord == ord
		&& doc_w->seqno >= window_start
		&& doc_w->seqno < window_start + window_size)
	    {
		int match = 0;
		for (hit_w = zebra_snippets_constlist(hit); hit_w;
		     hit_w = hit_w->next)
		{
		    if (hit_w->ord == ord && hit_w->seqno == doc_w->seqno)
			
		    {
			match = 1;
			break;
		    }
		}
		zebra_snippets_append_match(result, doc_w->seqno,
                                            doc_w->ws,
					    ord, doc_w->term, 
                                            strlen(doc_w->term), match);
	    }
    }
    return result;
}

static void zebra_snippets_clear(zebra_snippets *sn)
{
    zebra_snippet_word *w;

    for (w = zebra_snippets_list(sn); w; w = w->next)
    {
        w->mark = 0;
        w->match = 0;
    }
}

const struct zebra_snippet_word *zebra_snippets_lookup(
    const zebra_snippets *doc, const zebra_snippets *hit)
{
    const zebra_snippet_word *hit_w;
    for (hit_w = zebra_snippets_constlist(hit); hit_w; hit_w = hit_w->next)
    {
	const zebra_snippet_word *doc_w;
        for (doc_w = zebra_snippets_constlist(doc); doc_w; doc_w = doc_w->next)
        {
            if (doc_w->ord == hit_w->ord && doc_w->seqno == hit_w->seqno
                && !doc_w->ws)
            {
                return doc_w;
            }
        }
    }
    return 0;
}

void zebra_snippets_ring(zebra_snippets *doc, const zebra_snippets *hit,
                         int before, int after)
{
    int ord = -1;

    zebra_snippets_clear(doc);
    while (1)
    {
	const zebra_snippet_word *hit_w;
	zebra_snippet_word *doc_w;
	int min_ord = 0; /* not set yet */

	for (hit_w = zebra_snippets_constlist(hit); hit_w; hit_w = hit_w->next)
	    if (hit_w->ord > ord &&
		(min_ord == 0 || hit_w->ord < min_ord))
	    {
		min_ord = hit_w->ord;
	    }
	if (min_ord == 0)
	    break;
	ord = min_ord;

	for (hit_w = zebra_snippets_constlist(hit); hit_w; hit_w = hit_w->next)
	{
	    if (hit_w->ord == ord)
	    {
                for (doc_w = zebra_snippets_list(doc); doc_w; doc_w = doc_w->next)
                {
                    if (doc_w->ord == ord && doc_w->seqno == hit_w->seqno
                        && !doc_w->ws)
                    {
                        doc_w->match = 1;
                        doc_w->mark = 1;
                        break;
                    }
                    
                }
                /* mark following terms */
                if (doc_w)
                {
                    zebra_snippet_word *w = doc_w->next;
                    while (w)
                        if (w->ord == ord
                            && hit_w->seqno - before < w->seqno 
                            && hit_w->seqno + after > w->seqno)
                        {
                            w->mark = 1;
                            w = w->next;
                        }
                        else
                            break;
                }
                /* mark preceding terms */
                if (doc_w)
                {
                    zebra_snippet_word *w = doc_w->prev;
                    while (w)
                        if (w->ord == ord
                            && hit_w->seqno - before < w->seqno 
                            && hit_w->seqno + after > w->seqno)
                        {
                            w->mark = 1;
                            w = w->prev;
                        }
                        else
                            break;
                }
	    }
	}
    }
}

			 
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

