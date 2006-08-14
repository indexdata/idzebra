/* $Id: reckeys.h,v 1.5 2006-08-14 10:40:15 adam Exp $
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

#ifndef RECKEYS_H
#define RECKEYS_H

typedef struct zebra_rec_keys_t_ *zebra_rec_keys_t;

zebra_rec_keys_t zebra_rec_keys_open();

void zebra_rec_keys_close(zebra_rec_keys_t p);

void zebra_rec_keys_write(zebra_rec_keys_t keys, 
			  const char *str, size_t slen,
			  const struct it_key *key);
void zebra_rec_keys_reset(zebra_rec_keys_t keys);

int zebra_rec_keys_read(zebra_rec_keys_t keys,
			const char **str, size_t *slen,
			struct it_key *key);
int zebra_rec_keys_rewind(zebra_rec_keys_t keys);

int zebra_rec_keys_empty(zebra_rec_keys_t keys);

void zebra_rec_keys_get_buf(zebra_rec_keys_t p, char **buf, size_t *sz);

void zebra_rec_keys_set_buf(zebra_rec_keys_t p, char *buf, size_t sz,
			    int copy_buf);

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

