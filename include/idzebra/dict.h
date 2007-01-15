/* $Id: dict.h,v 1.14 2007-01-15 20:08:24 adam Exp $
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

/** \file dict.h
    \brief Zebra dictionary
    
    The dictionary is a hash that maps a string to a value.
    The value is opaque and is defined as a sequence of bytes
    with a length in the range 0 to 255.
*/

#ifndef DICT_H
#define DICT_H

#include <yaz/yconfig.h>
#include <idzebra/bfile.h>

YAZ_BEGIN_CDECL

/** \var Dict
 * \brief Dictionary handle
 *
 * Most dictionary functions operatate on a Dict type (object).
 */
typedef struct Dict_struct *Dict;

/** \brief open dictionary
    \param bfs block file storage
    \param name name of dictionary file
    \param cache number of pages to cache
    \param rw 0=read-only, 1=read&write
    \param compact_flag 1=write with compact, 0=normal paged operation
    \param page_size page size of disc block
    \returns dictionary handle
*/
YAZ_EXPORT 
Dict dict_open(BFiles bfs, const char *name, int cache, int rw,
               int compact_flag, int page_size);

/** \brief closes dictionary
    \param dict handle
    \retval 0 OK
    \retval -1 failure
*/
YAZ_EXPORT
int dict_close(Dict dict);

/** \brief insert item into dictionary
    \param dict dictionary handle
    \param p string-z with lookup string
    \param userlen length of user data (associated with p)
    \param userinfo userdata (of size userlen)
    \retval 0 p is new and inserted OK
    \retval 1 p is updated (already exists) and userinfo is modified
    \retval 2 p already exists and userinfo is unmodified (same as before)
    \retval -1 error
*/
YAZ_EXPORT
int dict_insert(Dict dict, const char *p, int userlen, void *userinfo);

/** \brief deletes item from dictionary
    \param dict dictionary handle
    \param p string-z with lookup string
    \retval 0 p not found
    \retval 1 p found and deleted
    \retval -1 error
*/
YAZ_EXPORT
int dict_delete(Dict dict, const char *p);

/** \brief lookup item in dictionary
    \param dict dictionary handle
    \param p string-z with lookup string
    \retval NULL not found
    \retval value where value[0]=userlen, value[1..userlen] is userinfo data
*/
YAZ_EXPORT
char *dict_lookup(Dict dict, const char *p);

/** \brief delete items with a given prefix from dictionary
    \param dict dictionary handle
    \param p string-z with prefix
    \param client client data to be supplied to function f
    \param f function which gets called for each item in tree
    \retval 0 OK (0 or more entries deleted)
    \retval 1 OK (1 or more entries delete)
    \retval -1 ERROR

    Function f is called for each item to be deleted.
*/
YAZ_EXPORT
int dict_delete_subtree(Dict dict, const char *p, void *client,
                        int (*f)(const char *info, void *client));


/** \brief lookup item(s) in dictionary with error correction
    \param dict dictionary handle
    \param p string-z with lookup string
    \param range number of allowed errors(extra/substituted/missing char)
    \param f function be called for each match (NULL for no call of f)
    \retval 0 OK
    \retval -1 error
    
    Function f is called for each item matched.
*/
YAZ_EXPORT
int dict_lookup_ec(Dict dict, char *p, int range, int (*f)(char *name));

/** \brief regular expression search with error correction
    \param dict dictionary handle
    \param p regular expression string-z
    \param range number of allowed errors(extra/substituted/missing char)
    \param client client data pointer to be passed to match function f
    \param max_pos on return holds maximum number of chars that match (prefix)
    \param init_pos number of leading non-error corrected chars.
    \param f function be called for each match
    \retval 0 Operation complete. Function f returned zero value always
    \retval >0 Operation incomplete. Function f returned a non-zero value
    \retval -1 error (such as bad regular expression)
    
    The function f is called for each match. If function f returns
    non-zero value the grep operation is stopped and the returned 
    non-zero value is also returned by dict_lookup_ec.
*/
YAZ_EXPORT
int dict_lookup_grep(Dict dict, const char *p, int range, void *client,
                     int *max_pos, int init_pos,
                     int (*f)(char *name, const char *info, void *client));

/** \brief dictionary scan
    \param dict dictionary handle
    \param str start pint term (string-z)
    \param before number of terms to be visited preceding str
    \param after number of terms to be visited following str
    \param client client data pointer to be passed to match function f
    \param f function be called for each matching term
    \retval 0 Successful
    \retval -1 error

    If the callback function f returns 0 the scan operation visits
    all terms in range (before to after); if the function returns non-zero
    the scan operation is cancelled.
*/
YAZ_EXPORT
int dict_scan(Dict dict, char *str, 
              int *before, int *after, void *client,
              int (*f)(char *name, const char *info, int pos, void *client));


/** \brief install character mapping handler for dict_lookup_grep
    \param dict dictionary handle
    \param vp client data to be passed to cmap function handler
    \param cmap function be called for each character
    
    This function must be called prior to using dict_grep_lookup.
    If vp is NULL, no character mapping takes place for dict_lookup_grep.
*/
YAZ_EXPORT 
void dict_grep_cmap(Dict dict, void *vp,
                    const char **(*cmap)(void *vp,
                                         const char **from, int len));

/** \brief copies one dictionary to another
    \param bfs block file handle
    \param from source dictionary file
    \param to destination dictionary file
*/
YAZ_EXPORT
int dict_copy_compact(BFiles bfs, const char *from, const char *to);

/** \brief reset Dictionary (makes it empty)
    \param dict dictionary handle
*/
YAZ_EXPORT
void dict_clean(Dict dict);

/** \brief get number of lookup operations, since dict_open 
    \param dict dictionary handle
    \returns number of operatons
*/
YAZ_EXPORT
zint dict_get_no_lookup(Dict dict);

/** \brief get number of insert operations, since dict_open 
    \param dict dictionary handle
    \returns number of operatons
*/
YAZ_EXPORT
zint dict_get_no_insert(Dict dict);

/** \brief get number of page split operations, since dict_open 
    \param dict dictionary handle
    \returns number of operatons
*/
YAZ_EXPORT
zint dict_get_no_split(Dict dict);

YAZ_END_CDECL
   
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
