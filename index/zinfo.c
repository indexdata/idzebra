/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zinfo.c,v $
 * Revision 1.6  1998-02-17 10:29:27  adam
 * Moved towards 'automatic' EXPLAIN database.
 *
 * Revision 1.5  1997/10/27 14:33:05  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 * Revision 1.4  1997/09/25 14:57:08  adam
 * Added string.h.
 *
 * Revision 1.3  1996/05/22 08:21:59  adam
 * Added public ZebDatabaseInfo structure.
 *
 * Revision 1.2  1996/05/14 06:16:41  adam
 * Compact use/set bytes used in search service.
 *
 * Revision 1.1  1996/05/13 14:23:07  adam
 * Work on compaction of set/use bytes in dictionary.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "zinfo.h"

struct zebSUInfo {
    int set;
    int use;
    int ordinal;
};

struct zebSUInfoB {
    struct zebSUInfo info;
    struct zebSUInfoB *next;
};

struct zebDatabaseInfoB {
    struct zebSUInfoB *SUInfo;
    char *databaseName;
    int sysno;
    int readFlag;
    int dirty;
    struct zebDatabaseInfo info;
    struct zebDatabaseInfoB *next;
};

struct zebTargetInfo {
    int  dictNum;
    int  dirty;
    Records records;
    struct zebDatabaseInfoB *databaseInfo;
    struct zebDatabaseInfoB *curDatabaseInfo;
};

void zebTargetInfo_close (ZebTargetInfo *zti, int writeFlag)
{
    struct zebDatabaseInfoB *zdi, *zdi1;
    
    if (writeFlag)
    {
        char p0[4096], *p = p0;

        memcpy (p, &zti->dictNum, sizeof(zti->dictNum));
        p += sizeof(zti->dictNum);
        for (zdi = zti->databaseInfo; zdi; zdi=zdi->next)
        {
            if (zdi->dirty)
            {
                char q0[4096], *q = q0;
                struct zebSUInfoB *zsui;
                Record drec;
                int no = 0;
                
                if (zdi->sysno)
                    drec = rec_get (zti->records, zdi->sysno);
                else
                {
                    drec = rec_new (zti->records);
		    
		    drec->info[recInfo_fileType] =
			rec_strdup ("grs.explain.databaseInfo",
				    &drec->size[recInfo_fileType]);

		    drec->info[recInfo_databaseName] =
			rec_strdup ("IR-Explain-1",
				    &drec->size[recInfo_databaseName]); 
                    zdi->sysno = drec->sysno;
                }
                assert (drec);
                for (zsui = zdi->SUInfo; zsui; zsui=zsui->next)
                    no++;
		memcpy (q, &zdi->info, sizeof(zdi->info));
                q += sizeof(zdi->info);
                memcpy (q, &no, sizeof(no));
                q += sizeof(no);
                for (zsui = zdi->SUInfo; zsui; zsui=zsui->next)
                {
                    memcpy (q, &zsui->info, sizeof(zsui->info));
                    q += sizeof(zsui->info);
                }
                xfree (drec->info[recInfo_storeData]);
                drec->size[recInfo_storeData] = q-q0;
                drec->info[recInfo_storeData] = xmalloc (drec->size[recInfo_storeData]);
                memcpy (drec->info[recInfo_storeData], q0, drec->size[recInfo_storeData]);
                rec_put (zti->records, &drec);
            }
            strcpy (p, zdi->databaseName);
            p += strlen(p)+1;
            memcpy (p, &zdi->sysno, sizeof(zdi->sysno));
            p += sizeof(zdi->sysno);
        }
        *p++ = '\0';
        if (zti->dirty)
        {
            Record grec = rec_get (zti->records, 1);

            assert (grec);
            xfree (grec->info[recInfo_storeData]);
            grec->size[recInfo_storeData] = p-p0;
            grec->info[recInfo_storeData] = xmalloc (grec->size[recInfo_storeData]);
            memcpy (grec->info[recInfo_storeData], p0, grec->size[recInfo_storeData]);
            rec_put (zti->records, &grec);
        }
    }
    for (zdi = zti->databaseInfo; zdi; zdi = zdi1)
    {
        struct zebSUInfoB *zsui, *zsui1;

        zdi1 = zdi->next;
        for (zsui = zdi->SUInfo; zsui; zsui = zsui1)
        {
            zsui1 = zsui->next;
            xfree (zsui);
        }
        xfree (zdi->databaseName);
        xfree (zdi);
    }
    xfree (zti);
}

ZebTargetInfo *zebTargetInfo_open (Records records, int writeFlag)
{
    Record trec;
    ZebTargetInfo *zti;
    struct zebDatabaseInfoB **zdi;

    zti = xmalloc (sizeof(*zti));
    zti->dirty = 0;
    zti->curDatabaseInfo = NULL;
    zti->records = records;

    zdi = &zti->databaseInfo;
    
    trec = rec_get (records, 1);
    if (trec)
    {
        const char *p;

        p = trec->info[recInfo_storeData];

        memcpy (&zti->dictNum, p, sizeof(zti->dictNum));
        p += sizeof(zti->dictNum);
        while (*p)
        {
            *zdi = xmalloc (sizeof(**zdi));
            (*zdi)->SUInfo = NULL;
            (*zdi)->databaseName = xstrdup (p);
            p += strlen(p)+1;
            memcpy (&(*zdi)->sysno, p, sizeof((*zdi)->sysno));
            p += sizeof((*zdi)->sysno);
            (*zdi)->readFlag = 1;
            (*zdi)->dirty = 0;
            zdi = &(*zdi)->next;
        }
        assert (p - trec->info[recInfo_storeData] == trec->size[recInfo_storeData]-1);
    }
    else
    {
        zti->dictNum = 1;
        if (writeFlag)
        {
            trec = rec_new (records);

	    trec->info[recInfo_fileType] =
		rec_strdup ("grs.explain.targetInfo",
			    &trec->size[recInfo_fileType]); 
	    trec->info[recInfo_databaseName] =
		rec_strdup ("IR-Explain-1",
			    &trec->size[recInfo_databaseName]); 
	    trec->info[recInfo_databaseName] = xstrdup ("IR-Explain-1");
            trec->info[recInfo_storeData] = xmalloc (1+sizeof(zti->dictNum));
            memcpy (trec->info[recInfo_storeData], &zti->dictNum, sizeof(zti->dictNum));
            trec->info[recInfo_storeData][sizeof(zti->dictNum)] = '\0';
            trec->size[recInfo_storeData] = sizeof(zti->dictNum)+1;
            rec_put (records, &trec);
        }
    }
    *zdi = NULL;
    rec_rm (&trec);
    return zti;
}

static void zebTargetInfo_readDatabase (ZebTargetInfo *zti,
                                        struct zebDatabaseInfoB *zdi)
{
    const char *p;
    struct zebSUInfoB **zsuip = &zdi->SUInfo;
    int i, no;
    Record rec;

    rec = rec_get (zti->records, zdi->sysno);
    assert (rec);
    p = rec->info[recInfo_storeData];
    memcpy (&zdi->info, p, sizeof(zdi->info));
    p += sizeof(zdi->info);
    memcpy (&no, p, sizeof(no));
    p += sizeof(no);
    for (i = 0; i<no; i++)
    {
        *zsuip = xmalloc (sizeof(**zsuip));
        memcpy (&(*zsuip)->info, p, sizeof((*zsuip)->info));
        p += sizeof((*zsuip)->info);
        zsuip = &(*zsuip)->next;
    }
    *zsuip = NULL;
    zdi->readFlag = 0;
    rec_rm (&rec);
}

int zebTargetInfo_curDatabase (ZebTargetInfo *zti, const char *database)
{
    struct zebDatabaseInfoB *zdi;
    
    assert (zti);
    if (zti->curDatabaseInfo &&
        !strcmp (zti->curDatabaseInfo->databaseName, database))
        return 0;
    for (zdi = zti->databaseInfo; zdi; zdi=zdi->next)
    {
        if (!strcmp (zdi->databaseName, database))
            break;
    }
    if (!zdi)
        return -1;
    if (zdi->readFlag)
        zebTargetInfo_readDatabase (zti, zdi);
    zti->curDatabaseInfo = zdi;
    return 0;
}

int zebTargetInfo_newDatabase (ZebTargetInfo *zti, const char *database)
{
    struct zebDatabaseInfoB *zdi;

    assert (zti);
    for (zdi = zti->databaseInfo; zdi; zdi=zdi->next)
    {
        if (!strcmp (zdi->databaseName, database))
            break;
    }
    if (zdi)
        return -1;
    zdi = xmalloc (sizeof(*zdi));
    zdi->next = zti->databaseInfo;
    zti->databaseInfo = zdi;
    zdi->sysno = 0;
    zdi->readFlag = 0;
    zdi->databaseName = xstrdup (database);
    zdi->SUInfo = NULL;
    zdi->dirty = 1;
    zti->dirty = 1;
    zti->curDatabaseInfo = zdi;
    return 0;
}

int zebTargetInfo_lookupSU (ZebTargetInfo *zti, int set, int use)
{
    struct zebSUInfoB *zsui;

    assert (zti->curDatabaseInfo);
    for (zsui = zti->curDatabaseInfo->SUInfo; zsui; zsui=zsui->next)
        if (zsui->info.use == use && zsui->info.set == set)
            return zsui->info.ordinal;
    return -1;
}

int zebTargetInfo_addSU (ZebTargetInfo *zti, int set, int use)
{
    struct zebSUInfoB *zsui;

    assert (zti->curDatabaseInfo);
    for (zsui = zti->curDatabaseInfo->SUInfo; zsui; zsui=zsui->next)
        if (zsui->info.use == use && zsui->info.set == set)
            return -1;
    zsui = xmalloc (sizeof(*zsui));
    zsui->next = zti->curDatabaseInfo->SUInfo;
    zti->curDatabaseInfo->SUInfo = zsui;
    zti->curDatabaseInfo->dirty = 1;
    zti->dirty = 1;
    zsui->info.set = set;
    zsui->info.use = use;
    zsui->info.ordinal = (zti->dictNum)++;
    return zsui->info.ordinal;
}

ZebDatabaseInfo *zebTargetInfo_getDB (ZebTargetInfo *zti)
{
    assert (zti->curDatabaseInfo);

    return &zti->curDatabaseInfo->info;
}

void zebTargetInfo_setDB (ZebTargetInfo *zti, ZebDatabaseInfo *zdi)
{
    assert (zti->curDatabaseInfo);

    zti->curDatabaseInfo->dirty = 1;
    memcpy (&zti->curDatabaseInfo->info, zdi, sizeof(*zdi));
}

