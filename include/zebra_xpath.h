/* This file is part of the Zebra server.
   Copyright (C) 2004-2013 Index Data

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

#ifndef ZEBRA_XPATH_H
#define ZEBRA_XPATH_H

#include <yaz/nmem.h>

#define XPATH_STEP_COUNT 10
struct xpath_predicate {
    int which;
    union {
#define XPATH_PREDICATE_RELATION 1
        struct {
            char *name;
            char *op;
            char *value;
        } relation;
#define XPATH_PREDICATE_BOOLEAN 2
        struct {
            const char *op;
            struct xpath_predicate *left;
            struct xpath_predicate *right;
        } boolean;
    } u;
};

struct xpath_location_step {
    char *part;
    struct xpath_predicate *predicate;
};

int zebra_parse_xpath_str(const char *xpath_string,
                          struct xpath_location_step *xpath,
                          int max, NMEM mem);

void dump_xp_steps (struct xpath_location_step *xpath, int no);


#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

