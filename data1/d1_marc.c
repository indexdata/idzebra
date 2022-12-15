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

/* converts data1 tree to ISO2709/MARC record */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <yaz/log.h>
#include <yaz/snprintf.h>
#include <yaz/oid_db.h>
#include <yaz/marcdisp.h>
#include <yaz/readconf.h>
#include <yaz/xmalloc.h>
#include <yaz/tpath.h>
#include <idzebra/data1.h>

data1_marctab *data1_read_marctab(data1_handle dh, const char *file)
{
    FILE *f;
    NMEM mem = data1_nmem_get (dh);
    data1_marctab *res = (data1_marctab *)nmem_malloc(mem, sizeof(*res));
    char line[512], *argv[50];
    int lineno = 0;
    int argc;

    if (!(f = data1_path_fopen(dh, file, "r")))
        return 0;

    res->name = 0;
    res->oid = 0;
    res->next = 0;
    res->length_data_entry = 4;
    res->length_starting = 5;
    res->length_implementation = 0;
    strcpy(res->future_use, "4");

    strcpy(res->record_status, "n");
    strcpy(res->implementation_codes, "    ");
    res->indicator_length = 2;
    res->identifier_length = 2;
    res->force_indicator_length = -1;
    res->force_identifier_length = -1;
    strcpy(res->user_systems, "z  ");

    while ((argc = readconf_line(f, &lineno, line, 512, argv, 50)))
        if (!strcmp(*argv, "name"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d:Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->name = nmem_strdup(mem, argv[1]);
        }
        else if (!strcmp(*argv, "reference"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->oid = yaz_string_to_oid_nmem(yaz_oid_std(),
                                              CLASS_TAGSET, argv[1],
                                              mem);
            if (!res->oid)
            {
                yaz_log(YLOG_WARN, "%s:%d: Unknown tagset reference '%s'",
                        file, lineno, argv[1]);
                continue;
            }
        }
        else if (!strcmp(*argv, "length-data-entry"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->length_data_entry = atoi(argv[1]);
        }
        else if (!strcmp(*argv, "length-starting"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->length_starting = atoi(argv[1]);
        }
        else if (!strcmp(*argv, "length-implementation"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->length_implementation = atoi(argv[1]);
        }
        else if (!strcmp(*argv, "future-use"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->future_use[0] = argv[1][0]; /* use[1] already \0 */
        }
        else if (!strcmp(*argv, "force-indicator-length"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->force_indicator_length = atoi(argv[1]);
        }
        else if (!strcmp(*argv, "force-identifier-length"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            res->force_identifier_length = atoi(argv[1]);
        }
        else if (!strcmp(*argv, "implementation-codes"))
        {
            if (argc != 2)
            {
                yaz_log(YLOG_WARN, "%s:%d: Missing arg for %s", file, lineno,
                        *argv);
                continue;
            }
            /* up to 4 characters .. space pad */
            if (strlen(argv[1]) > 4)
                yaz_log(YLOG_WARN, "%s:%d: Max 4 characters for "
                                   "implementation-codes", file, lineno);
            else
                memcpy(res->implementation_codes, argv[1], strlen(argv[1]));
        }
        else
            yaz_log(YLOG_WARN, "%s:%d: Unknown directive '%s'", file, lineno,
                    *argv);

    fclose(f);
    return res;
}


static void get_data2(data1_node *n, int *len, char *dst, size_t max)
{
    *len = 0;
    while (n)
    {
        if (n->which == DATA1N_data)
        {
            if (dst && *len < max)
            {
                size_t copy_len = max - *len;
                if (copy_len > n->u.data.len)
                    copy_len = n->u.data.len;
                memcpy(dst + *len, n->u.data.data, copy_len);
            }
            *len += n->u.data.len;
        }
        if (n->which == DATA1N_tag && *len == 0)
            n = n->child;
        else if (n->which == DATA1N_data)
            n = n->next;
        else
            break;
    }
}

static void memint(char *p, int val, int len)
{
    char buf[10];

    if (len == 1)
        *p = val + '0';
    else
    {
        yaz_snprintf(buf, sizeof(buf), "%08d", val);
        memcpy(p, buf+8-len, len);
    }
}

/* check for indicator. non MARCXML only */
static int is_indicator(data1_marctab *p, data1_node *subf)
{
    if (p->indicator_length != 2 ||
        (subf && subf->which == DATA1N_tag && strlen(subf->u.tag.tag) == 2))
        return 1;
    return 0;
}

static int nodetomarc(data1_handle dh,
                      data1_marctab *p, data1_node *n, int selected,
                      char **buf, int *size)
{
    char leader[24];

    int len = 26;
    int dlen;
    int base_address = 25;
    int entry_p, data_p;
    char *op;
    data1_node *field, *subf;

#if 0
    data1_pr_tree(dh, n, stdout);
#endif
    yaz_log(YLOG_DEBUG, "nodetomarc");

    memcpy(leader+5, p->record_status, 1);
    memcpy(leader+6, p->implementation_codes, 4);
    memint(leader+10, p->indicator_length, 1);
    memint(leader+11, p->identifier_length, 1);
    memcpy(leader+17, p->user_systems, 3);
    memint(leader+20, p->length_data_entry, 1);
    memint(leader+21, p->length_starting, 1);
    memint(leader+22, p->length_implementation, 1);
    memcpy(leader+23, p->future_use, 1);

    for (field = n->child; field; field = field->next)
    {
        int control_field = 0;  /* 00X fields - usually! */
        int marc_xml = 0;

        if (field->which != DATA1N_tag)
            continue;
        if (selected && !field->u.tag.node_selected)
            continue;

        subf = field->child;
        if (!subf)
            continue;

        if (!yaz_matchstr(field->u.tag.tag, "mc?"))
            continue;
        else if (!strcmp(field->u.tag.tag, "leader"))
        {
            int dlen = 0;
            get_data2(subf, &dlen, leader, 24);
            continue;
        }
        else if (!strcmp(field->u.tag.tag, "controlfield"))
        {
            control_field = 1;
            marc_xml = 1;
        }
        else if (!strcmp(field->u.tag.tag, "datafield"))
        {
            control_field = 0;
            marc_xml = 1;
        }
        else if (subf->which == DATA1N_data)
        {
            control_field = 1;
            marc_xml = 0;
        }
        else
        {
            control_field = 0;
            marc_xml = 0;
        }

        len += 4 + p->length_data_entry + p->length_starting
            + p->length_implementation;
        base_address += 3 + p->length_data_entry + p->length_starting
            + p->length_implementation;

        if (!control_field)
            len += p->indicator_length;

        /* we'll allow no indicator if length is not 2 */
        /* select when old XML format, since indicator is an element */
        if (marc_xml == 0 && is_indicator (p, subf))
            subf = subf->child;

        for (; subf; subf = subf->next)
        {
            if (!control_field)
            {
                if (marc_xml && subf->which != DATA1N_tag)
                    continue; /* we skip comments, cdata .. */
                len += p->identifier_length;
            }
            get_data2(subf, &dlen, 0, 0);
            len += dlen;
        }
    }

    if (!*buf)
        *buf = (char *)xmalloc(*size = len);
    else if (*size <= len)
        *buf = (char *)xrealloc(*buf, *size = len);

    op = *buf;

    /* we know the base address now */
    memint(leader+12, base_address, 5);

    /* copy temp leader to real output buf op */
    memcpy(op, leader, 24);
    memint(op, len, 5);

    entry_p = 24;
    data_p = base_address;

    for (field = n->child; field; field = field->next)
    {
        int control_field = 0;
        int marc_xml = 0;
        const char *tag = 0;

        int data_0 = data_p;
        char indicator_data[6];

        memset(indicator_data, ' ', sizeof(indicator_data) - 1);
        indicator_data[sizeof(indicator_data)-1] = '\0';

        if (field->which != DATA1N_tag)
            continue;

        if (selected && !field->u.tag.node_selected)
            continue;

        subf = field->child;
        if (!subf)
            continue;

        if (!yaz_matchstr(field->u.tag.tag, "mc?"))
            continue;
        else if (!strcmp(field->u.tag.tag, "leader"))
            continue;
        else if (!strcmp(field->u.tag.tag, "controlfield"))
        {
            control_field = 1;
            marc_xml = 1;
        }
        else if (!strcmp(field->u.tag.tag, "datafield"))
        {
            control_field = 0;
            marc_xml = 1;
        }
        else if (subf->which == DATA1N_data)
        {
            control_field = 1;
            marc_xml = 0;
        }
        else
        {
            control_field = 0;
            marc_xml = 0;
        }
        if (marc_xml == 0 && is_indicator(p, subf))
        {
            if (strlen(subf->u.tag.tag) < sizeof(indicator_data))
               strcpy(indicator_data, subf->u.tag.tag);
            subf = subf->child;
        }
        else if (marc_xml == 1 && !control_field)
        {
            data1_xattr *xa;
            for (xa = field->u.tag.attributes; xa; xa = xa->next)
            {
                if (!strcmp(xa->name, "ind1"))
                    indicator_data[0] = xa->value[0];
                if (!strcmp(xa->name, "ind2"))
                    indicator_data[1] = xa->value[0];
                if (!strcmp(xa->name, "ind3"))
                    indicator_data[2] = xa->value[0];
            }
        }
        if (!control_field)
        {
            memcpy(op + data_p, indicator_data, p->indicator_length);
            data_p += p->indicator_length;
        }
        for (; subf; subf = subf->next)
        {
            if (!control_field)
            {
                const char *identifier = "a";
                if (marc_xml)
                {
                    data1_xattr *xa;
                    if (subf->which != DATA1N_tag)
                        continue;
                    if (strcmp(subf->u.tag.tag, "subfield"))
                        yaz_log(YLOG_WARN, "Unhandled tag %s",
                                subf->u.tag.tag);

                    for (xa = subf->u.tag.attributes; xa; xa = xa->next)
                        if (!strcmp(xa->name, "code"))
                            identifier = xa->value;
                }
                else if (subf->which != DATA1N_tag)
                    yaz_log(YLOG_WARN, "Malformed fields for marc output.");
                else
                    identifier = subf->u.tag.tag;
                op[data_p] = ISO2709_IDFS;
                memcpy(op + data_p+1, identifier, p->identifier_length-1);
                data_p += p->identifier_length;
            }
            get_data2(subf, &dlen, op + data_p, 100000);
            data_p += dlen;
        }
        op[data_p++] = ISO2709_FS;

        if (marc_xml)
        {
            data1_xattr *xa;
            for (xa = field->u.tag.attributes; xa; xa = xa->next)
                if (!strcmp(xa->name, "tag"))
                    tag = xa->value;
        }
        else
            tag = field->u.tag.tag;

        if (!tag || strlen(tag) != 3)
            tag = "000";
        memcpy(op + entry_p, tag, 3);

        entry_p += 3;
        memint(op + entry_p, data_p - data_0, p->length_data_entry);
        entry_p += p->length_data_entry;
        memint(op + entry_p, data_0 - base_address, p->length_starting);
        entry_p += p->length_starting;
        entry_p += p->length_implementation;
    }
    op[entry_p++] = ISO2709_FS;
    assert (entry_p == base_address);
    op[data_p++] = ISO2709_RS;
    assert (data_p == len);
    return len;
}

char *data1_nodetomarc(data1_handle dh, data1_marctab *p, data1_node *n,
                       int selected, int *len)
{
    int *size;
    char **buf = data1_get_map_buf(dh, &size);

    n = data1_get_root_tag(dh, n);
    if (!n)
        return 0;
    *len = nodetomarc(dh, p, n, selected, buf, size);
    return *buf;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

