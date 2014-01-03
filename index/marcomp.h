/* This file is part of the Zebra server.
   Copyright (C) Index Data

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
#ifndef MARCOMP_H
#define MARCOMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mc_subfield
{
    char *name;
    char *prefix;
    char *suffix;
    struct {
	int start;
	int end;
    } interval;
    int which;
    union {
#define MC_SF           1
#define MC_SFGROUP	2
#define MC_SFVARIANT    3
        struct mc_field *in_line;
        struct mc_subfield *child;
    } u;
    struct mc_subfield *next;
    struct mc_subfield *parent;
} mc_subfield;

#define SZ_FNAME	3
#define SZ_IND		1
#define SZ_SFNAME	1
#define SZ_PREFIX	1
#define SZ_SUFFIX	1

typedef struct mc_field
{
    char *name;
    char *ind1;
    char *ind2;
    struct {
	int start;
	int end;
    } interval;
    struct mc_subfield *list;
} mc_field;

typedef enum
{
    NOP,
    REGULAR,
    LVARIANT,
    RVARIANT,
    LGROUP,
    RGROUP,
    LINLINE,
    RINLINE,
    SUBFIELD,
    LINTERVAL,
    RINTERVAL,
} mc_token;

typedef enum
{
    EMCOK = 0,	/* first always, mondatory */
    EMCNOMEM,
    EMCF,
    EMCSF,
    EMCSFGROUP,
    EMCSFVAR,
    EMCSFINLINE,
    EMCEND	/* last always, mondatory */
} mc_errcode;

typedef struct mc_context
{
    int offset;

    int crrval;
    mc_token crrtok;

    mc_errcode errcode;

    int len;
    const char *data;
} mc_context;

mc_context *mc_mk_context(const char *s);
void mc_destroy_context(mc_context *c);

mc_field *mc_getfield(mc_context *c);
void mc_destroy_field(mc_field *p);
void mc_pr_field(mc_field *p, int offset);

mc_subfield *mc_getsubfields(mc_context *c, mc_subfield *parent);
void mc_destroy_subfield(mc_subfield *p);
void mc_destroy_subfields_recursive(mc_subfield *p);
void mc_pr_subfields(mc_subfield *p, int offset);

mc_errcode mc_errno(mc_context *c);
const char *mc_error(mc_errcode no);

#ifdef __cplusplus
}
#endif

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

