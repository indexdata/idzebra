/*
 * Copyright (c) 1995-1996, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isamc.h,v $
 * Revision 1.3  1996-11-01 13:35:03  adam
 * New element, max_blocks_mem, that control how many blocks of max size
 * to store in memory during isc_merge.
 *
 * Revision 1.2  1996/10/29  16:44:42  adam
 * Added isc_type, isc_block macros.
 *
 * Revision 1.1  1996/10/29  13:40:37  adam
 * First work.
 *
 */

#ifndef ISAMC_H
#define ISAMC_H

typedef struct ISAMC_s *ISAMC;
typedef int ISAMC_P;
typedef struct ISAMC_PP_s *ISAMC_PP;

typedef struct ISAMC_filecat_s {
    int bsize;         /* block size */
    int ifill;         /* initial fill */
    int mfill;         /* minimum fill */
    int mblocks;       /* maximum blocks */
} *ISAMC_filecat;

typedef struct ISAMC_M_s {
    ISAMC_filecat filecat;

    int (*compare_item)(const void *a, const void *b);

#define ISAMC_DECODE 0
#define ISAMC_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);

    int max_blocks_mem;
    int debug;
} *ISAMC_M;

typedef struct ISAMC_I_s {
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMC_I;

ISAMC_M isc_getmethod (void);

ISAMC isc_open (const char *name, int writeflag, ISAMC_M method);
int isc_close (ISAMC is);
ISAMC_P isc_merge (ISAMC is, ISAMC_P pos, ISAMC_I data);

ISAMC_PP isc_pp_open (ISAMC is, ISAMC_P pos);
void isc_pp_close (ISAMC_PP pp);
int isc_read_item (ISAMC_PP pp, char **dst);
int isc_read_key (ISAMC_PP pp, void *buf);
int isc_numkeys (ISAMC_PP pp);

#define isc_type(x) ((x) & 7)
#define isc_block(x) ((x) >> 3)

#endif
