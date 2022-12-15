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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaz/log.h>
#include <idzebra/data1.h>
#include <yaz/oid_db.h>

/*
 * We'll probably want to add some sort of hashed index to these lookup-
 * functions eventually.
 */

data1_datatype data1_maptype(data1_handle dh, char *t)
{
    static struct
    {
        char *tname;
        data1_datatype type;
    } types[] =
    {
        {"structured", DATA1K_structured},
        {"string", DATA1K_string},
        {"numeric", DATA1K_numeric},
        {"oid", DATA1K_oid},
        {"bool", DATA1K_bool},
        {"generalizedtime", DATA1K_generalizedtime},
        {"intunit", DATA1K_intunit},
        {"int", DATA1K_int},
        {"octetstring", DATA1K_octetstring},
        {"null", DATA1K_null},
        {NULL, (data1_datatype) -1}
    };
    int i;

    for (i = 0; types[i].tname; i++)
        if (!data1_matchstr(types[i].tname, t))
            return types[i].type;
    return DATA1K_unknown;
}

data1_tag *data1_gettagbynum(data1_handle dh, data1_tagset *s,
                             int type, int value)
{
    data1_tag *r;

    for (; s; s = s->next)
    {
        /* scan local set */
        if (type == s->type)
            for (r = s->tags; r; r = r->next)
                if (r->which == DATA1T_numeric && r->value.numeric == value)
                    return r;
        /* scan included sets */
        if (s->children &&
            (r = data1_gettagbynum(dh, s->children, type, value)))
            return r;
    }
    return 0;
}

data1_tag *data1_gettagbyname(data1_handle dh, data1_tagset *s,
                              const char *name)
{
    data1_tag *r;

    for (; s; s = s->next)
    {
        /* scan local set */
        for (r = s->tags; r; r = r->next)
        {
            data1_name *np;

            for (np = r->names; np; np = np->next)
                if (!data1_matchstr(np->name, name))
                    return r;
        }
        /* scan included sets */
        if (s->children && (r = data1_gettagbyname(dh, s->children, name)))
            return r;
    }
    return 0;
}

data1_tagset *data1_empty_tagset(data1_handle dh)
{
    data1_tagset *res =
        (data1_tagset *) nmem_malloc(data1_nmem_get(dh), sizeof(*res));
    res->name = 0;
    res->oid = 0;
    res->tags = 0;
    res->type = 0;
    res->children = 0;
    res->next = 0;
    return res;
}

data1_tagset *data1_read_tagset(data1_handle dh, const char *file, int type)
{
    NMEM mem = data1_nmem_get(dh);
    data1_tagset *res = 0;
    data1_tagset **childp;
    data1_tag **tagp;
    FILE *f;
    int lineno = 0;
    int argc;
    char *argv[50], line[512];

    if (!(f = data1_path_fopen(dh, file, "r")))
    {
        yaz_log(YLOG_WARN|YLOG_ERRNO, "%s", file);
        return 0;
    }
    res = data1_empty_tagset(dh);
    res->type = type;
    childp = &res->children;
    tagp = &res->tags;

    while ((argc = readconf_line(f, &lineno, line, 512, argv, 50)))
    {
        char *cmd = *argv;
        if (!strcmp(cmd, "tag"))
        {
            int value;
            char *names, *type, *nm;
            data1_tag *rr;
            data1_name **npp;

            if (argc != 4)
            {
                yaz_log(YLOG_WARN, "%s:%d: Bad # args to tag", file, lineno);
                continue;
            }
            value = atoi(argv[1]);
            names = argv[2];
            type = argv[3];

            rr = *tagp = (data1_tag *)nmem_malloc(mem, sizeof(*rr));
            rr->tagset = res;
            rr->next = 0;
            rr->which = DATA1T_numeric;
            rr->value.numeric = value;
            /*
             * how to deal with local numeric tags?
             */

            if (!(rr->kind = data1_maptype(dh, type)))
            {
                yaz_log(YLOG_WARN, "%s:%d: Unknown datatype %s",
                     file, lineno, type);
                fclose(f);
                return 0;
            }

            /* read namelist */
            nm = names;
            npp = &rr->names;
            do
            {
                char *e;

                *npp = (data1_name *)nmem_malloc(mem, sizeof(**npp));
                if ((e = strchr(nm, '/')))
                    *(e++) = '\0';
                (*npp)->name = nmem_strdup(mem, nm);
                (*npp)->next = 0;
                npp = &(*npp)->next;
                nm = e;
            }
            while (nm);
            tagp = &rr->next;
        }
        else if (!strcmp(cmd, "name"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Bad # args to name", file, lineno);
                continue;
            }
            res->name = nmem_strdup(mem, argv[1]);
        }
        else if (!strcmp(cmd, "reference"))
        {
            char *name;

            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Bad # args to reference",
                        file, lineno);
                continue;
            }
            name = argv[1];
            res->oid = yaz_string_to_oid_nmem(yaz_oid_std(),
                                              CLASS_TAGSET, name, mem);
            if (!res->oid)
            {
                yaz_log(YLOG_WARN, "%s:%d: Unknown tagset ref '%s'",
                        file, lineno, name);
                continue;
            }
        }
        else if (!strcmp(cmd, "type"))
        {
            if (argc != 2)
            {
                yaz_log (YLOG_WARN, "%s:%d: Bad # args to type", file, lineno);
                continue;
            }
            if (!res->type)
                res->type = atoi(argv[1]);
        }
        else if (!strcmp(cmd, "include"))
        {
            char *name;
            int type = 0;

            if (argc < 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Bad # args to include",
                        file, lineno);
                continue;
            }
            name = argv[1];
            if (argc == 3)
                type = atoi(argv[2]);
            *childp = data1_read_tagset(dh, name, type);
            if (!(*childp))
            {
                yaz_log(YLOG_WARN, "%s:%d: Inclusion failed for tagset %s",
                        file, lineno, name);
                continue;
            }
            childp = &(*childp)->next;
        }
        else
        {
            yaz_log(YLOG_WARN, "%s:%d: Unknown directive '%s'",
                    file, lineno, cmd);
        }
    }
    fclose(f);
    return res;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

