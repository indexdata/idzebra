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

/** testlib - utilities for the api tests */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>

#include <assert.h>
#include <yaz/log.h>
#include <yaz/pquery.h>
#include <yaz/oid_db.h>
#include <idzebra/api.h>
#include "testlib.h"

int log_level = YLOG_LOG;

/*
 * tl_start_up : do common start things, and a zebra_start
 *    - build the name of logfile from argv[0], and open it
 *      if no argv passed, do not open a log
 *    - read zebra.cfg from env var srcdir if it exists; otherwise current dir
 *      default to zebra.cfg, if no name is given
 */
ZebraService tl_start_up(char *cfgname, int argc, char **argv)
{
#if HAVE_SYS_RESOURCE_H
#if HAVE_SYS_TIME_H
    struct rlimit rlim;
    rlim.rlim_cur = 60;
    rlim.rlim_max = 60;
    setrlimit(RLIMIT_CPU, &rlim);
#endif
#endif
    return tl_zebra_start(cfgname);
}

/**
 * get_srcdir: return env srcdir or . (if does does not exist)
 */
const char *tl_get_srcdir(void)
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir || ! *srcdir)
        srcdir = ".";
    return srcdir;

}
/** tl_zebra_start - do a zebra_start with a decent config name */
ZebraService tl_zebra_start(const char *cfgname)
{
    char cfg[FILENAME_MAX];
    const char *srcdir = tl_get_srcdir();
    if (!cfgname || ! *cfgname )
        cfgname="zebra.cfg";

    yaz_snprintf(cfg, sizeof(cfg), "%s/%s", srcdir, cfgname);
    return zebra_start(cfg);
}

/** tl_close_down closes down the zebra, logfile, nmem, xmalloc etc. logs an OK */
int tl_close_down(ZebraHandle zh, ZebraService zs)
{
    if (zh)
        zebra_close(zh);
    if (zs)
        zebra_stop(zs);

    xmalloc_trav("x");
    return 1;
}

/** inits the database and inserts test data */

int tl_init_data(ZebraHandle zh, const char **recs)
{
    ZEBRA_RES res;

    if (!zh)
        return 0;

    if (zebra_select_database(zh, "Default") != ZEBRA_OK)
        return 0;

    yaz_log(log_level, "going to call init");
    res = zebra_init(zh);
    if (res == ZEBRA_FAIL)
    {
        yaz_log(log_level, "init_data: zebra_init failed with %d", res);
        printf("init_data failed with %d\n", res);
        return 0;
    }
    if (recs)
    {
        int i;
        if (zebra_begin_trans (zh, 1) != ZEBRA_OK)
            return 0;
        for (i = 0; recs[i]; i++)
        {
            if (zebra_add_record(zh, recs[i], strlen(recs[i])) != ZEBRA_OK)
            {
                if (zebra_end_trans(zh) != ZEBRA_OK)
                    return 0;
                return 0;
            }
        }
        if (zebra_end_trans(zh) != ZEBRA_OK)
            return 0;
        zebra_commit(zh);
    }
    return 1;
}

int tl_query_x(ZebraHandle zh, const char *query, zint exphits, int experror)
{
    ODR odr;
    YAZ_PQF_Parser parser;
    Z_RPNQuery *rpn;
    const char *setname="rsetname";
    zint hits;
    ZEBRA_RES rc;

    yaz_log(log_level, "======================================");
    yaz_log(log_level, "query: %s", query);
    odr = odr_createmem (ODR_DECODE);
    if (!odr)
        return 0;

    parser = yaz_pqf_create();
    rpn = yaz_pqf_parse(parser, odr, query);
    yaz_pqf_destroy(parser);
    if (!rpn)
    {
        yaz_log(log_level, "could not parse pqf query %s\n", query);
        printf("could not parse pqf query %s\n", query);
        odr_destroy(odr);
        return 0;
    }

    rc = zebra_search_RPN(zh, odr, rpn, setname, &hits);
    odr_destroy(odr);
    if (experror)
    {
        int code;
        if (rc != ZEBRA_FAIL)
        {
            yaz_log(log_level, "search returned %d (OK), but error was "
                    "expected", rc);
            printf("Error: search returned %d (OK), but error was expected\n"
                   "%s\n",  rc, query);
            return 0;
        }
        code = zebra_errCode(zh);
        if (code != experror)
        {
            yaz_log(log_level, "search returned error code %d, but error %d "
                    "was expected", code, experror);
            printf("Error: search returned error code %d, but error %d was "
                   "expected\n%s\n",
                   code, experror, query);
            return 0;
        }
    }
    else
    {
        if (rc == ZEBRA_FAIL) {
            int code = zebra_errCode(zh);
            yaz_log(log_level, "search returned %d. Code %d", rc, code);

            printf("Error: search returned %d. Code %d\n%s\n", rc,
                   code, query);
            return 0;
        }
        if (exphits != -1 && hits != exphits)
        {
            yaz_log(log_level, "search returned " ZINT_FORMAT
                   " hits instead of " ZINT_FORMAT, hits, exphits);
            printf("Error: search returned " ZINT_FORMAT
                   " hits instead of " ZINT_FORMAT "\n%s\n",
                   hits, exphits, query);
            return 0;
        }
    }
    return 1;
}


int tl_query(ZebraHandle zh, const char *query, zint exphits)
{
    return tl_query_x(zh, query, exphits, 0);
}

int tl_scan(ZebraHandle zh, const char *query,
            int pos, int num,
            int exp_pos, int exp_num, int exp_partial,
            const char **exp_entries)
{
    int ret = 1;
    ODR odr = odr_createmem(ODR_ENCODE);
    ZebraScanEntry *entries = 0;
    int partial = -123;
    ZEBRA_RES res;

    yaz_log(log_level, "======================================");
    yaz_log(log_level, "scan: pos=%d num=%d %s", pos, num, query);

    res = zebra_scan_PQF(zh, odr, query, &pos, &num, &entries, &partial,
                         0 /* setname */);

    if (partial == -123)
    {
        printf("Error: scan returned OK, but partial was not set\n"
               "%s\n", query);
        ret = 0;
    }
    if (partial != exp_partial)
    {
        printf("Error: scan OK, with partial/expected %d/%d\n",
               partial, exp_partial);
        ret = 0;
    }
    if (res != ZEBRA_OK) /* failure */
    {
        if (exp_entries)
        {
            printf("Error: scan failed, but no error was expected\n");
            ret = 0;
        }
    }
    else
    {
        if (!exp_entries)
        {
            printf("Error: scan OK, but error was expected\n");
            ret = 0;
        }
        else
        {
            int fails = 0;
            if (num != exp_num)
            {
                printf("Error: scan OK, with num/expected %d/%d\n",
                       num, exp_num);
                fails++;
            }
            if (pos != exp_pos)
            {
                printf("Error: scan OK, with pos/expected %d/%d\n",
                       pos, exp_pos);
                fails++;
            }
            if (!fails)
            {
                if (exp_entries)
                {
                    int i;
                    for (i = 0; i<num; i++)
                    {
                        if (strcmp(exp_entries[i], entries[i].term))
                        {
                            printf("Error: scan OK of %s, no %d got=%s exp=%s\n",
                                   query, i, entries[i].term, exp_entries[i]);
                            fails++;
                        }
                    }
                }
            }
            if (fails)
                ret = 0;
        }
    }
    odr_destroy(odr);
    return ret;
}

/**
 * makes a query, checks number of hits, and for the first hit, that
 * it contains the given string, and that it gets the right score
 */
int tl_ranking_query(ZebraHandle zh, char *query,
                     int exphits, char *firstrec, int firstscore)
{
    ZebraRetrievalRecord retrievalRecord[10];
    ODR odr_output = 0;
    const char *setname = "rsetname";
    int rc;
    int i;
    int ret = 1;

    if (!tl_query(zh, query, exphits))
        return 0;

    for (i = 0; i<10; i++)
        retrievalRecord[i].position = i+1;

    odr_output = odr_createmem(ODR_ENCODE);
    rc = zebra_records_retrieve(zh, odr_output, setname, 0,
                                yaz_oid_recsyn_xml, exphits, retrievalRecord);
    if (rc != ZEBRA_OK)
        ret = 0;
    else if (!strstr(retrievalRecord[0].buf, firstrec))
    {
        printf("Error: Got the wrong record first\n");
        printf("Expected '%s' but got\n", firstrec);
        printf("%.*s\n", retrievalRecord[0].len, retrievalRecord[0].buf);
        ret = 0;
    }
    else if (retrievalRecord[0].score != firstscore)
    {
        printf("Error: first rec got score %d instead of %d\n",
               retrievalRecord[0].score, firstscore);
        ret = 0;
    }
    odr_destroy (odr_output);
    return ret;
}

int tl_meta_query(ZebraHandle zh, char *query, int exphits,
                  zint *ids)
{
    ZebraMetaRecord *meta;
    const char *setname= "rsetname";
    zint *positions = 0;
    int i, ret = 1;

    if (!tl_query(zh, query, exphits))
        return 0;

    positions = (zint *) xmalloc(1 + (exphits * sizeof(zint)));
    for (i = 0; i<exphits; i++)
        positions[i] = i+1;

    meta = zebra_meta_records_create(zh, setname,  exphits, positions);

    if (!meta)
    {
        printf("Error: retrieve returned error\n%s\n", query);
        xfree(positions);
        return 0;
    }

    for (i = 0; i<exphits; i++)
    {
        if (meta[i].sysno != ids[i])
        {
            printf("Expected id=" ZINT_FORMAT " but got id=" ZINT_FORMAT "\n",
                   ids[i], meta[i].sysno);
            ret = 0;
        }
    }
    zebra_meta_records_destroy(zh, meta, exphits);
    xfree(positions);
    return ret;
}

int tl_sort(ZebraHandle zh, const char *query, zint hits, zint *exp)
{
    ZebraMetaRecord *recs;
    zint i;
    int errs = 0;
    zint min_val_recs = 0;
    zint min_val_exp = 0;

    assert(query);
    if (!tl_query(zh, query, hits))
        return 0;

    recs = zebra_meta_records_create_range (zh, "rsetname", 1, 4);
    if (!recs)
        return 0;

    /* find min for each sequence to get proper base offset */
    for (i = 0; i<hits; i++)
    {
        if (min_val_recs == 0 || recs[i].sysno < min_val_recs)
            min_val_recs = recs[i].sysno;
        if (min_val_exp == 0 || exp[i] < min_val_exp)
            min_val_exp = exp[i];
    }

    /* compare sequences using base offset */
    for (i = 0; i<hits; i++)
        if ((recs[i].sysno-min_val_recs) != (exp[i]-min_val_exp))
            errs++;
    if (errs)
    {
        printf("Sequence not in right order for query\n%s\ngot exp\n",
               query);
        for (i = 0; i<hits; i++)
            printf(" " ZINT_FORMAT "   " ZINT_FORMAT "\n",
                   recs[i].sysno, exp[i]);
    }
    zebra_meta_records_destroy (zh, recs, 4);

    if (errs)
        return 0;
    return 1;
}


struct finfo {
    const char *name;
    int occurred;
};

static void filter_cb(void *cd, const char *name)
{
    struct finfo *f = (struct finfo*) cd;
    if (!strcmp(f->name, name))
        f->occurred = 1;
}

void tl_check_filter(ZebraService zs, const char *name)
{
    struct finfo f;

    f.name = name;
    f.occurred = 0;
    zebra_filter_info(zs, &f, filter_cb);
    if (!f.occurred)
    {
        yaz_log(YLOG_WARN, "Filter %s does not exist.", name);
        exit(0);
    }
}

ZEBRA_RES tl_fetch(ZebraHandle zh, int position, const char *element_set,
                   const Odr_oid * format, ODR odr,
                   const char **rec_buf, size_t *rec_len)
{
    ZebraRetrievalRecord retrievalRecord[1];
    Z_RecordComposition *comp;
    ZEBRA_RES res;

    retrievalRecord[0].position = position;

    yaz_set_esn(&comp, element_set, odr->mem);

    res = zebra_records_retrieve(zh, odr, "rsetname", comp, format, 1,
                                 retrievalRecord);
    if (res != ZEBRA_OK)
    {
        int code = zebra_errCode(zh);
        yaz_log(YLOG_FATAL, "zebra_records_retrieve returned error %d",
                code);
    }
    else
    {
        *rec_buf = retrievalRecord[0].buf;
        *rec_len = retrievalRecord[0].len;
    }
    return res;
}

ZEBRA_RES tl_fetch_compare(ZebraHandle zh,
                           int position, const char *element_set,
                           const Odr_oid *format, const char *cmp_rec)
{
    const char *rec_buf = 0;
    size_t rec_len = 0;
    ODR odr = odr_createmem(ODR_ENCODE);
    ZEBRA_RES res = tl_fetch(zh, position, element_set, format, odr,
                             &rec_buf, &rec_len);
    if (res == ZEBRA_OK)
    {
        if (strlen(cmp_rec) != rec_len)
            res = ZEBRA_FAIL;
        else if (memcmp(cmp_rec, rec_buf, rec_len))
            res = ZEBRA_FAIL;
        if (res == ZEBRA_FAIL)
        {
            int l = rec_len;
            yaz_log(YLOG_LOG, "Expected: %s", cmp_rec);
            yaz_log(YLOG_LOG, "Got: %.*s", l, rec_buf);
        }
    }
    odr_destroy(odr);
    return res;
}

ZEBRA_RES tl_fetch_first_compare(ZebraHandle zh,
                                 const char *element_set,
                                 const Odr_oid *format, const char *cmp_rec)
{
    return tl_fetch_compare(zh, 1, element_set, format, cmp_rec);
}

void tl_profile_path(ZebraHandle zh)
{
    char profile_path[FILENAME_MAX];
    yaz_snprintf(profile_path, sizeof(profile_path), "%s:%s/../../tab",
                tl_get_srcdir(), tl_get_srcdir());
    zebra_set_resource(zh, "profilePath", profile_path);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

